#include "qparseobject.h"
#include "qparse.h"
#include "qparserequest.h"
#include <QDebug>

QParseObject::QParseObject()
	// no parent in order to let QML to take ownership
	: QObject()
	, id()
	, updating(false) {
}

QParseObject::QParseObject(QString id)
	// parent to cloud interface to prevent the QML to take ownership
	: QObject(QParse::instance())
	, id(id)
	, updating(false) {
}

void UserObject::updateDone() {
	updating = false;
	pulledAt = QDateTime::currentDateTime();
	emit updatingChanged( updating );
	emit updatingFinished();
}

QString QParseObject::getId() {
	return id;
}

bool QParseObject::isUpdating() {
	return updating;
}

void QParseObject::forceUpdate() {
	if ( id.isEmpty() ) return;
	if ( updating ) return;
	updating = true;
	QParseRequest* update = new QParseRequest(this);
	QParseReply* reply = QParse::instance()->get( update );

	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadUserData;
	opdata->isJSON = true;
	opdata->user = userObject;
	opdata->phase = 0;
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("include"), QString("friends")) );
	operations[get("/users/"+userObject->getId(), params)] = opdata;


	CloudInterface::instance()->downloadUserData( this );
}

void QParseObject::updateIfOld( QDateTime validTime ) {
	if ( pulledAt < validTime ) {
		forceUpdate();
	}
}
