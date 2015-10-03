
#include "qparserequest.h"

QParseRequest::QParseRequest( QString parseClassName, QParseObject *parent )
	: QObject(parent)
	, parseClassName(parseClassName)
	, params() {
}

void QParseRequest::addOption( QString name, QString value ) {
	params.append( qMakePair(name, value) );
}
