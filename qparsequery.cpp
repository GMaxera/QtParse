#include "qparsequery.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

QParseQuery::QParseQuery( QString parseClassName, QMetaObject metaParseObject )
	: QObject(QParse::instance())
	, cacheControl(QParse::AlwaysCache)
	, metaParseObject(metaParseObject)
	, parseClassName(parseClassName) {
	queryRequest = new QParseRequest(parseClassName);
}

QParseQuery* QParseQuery::whereIn(QString property, QStringList values) {
	QJsonObject in;
	in["$in"] = QJsonArray::fromStringList( values );
	where[property] = in;
	return this;
}

QParseQuery* QParseQuery::orderBy( QString property, bool descending ) {
	if ( descending ) {
		queryRequest->addOption( "order", QString("-")+property );
	} else {
		queryRequest->addOption( "order", property );
	}
	return this;
}

void QParseQuery::query() {
	// add the where clause
	if ( !where.isEmpty() ) {
		queryRequest->addOption( "where", QJsonDocument(where).toJson(QJsonDocument::Compact) );
	}
	QParseReply* reply = QParse::instance()->get( queryRequest );
	connect( reply, &QParseReply::finished, this, &QParseQuery::onQueryReply );
}

void QParseQuery::onQueryReply( QParseReply* reply ) {
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
	reply->deleteLater();
}

QParse::CacheControl QParseQuery::getCacheControl() const {
	return cacheControl;
}

void QParseQuery::setCacheControl(const QParse::CacheControl &value) {
	queryRequest->setCacheControl( value );
	cacheControl = value;
}
