#ifndef QPARSETYPES_H
#define QPARSETYPES_H

#include <QJsonObject>
#include <QDateTime>
#include <QUrl>
#include <QMetaType>
#include <QObject>

class QParseReply;

//! \file This file contains the classes for handle easily the PARSE data types

//! The Date type on PARSE
class QParseDate {
	Q_GADGET
	Q_PROPERTY( QDateTime dateTime READ toDateTime CONSTANT )
public:
	//! default constructor
	QParseDate();
	//! copy-constructor
	QParseDate( const QParseDate& src );
	//! construct a QParseDate from the Json PARSE representation
	QParseDate( QJsonObject fromParse );
	/*! construct a QParseDate from ISO Date representation
	 *  This is mainly used for createdAt and updatedAt default PARSE properties
	 *  that are returned directly as string and not as Date type
	 */
	QParseDate( QString isoParse );
	//! construct a QParseDate from a QDateTime object
	QParseDate( QDateTime fromDateTime );
	bool operator==( QParseDate const& other ) const;
	bool operator!=( QParseDate const& other ) const;
	//! export into Json PARSE representation
	QJsonObject toJson() const;
	//! export into QDateTime
	QDateTime toDateTime() const;
	//! return the ISO representation of the date
	QString toISO();
private:
	//! Json representation for PARSE
	QJsonObject json;
	//! QDateTime representation
	QDateTime dateTime;
};
Q_DECLARE_METATYPE( QParseDate )

//! The File type on PARSE
class QParseFile : public QObject {
	Q_OBJECT
	Q_PROPERTY( QUrl url READ getUrl CONSTANT )
	Q_PROPERTY( QString name READ getName CONSTANT )
	Q_PROPERTY( QUrl localUrl MEMBER localUrl NOTIFY localUrlChanged )
	Q_PROPERTY( Status status MEMBER status NOTIFY statusChanged )
public:
	enum Status { NotValid, ToUpload, NotCached, Caching, Cached };
	Q_ENUM( Status )
	//! default constructor
	QParseFile( QObject* parent=0 );
	//! copy-constructor
	QParseFile( const QParseFile& src, QObject* parent=0 );
	//! construct a QParseFile from the Json PARSE representation
	QParseFile( QJsonObject fromParse, QObject* parent=0 );
	//! construct a QParseFile from a local file
	QParseFile( QUrl localFile, QObject* parent=0 );
public slots:
	/*! the URL where the file is currently stored
	 *
	 *  If the URL points to a local file it means
	 *  is not uploaded to PARSE yet
	 *
	 *  If the URL is remote, then it means the file
	 *  has been uploaded to PARSE
	 */
	QUrl getUrl() const;
	/*! the name of the file on PARSE
	 *
	 *  If the file has not been uploaded to PARSE yet,
	 *  this name will be empty
	 */
	QString getName() const;
	//! export into Json PARSE representation
	QJsonObject toJson();
	//! return true if the data represent a valid local file or a valid remote PARSE file
	bool isValid();

	QUrl getLocalUrl() const;
	void setLocalUrl(const QUrl &value);

	Status getStatus() const;
	void setStatus(const Status &value);

	/*! send a request for caching the file */
	void pull();
signals:
	void localUrlChanged( QUrl localUrl );
	void statusChanged( Status status );
	void cached( QUrl localFile );
private:
	//! Json representation for PARSE
	QJsonObject json;
	/*! the URL where the file is currently stored
	 *
	 *  If the URL points to a local file it means
	 *  is not uploaded to PARSE yet
	 *
	 *  If the URL is remote, then it means the file
	 *  has been uploaded to PARSE
	 */
	QUrl url;
	/*! the name of the file on PARSE
	 *
	 *  If the file has not been uploaded to PARSE yet,
	 *  this name will be empty
	 */
	QString name;
	/*! The local Url is where the file has been cached
	 *  It's only valid where status == Cached
	 */
	QUrl localUrl;
	/*! the status of this QParseFile
	 *  NotValid -> Not valid
	 *  ToUpload -> is a local file not uploaded yet to PARSE
	 *  NotCached -> is a Parse file not downloaded yet
	 *  Cached -> is a Parse file cached locally
	 */
	Status status;
};

#endif // QPARSETYPES_H
