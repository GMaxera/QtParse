#ifndef QPARSEUSER_H
#define QPARSEUSER_H

#include "qparseobject.h"

/*  */
class QParseUser : public QParseObject {
	Q_OBJECT
	//! the session token; SessionToken returned by PARSE never expires
	Q_PROPERTY( QString token MEMBER token )
public:
	QString getToken() const;
	void setToken(const QString &value);
public slots:
signals:
protected:
	//! return the class name used on PARSE for this object
	virtual QString parseClassName();
	//! return the list of properties used on PARSE for this object
	virtual QStringList parseProperties();
private:
	//! the session token
	QString token;
};

#endif // QPARSEUSER_H
