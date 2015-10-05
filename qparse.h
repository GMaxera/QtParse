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
#include <QVariantMap>

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
	//! this allow to bind the logged user to QML properties
	Q_PROPERTY( QParseUser* me READ getMe NOTIFY meChanged )
public:
	//! PARSE Keys
	static const QString appId;
	static const QString restKey;
	//! return the singleton instance of CloudInterface
	static QParse* instance();
public slots:
	//! Perform a get request on PARSE
	QParseReply* get( QParseRequest* request );
	//! Perform a post request on PARSE
	QParseReply* post( QParseRequest* request );
	//! Perform a put request on PARSE
	QParseReply* put( QParseRequest* request );


	/*! create a new user
	 *  \param username the username used for login
	 *  \param password to validate the user on login
	 *  \param properties a map of field => value of additional properties to set on User object
	 */
	void createUser( QString username, QString password, QMap<QString,QString> properties );
	//! log in using the passed credentials; signaled by userLoggedIn
	void loginAs( QString username, QString password );
	//! perform an auto login (if possible)
	void autoLogin();
	//! login/signup using the facebook auth data
	void linkToFacebook( QString facebookId, QString accessToken, QString expireDate, QMap<QString,QString> properties );
	//! log out; the effective log out is signaled by userLoggedOut
	void logOut();
	//! return the logged user; a NULL pointer means no user is logged in
	UserObject* getMe();
	//! return the user with specified ID
	UserObject* getUser( QString objectId );
	/*! upload the current data of userObject to the PARSE cloud
	 *
	 *  SHOULD BE A GENERIC WAY FOR DOWNLOAD / UPLOAD A Single object from PARSE
	 *
	 */
	void uploadUserData(UserObject* userObject , QStringList properties);
signals:
	void meChanged( UserObject* user );
	//! emitted when the user has been logged in
	void userLoggedIn();
	//! emitted when the user has been logged out
	void userLoggedOut();
	//! emitted when the log in process fails
	void userLoggingError( QString errorMessage );
	//! emitted when an attempt to create a user fails
	void userCreationFailed( QString errorMessage );
	//! emitted when an attempt to create a user successed
	void userCreationDone();
private slots:
	//! it manage the returned data from the cloud backed
	void onRequestFinished( QNetworkReply* reply );
private:
	// private constructor; this is a singleton
	QParse(QObject *parent = 0);
	Q_DISABLE_COPY( QParse )

	//! save the information about the last logged user into settings
	void saveLoggedUser();

	/*! process a queued QParseRequest and create the corresponding
	 *  QNetworkRequest to send over internet for the reply to PARSE
	 */
	void processOperationsQueue();

	/*! execute a POST request on PARSE
	 *  \param endPoint is the url of the REST API of PARSE after https://api.parse.com/1/
	 *  \param operation is the JSON encoded data or query to execute on PARSE
	 */
	QNetworkReply* post( QString endPoint, QJsonObject operation );
	/*! execute a GET request on PARSE
	 *  \param endPoint is the url of the REST API of PARSE after https://api.parse.com/1/
	 *  \param operation is the list of key=value pairs to pass as GET parameters
	 */
	QNetworkReply* get( QString endPoint, QList< QPair<QString,QString> > operation=QList< QPair<QString,QString> >() );
	/*! upload a file on PARSE
	 *  \param filename is the full path of the file to upload
	 */
	QNetworkReply* post( QString filename );
	/*! upload a file on PARSE
	 *  \param filename is the URL of the file to upload (has to be a local filename)
	 */
	QNetworkReply* post(QUrl url );
	/*! download a file from PARSE */
	QNetworkReply* download( QUrl fileurl );
	/*! execute a PUT request on PARSE
	 *  \param endPoint is the url of the REST API of PARSE after https://api.parse.com/1/
	 *  \param operation is the JSON encoded data or query to execute on PARSE
	 */
	QNetworkReply* put( QString endPoint, QJsonObject operation );
	/*! execute a DELETE on PARSE */
	QNetworkReply* deleteObject( QString className, QString objectId );

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

};

#endif // QPARSE_H
