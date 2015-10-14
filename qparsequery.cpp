#include "qparsequery.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QDebug>

void QParseQuery::query() {
	QParseRequest* query = new QParseRequest(parseClassName);
	QParseReply* reply = QParse::instance()->get( query );
	connect( reply, &QParseReply::finished, this, &QParseQuery::onQueryReply );
}

void QParseQuery::onQueryReply( QParseReply* reply ) {
	qDebug() << "QParseQuery REPLY: " << reply->getJson();
}
