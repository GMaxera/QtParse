#ifndef QPARSE_H
#define QPARSE_H

#include <QObject>
#include <QMap>
#include <QList>
#include <QPair>
#include <QStringList>
#include <QMimeDatabase>
#include <QDateTime>
#include <QUrl>
#include <QQueue>
#include <QJsonObject>
#include <QVariantMap>
#include <QTimer>

class QNetworkAccessManager;
class QNetworkRequest;
class QNetworkReply;
class OperationData;
class QFile;
class QParseRequest;
class QParseReply;
class QParseObject;
class QParseUser;

/*! This class creates allow to connect to PARSE cloud and
 *  perform operations on it as query, storing and retriveing data
 *
 *  This object is a singleton
 *
 *  Now, it use REST API of PARSE
 */
class QParse : public QObject {
	Q_OBJECT
	//! PARSE Application ID
	Q_PROPERTY( QString appId MEMBER appId NOTIFY appIdChanged )
	//! PARSE REST API Key (use the client key, not the master)
	Q_PROPERTY( QString restKey MEMBER restKey NOTIFY restKeyChanged )
	//! this allow to bind the logged user to QML properties
	Q_PROPERTY( QParseUser* me READ getMe NOTIFY meChanged )
public:
	//! return the singleton instance of CloudInterface
	static QParse* instance();
public slots:
	QString getAppId() const;
	void setAppId(const QString &value);
	QString getRestKey() const;
	void setRestKey(const QString &value);
	//! return the logged user; a NULL pointer means no user is logged in
	QParseUser* getMe();
	//! Perform a get request on PARSE
	QParseReply* get( QParseRequest* request );
	//! Perform a post request on PARSE
	QParseReply* post( QParseRequest* request );
	//! Perform a put request on PARSE
	QParseReply* put( QParseRequest* request );
signals:
	void appIdChanged( QString appId );
	void restKeyChanged( QString restKey );
	void meChanged( QParseUser* user );
private slots:
	//! it manage the returned data from the cloud backed
	void onRequestFinished( QNetworkReply* reply );
private:
	// private constructor; this is a singleton
	QParse(QObject *parent = 0);
	Q_DISABLE_COPY( QParse )

	/*! process a queued QParseRequest and create the corresponding
	 *  QNetworkRequest to send over internet for the reply to PARSE
	 */
	void processOperationsQueue();

	//! PARSE Keys
	QString appId;
	QString restKey;

	//! current user
	QParseUser* user;
	//! cached object
	QMap<QString, QParseObject*> cachedObject;

	/*! date of cache validity
	 * if the user has been downloaded before this date
	 * then a forceUpdate will be raised
	 */
	QDateTime cacheValidity;

	//! the newtork manager for sending requests to cloud backend
	QNetworkAccessManager* net;
	//! inner private class for storing data about operations on Parse
	class OperationData {
	public:
		OperationData()
			: parseRequest(NULL)
			, parseReply(NULL)
			, netRequest(NULL)
			, netMethod(GET)
			, dataToPost()
			, fileToPost(NULL)
			, mimeDb() { }
		QParseRequest* parseRequest;
		QParseReply* parseReply;
		QNetworkRequest* netRequest;
		enum NetMethod { GET, PUT, POST, DELETE, GET_FILE, POST_FILE };
		NetMethod netMethod;
		QJsonObject dataToPost;
		QFile* fileToPost;
		QMimeDatabase mimeDb;
	};
	/*! the queue of the operation to process
	 *  A parseRequest will be put in this queue and as soon as possible
	 *  will be processed and created a netRequest to send over internet
	 */
	QQueue<OperationData*> operationsQueue;
	//! the map of operation sent to Parse waiting for a netReply to process
	QMap<QNetworkReply*, OperationData*> operationsPending;

	//! Timer for triggering the execution of processOperationsQueue()
	QTimer* timer;
};

#endif // QPARSE_H
