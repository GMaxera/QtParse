
#include "qparsereply.h"
#include "qparse.h"
#include "qparserequest.h"

QParseReply::QParseReply(QParseRequest *request, QParse *parent)
	: QObject(parent)
	, request(request)
	, isJSON(true) {

}

bool QParseReply::getIsJSON() const
{
	return isJSON;
}

void QParseReply::setIsJSON(bool value)
{
	isJSON = value;
}
