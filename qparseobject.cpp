#include "qparseobject.h"
#include "qparse.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QDebug>

QParseObject::QParseObject( QObject* parent )
	: QObject(parent)
	, objectId()
	, createdAt()
	, updatedAt()
	, updating(false)
	, saving(false) {
}

QParseObject::QParseObject(QJsonObject jsonData, QObject* parent)
	: QObject(parent)
	, objectId()
	, createdAt()
	, updatedAt()
	, updating(false)
	, saving(false) {
	objectId = jsonData["objectId"].toString();
	createdAt = QParseDate( jsonData["createdAt"].toString() );
	updatedAt = QParseDate( jsonData["updatedAt"].toString() );
}

QString QParseObject::getObjectId() const {
	return objectId;
}

QParseDate QParseObject::getCreatedAt() const {
	return createdAt;
}

QParseDate QParseObject::getUpdatedAt() const {
	return updatedAt;
}

bool QParseObject::isUpdating() {
	return updating;
}

void QParseObject::update() {
	if ( objectId.isEmpty() || updating || saving ) return;
	updating = true;
	QParseRequest* update = new QParseRequest(parseClassName());
	update->setParseObject( this );
	QParseReply* reply = QParse::instance()->get( update );
	connect( reply, &QParseReply::finished, this, &QParseObject::onUpdateReply );
}

bool QParseObject::isSaving() {
	return saving;
}

void QParseObject::save() {
	if ( updating || saving ) return;
	saving = true;
	QParseRequest* save = new QParseRequest(parseClassName());
	save->setParseObject( this );
	QParseReply* reply;
	if ( objectId.isEmpty() ) {
		// create the object
		reply = QParse::instance()->post( save );
	} else {
		// updated the object
		reply = QParse::instance()->put( save );
	}
	connect( reply, &QParseReply::finished, this, &QParseObject::onSaveReply );
}

QJsonObject QParseObject::toJson( bool onlyChanged ) {
	Q_UNUSED( onlyChanged )
	QJsonObject data;
	foreach( QString property, parseProperties() ) {
		QVariant value = this->property( property.toLatin1().data() );
		// handle PARSE specific data
		if ( value.canConvert<QParseObject*>() ) {
			// pointer to Parse object
			data[property] = value.value<QParseObject*>()->getJsonPointer();
		} else if ( value.canConvert<QParseDate>() ) {
			// Parse Date type
			data[property] = value.value<QParseDate>().toJson();
		} else if ( value.canConvert<QParseFile*>() ) {
			// Parse File type
			data[property] = value.value<QParseFile*>()->toJson();
		} else {
			data[property] = QJsonValue::fromVariant(this->property( property.toLatin1().data() ));
		}
	}
	return data;
}

QJsonObject QParseObject::getJsonPointer() {
	QJsonObject pointer;
	pointer["__type"] = "Pointer";
	pointer["className"] = parseClassName();
	pointer["objectId"] = getObjectId();
	return pointer;
}

void QParseObject::onUpdateReply( QParseReply* reply ) {
	updating = false;
	emit updatingChanged( updating );
	emit updatingDone();
	reply->deleteLater();
}

void QParseObject::onSaveReply( QParseReply* reply ) {
	saving = false;
	// if id.isEmpty a new object has been created
	emit savingChanged( saving );
	emit savingDone();
	reply->deleteLater();
}
