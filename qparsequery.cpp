#include "qparsequery.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

void QParseQuery::query() {
	QParseRequest* query = new QParseRequest(parseClassName);
	QParseReply* reply = QParse::instance()->get( query );
	connect( reply, &QParseReply::finished, this, &QParseQuery::onQueryReply );
}

void QParseQuery::onQueryReply( QParseReply* reply ) {
	qDebug() << "QParseQuery REPLY: " << reply->getJson();
	if ( reply->getHasError() ) {
		emit queryError( reply->getErrorMessage() );
		return;
	}
	// elaborate the result and create objects
	QParse* parse = QParse::instance();
	QJsonArray results = reply->getJson()["results"].toArray();
	QList<QParseObject*> parseObjects;
	for( int i=0; i<results.count(); i++ ) {
		QJsonObject object = results.at(i).toObject();
		// call the constructor passing the json object data
		QParseObject* parseObject = qobject_cast<QParseObject*>(metaParseObject.newInstance( Q_ARG(QJsonObject, object), Q_ARG(QObject*, parse) ));
		parseObjects << parseObject;
	}
	emit queryResults( parseObjects );
}
