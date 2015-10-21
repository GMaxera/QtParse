
#include "qparse.h"
#include "qparserequest.h"

QParseRequest::QParseRequest( QString parseClassName )
	: QObject(QParse::instance())
	, parseClassName(parseClassName)
	, parseObject(NULL)
	, parseFile(NULL)
	, cacheControl(QParse::AlwaysCache)
	, params() {
}

QParseRequest::QParseRequest( QParseFile* parseFile )
	: QObject(QParse::instance())
	, parseClassName()
	, parseObject(NULL)
	, parseFile(parseFile)
	, cacheControl(QParse::AlwaysCache)
	, params() {
}

void QParseRequest::addOption( QString name, QString value ) {
	params.append( qMakePair(name, value) );
}

QList< QPair<QString,QString> > QParseRequest::getOptions() {
	return params;
}

QParse::CacheControl QParseRequest::getCacheControl() const {
	return cacheControl;
}

void QParseRequest::setCacheControl(const QParse::CacheControl &value) {
	cacheControl = value;
}

QParseFile* QParseRequest::getParseFile() const {
	return parseFile;
}

void QParseRequest::setParseFile(QParseFile* value) {
	parseFile = value;
}

QParseObject* QParseRequest::getParseObject() const {
	return parseObject;
}

void QParseRequest::setParseObject(QParseObject *value) {
	parseObject = value;
}

QString QParseRequest::getParseClassName() const {
    return parseClassName;
}
