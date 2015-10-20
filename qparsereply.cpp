
#include "qparsereply.h"
#include "qparse.h"
#include "qparserequest.h"

QParseReply::QParseReply(QParseRequest *request, QParse *parent)
	: QObject(parent)
	, request(request)
	, isJson(true)
	, hasError(false)
	, errorMessage()
	, errorCode(0) {

}

QParseRequest* QParseReply::getRequest() const {
	return request;
}

bool QParseReply::getIsJson() const {
	return isJson;
}

void QParseReply::setIsJson(bool value) {
	isJson = value;
}

QJsonObject QParseReply::getJson() const {
	return json;
}

void QParseReply::setJson(const QJsonObject &value) {
	json = value;
}

QUrl QParseReply::getLocalUrl() const {
	return localUrl;
}

void QParseReply::setLocalUrl(const QUrl &value) {
	localUrl = value;
}

bool QParseReply::getHasError() const {
	return hasError;
}

void QParseReply::setHasError(bool value) {
	hasError = value;
}

QString QParseReply::getErrorMessage() const {
	return errorMessage;
}

void QParseReply::setErrorMessage(const QString &value) {
	errorMessage = value;
}

int QParseReply::getErrorCode() const {
	return errorCode;
}

void QParseReply::setErrorCode(int value) {
	errorCode = value;
}
