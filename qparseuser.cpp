
#include "qparse.h"
#include "qparseuser.h"
#include <QDebug>

QString QParseUser::getToken() const {
	return token;
}

void QParseUser::setToken(const QString &value) {
	token = value;
}

QString QParseUser::parseClassName() {
	return "_Users";
}

QStringList QParseUser::parseProperties() {
	return QStringList() << "id";
}
