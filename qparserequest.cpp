
#include "qparse.h"
#include "qparserequest.h"

QParseRequest::QParseRequest( QString parseClassName )
	: QObject(QParse::instance())
	, parseClassName(parseClassName)
	, parseObject(NULL)
	, params() {
}

void QParseRequest::addOption( QString name, QString value ) {
	params.append( qMakePair(name, value) );
}

QList< QPair<QString,QString> > QParseRequest::getOptions() {
	return params;
}

QParseObject *QParseRequest::getParseObject() const
{
    return parseObject;
}

void QParseRequest::setParseObject(QParseObject *value)
{
    parseObject = value;
}

QString QParseRequest::getParseClassName() const
{
    return parseClassName;
}
