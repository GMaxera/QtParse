#include "qparseobject.h"
#include "qparse.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QDebug>

QParseObject::QParseObject( QObject* parent )
	: QObject(parent)
	, id()
	, updating(false) {
}

QParseObject::QParseObject(QString id, QObject* parent)
	: QObject(parent)
	, id(id)
	, updating(false) {
}

QString QParseObject::getId() {
	return id;
}

bool QParseObject::isUpdating() {
	return updating;
}

void QParseObject::forceUpdate() {
	if ( id.isEmpty() || updating || saving ) return;
	updating = true;
	QParseRequest* update = new QParseRequest(parseClassName());
	update->setParseObject( this );
	QParseReply* reply = QParse::instance()->get( update );
	connect( reply, &QParseReply::finished, this, &QParseObject::onUpdateReply );
}

void QParseObject::updateIfOld( QDateTime validTime ) {
	if ( pulledAt < validTime ) {
		forceUpdate();
	}
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
	if ( id.isEmpty() ) {
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
		data[property] = QJsonValue::fromVariant(this->property( property.toLatin1().data() ));
	}
	return data;
}

void QParseObject::onUpdateReply() {
	updating = false;
	pulledAt = QDateTime::currentDateTime();
	emit updatingChanged( updating );
	emit updatingDone();
}

void QParseObject::onSaveReply() {
	saving = false;
	// if id.isEmpty a new object has been created
	emit savingChanged( saving );
	emit savingDone();
}
