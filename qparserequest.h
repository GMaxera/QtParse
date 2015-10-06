#ifndef QPARSEREQUEST_H
#define QPARSEREQUEST_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QString>

class QParseObject;

/*! It represent a request to perform to PARSE.
 *  It can be a query, posting a file or object, retrieving single object
 *
 *  Submit a request to QParse singleton and use the QParseReply to collect results
 *
 */
class QParseRequest : public QObject {
	Q_OBJECT
	//! the parse class name involved to this request
	Q_PROPERTY( QString parseClassName MEMBER parseClassName CONSTANT )
	//! the target object of this request (if null the request target is the whole PARSE class)
	Q_PROPERTY( QParseObject* parseObject MEMBER parseObject )
public:
	/*! constructor
	 *  \param parseClassName is the name of the Parse class for creating the endpoint on the underlying
	 *         REST API request.
	 *  \note Some has special meaning for Parse and they will be handled by QParse:
	 *        like _Users, login, logout
	 *        but you don't worry about that because they are related to special classes
	 *        which implementation is already provided by QParse library
	 */
	QParseRequest( QString parseClassName );
	//! return the class name used on PARSE for the target of this request
	QString getParseClassName() const;

	QParseObject *getParseObject() const;
	void setParseObject(QParseObject *value);

	/*! add the option and its value to the request
	 *  \param name is the name of option (like 'include', 'where', etc)
	 *  \param value is the value of the option to send
	 *  \warning it does not check for duplications
	 */
	void addOption( QString name, QString value );
	//! return the list of all options added so far
	QList< QPair<QString,QString> > getOptions();
private:
	//! this is used to create the correct endpoint to network request
	QString parseClassName;
	//! the target object of this request
	QParseObject* parseObject;
	//! these are used for get network requests
	QList< QPair<QString,QString> > params;
};

#endif // QPARSEREQUEST
