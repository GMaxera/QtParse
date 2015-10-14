#ifndef QPARSEREPLY_H
#define QPARSEREPLY_H

#include <QObject>
#include <QJsonObject>

class QParseRequest;
class QParse;

/*! This object is returned by QParse when a QParseRequest has been submitted.
 *
 *  Bind the signal finished to the proper slot delegate to handle the data returned
 *  by PARSE
 *
 *  \warning Never create by yourself
 */
class QParseReply : public QObject {
	Q_OBJECT
	//! The request corresponding to this reply
	Q_PROPERTY( QParseRequest* request MEMBER request CONSTANT )
	/*! If it's true, then the returned data is a JSON object
	 *  \warning it can be an error, because in case of error Parse.com return a JSON message
	 */
	Q_PROPERTY( bool isJson MEMBER isJson )
	//! the Json object containing the reply in case of isJson is true
	Q_PROPERTY( QJsonObject json MEMBER json )
	//! if true means that an error occurred during the request on PARSE
	Q_PROPERTY( bool hasError MEMBER hasError )
	//! the error message, if any
	Q_PROPERTY( QString errorMessage MEMBER errorMessage )
	//! the error code, if any
	Q_PROPERTY( int errorCode MEMBER errorCode )
public:
	/*! Constructor
	 *  the parent is always QParse singleton instance because
	 *  this object is created by QParse singleton
	 */
	QParseReply( QParseRequest* request, QParse* parent );

	bool getIsJson() const;
	void setIsJson(bool value);

	QJsonObject getJson() const;
	void setJson(const QJsonObject &value);

	bool getHasError() const;
	void setHasError(bool value);

	QString getErrorMessage() const;
	void setErrorMessage(const QString &value);

	int getErrorCode() const;
	void setErrorCode(int value);

signals:
	//! emitted when the reply has been arrived and prepared to be processed
	void finished( QParseReply* reply);
private:
	//! the request
	QParseRequest* request;
	//! true means the data is a JSON object
	bool isJson;
	//! the Json object containing the reply
	QJsonObject json;
	//! true means there was an error during the request
	bool hasError;
	//! the error message, if any
	QString errorMessage;
	//! the error code, if any
	int errorCode;
};

#endif // QPARSEREPLY
