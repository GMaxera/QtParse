#ifndef QPARSEREQUEST_H
#define QPARSEREQUEST_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QString>
#include "qparsetypes.h"

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
	//! the Parse File object of this request (if set and valid)
	Q_PROPERTY( QParseFile* parseFile MEMBER parseFile )
	//! the local file to store the downloaded file in case the request was a Parse file download
	Q_PROPERTY( QString localFilename MEMBER localFilename )
public:
	/*! constructor
	 *  \param parseClassName is the name of the Parse class used for creating the endpoint on the underlying
	 *         REST API request.
	 *  \note Some has special meaning for Parse and they will be handled by QParse:
	 *        like _Users, login, logout
	 *        but you don't worry about that because they are related to special classes
	 *        which implementation is already provided by QParse library
	 *  \warning it's responsability to the creator of QParseRequest to destroy it, but NEVER delete it explicity.
	 *			 Instead, you MUST call the deleteLater() method on the associated QParseReply.
	 */
	QParseRequest( QString parseClassName );
	/*! constructor a Parse File request
	 *  \param QParseFile is the target of the request operation
	 *  \warning it's responsability to the creator of QParseRequest to destroy it, but NEVER delete it explicity.
	 *			 Instead, you MUST call the deleteLater() method on the associated QParseReply.
	 */
	QParseRequest( QParseFile* parseFile );

	//! return the class name used on PARSE for the target of this request
	QString getParseClassName() const;

	QParseObject *getParseObject() const;
	void setParseObject(QParseObject *value);

	QParseFile* getParseFile() const;
	void setParseFile(QParseFile* value);

	QString getLocalFilename() const;
	void setLocalFilename(const QString &value);

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
	//! the parse file object of this request (if any)
	QParseFile* parseFile;
	//! local Filename where to save the downloaded file from PARSE
	QString localFilename;
	//! these are used for get network requests
	QList< QPair<QString,QString> > params;
};

#endif // QPARSEREQUEST
