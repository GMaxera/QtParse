
#include "qparse.h"
#include "qparsetypes.h"
#include "qparseobject.h"
#include "qparseuser.h"
#include "qparserequest.h"
#include "qparsereply.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUrlQuery>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QTimer>
#include <QDir>
#include <QSettings>
#include <QtQml>

QParse::QParse(QObject *parent)
	: QObject(parent) {
	// register metatypes
	qRegisterMetaType<QParseDate>("QParseDate");
	//qRegisterMetaType<QParseFile>("QParseFile");
	qmlRegisterType<QParseFile>("org.gmaxera.qparse", 1, 0, "ParseFile");
	// initialize the singleton
	cacheDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+"/QParseCache";
	QDir dir(cacheDir);
	dir.mkpath(cacheDir);
	cacheIni = "cache.ini";
	loadCacheInfoData();
	user = NULL;
	net = new QNetworkAccessManager(this);
	connect( net, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRequestFinished(QNetworkReply*)) );
	// set the timer for processing the queue
	timer = new QTimer(this);
	timer->setInterval(0);
	timer->setSingleShot(false);
	connect( timer, &QTimer::timeout, this, &QParse::processOperationsQueue );
	timer->start();
}

QParse* QParse::instance() {
	static QParse* singleton = new QParse();
	return singleton;
}

QString QParse::getRestKey() const {
	return restKey;
}

void QParse::setRestKey(const QString &value) {
	restKey = value;
}

QString QParse::getAppId() const {
	return appId;
}

void QParse::setAppId(const QString &value) {
	appId = value;
}

QString QParse::getGCMSenderId() {
	return gcmSenderId;
}

void QParse::setGCMSenderId(const QString& value) {
	gcmSenderId = value;
}

QJsonValue QParse::getAppConfigValue( QString key ) {
	return appConfig[key];
}

void QParse::updateAppConfigValues() {
	if ( isRequestCached(QUrl("https://api.parse.com/1/config")) ) {
		QJsonObject data = getCachedJson( QUrl("https://api.parse.com/1/config") );
		if ( data.contains("params") ) {
			appConfig = data["params"].toObject();
			emit appConfigChanged();
		}
	} else {
		// direct request, the reply will be handled on onRequestFinished
		QNetworkRequest request(QUrl("https://api.parse.com/1/config"));
		request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
		request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
		if ( user ) {
			// if there is a user logged in, send also the session token
			request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
		}
		net->get(request);
	}
}

QJsonValue QParse::getInstallationValue( QString key ) {
	return installation[key];
}

QParseUser* QParse::getMe() {
	return user;
}

QParseReply* QParse::get( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	if ( request->getParseFile() && request->getParseFile()->isValid() ) {
		reply->setIsJson( false );
	} else {
		reply->setIsJson( true );
	}
	// reparent QParseRequest to be destroyed with QParseReply later
	request->setParent(reply);
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = reply;
	data->netMethod = QParse::OperationData::GET;
	operationsQueue.enqueue( data );
	return reply;
}

QParseReply* QParse::post( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	reply->setIsJson( true );
	// reparent QParseRequest to be destroyed with QParseReply later
	request->setParent(reply);
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = reply;
	data->netMethod = QParse::OperationData::POST;
	data->dataToPost = request->getParseObject()->toJson();
	operationsQueue.enqueue( data );
	return reply;
}

QParseReply* QParse::put( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	reply->setIsJson( true );
	// reparent QParseRequest to be destroyed with QParseReply later
	request->setParent(reply);
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = reply;
	data->netMethod = QParse::OperationData::PUT;
	data->dataToPost = request->getParseObject()->toJson();
	operationsQueue.enqueue( data );
	return reply;
}

void QParse::onRequestFinished(QNetworkReply *reply) {
	qDebug() << "REPLY FROM PARSE" << reply->url();
	if ( !operationsPending.contains(reply) ) {
		// check the special case of PARSE App Config
		if ( reply->url() == QUrl("https://api.parse.com/1/config") ) {
			if ( reply->error() == QNetworkReply::NoError ) {
				updateCache( reply, NULL );
				QJsonObject data = getCachedJson( reply->url() );
				if ( data.contains("params") ) {
					appConfig = data["params"].toObject();
					emit appConfigChanged();
				}
			}
		// check the special case of PARSE Installation
		} else if ( reply->url() == QUrl("https://api.parse.com/1/installations") ) {
			if ( reply->error() == QNetworkReply::NoError ) {
				//QSettings cacheSets( cacheDir+"/"+cacheIni, QSettings::IniFormat, this );
				//cacheSets.setIniCodec("UTF-8");
				QJsonObject data = QJsonDocument::fromJson( reply->readAll() ).object();
				foreach( QString key, data.keys() ) {
					installation[key] = data[key];
				}
				qDebug() << "INSTALLATION REPLY" << installation;
			}
		} else {
			qDebug() << "QParse Error - Not reply into operations map !!";
		}
		return;
	}
	OperationData* opdata = operationsPending.take(reply);
	// check for any errors
	if ( reply->error() != QNetworkReply::NoError ) {
		opdata->parseReply->setHasError( true );
		QJsonObject data = QJsonDocument::fromJson( reply->readAll() ).object();
		if ( data.contains("error") ) {
			opdata->parseReply->setErrorMessage( data["error"].toString() );
			opdata->parseReply->setErrorCode( data["code"].toInt() );
			qDebug() << "PARSE ERROR" << data;
		} else {
			opdata->parseReply->setErrorMessage( reply->errorString() );
			opdata->parseReply->setErrorCode( reply->error() );
			qDebug() << "NETWORK ERROR" << reply->errorString();
		}
		// emit the signal and terminates
		emit (opdata->parseReply->finished(opdata->parseReply));
		return;
	}
	// cache the reply, and prepare QParseReply
	updateCache( reply, opdata );
	fillWithCachedData( opdata->netRequest->url(), opdata->parseReply );
	emit (opdata->parseReply->finished(opdata->parseReply));
	return;
}

void QParse::processOperationsQueue() {
	if ( operationsQueue.isEmpty() ) return;
	OperationData* data = operationsQueue.dequeue();
	// create the endpoint
	QUrl endpoint;
	QString urlPrefix = "https://api.parse.com/1";
	if ( data->parseRequest->getParseFile() && data->parseRequest->getParseFile()->isValid() ) {
		// FILE ENDPOINT CREATION
		if ( data->netMethod == QParse::OperationData::GET ) {
			endpoint = data->parseRequest->getParseFile()->getUrl();
		} else {
			endpoint = QUrl( QString("%1/files/%2")
								.arg(urlPrefix)
								.arg(data->parseRequest->getParseFile()->getName()) );
		}
	} else {
		// NO FILE ENDPOINT CREATION
		QString parseClassName = data->parseRequest->getParseClassName();
		QParseObject* parseObject = data->parseRequest->getParseObject();
		if ( parseClassName == "_Users" ) {
			endpoint = QUrl( QString("%1/users/%2")
								.arg( urlPrefix )
								.arg( parseObject ? parseObject->getObjectId() : "" ) );
		} else if (parseClassName == "login") {
			endpoint = QUrl( QString("%1/login")
								.arg( urlPrefix ) );
		} else {
			endpoint = QUrl( QString("%1/classes/%2/%3")
								.arg( urlPrefix )
								.arg( parseClassName )
								.arg( parseObject ? parseObject->getObjectId() : "" ) );
		}
		QList< QPair<QString,QString> > options = data->parseRequest->getOptions();
		if ( options.size() > 0 ) {
			QUrlQuery query;
			query.setQueryItems( options );
			endpoint.setQuery( query );
		}
	}
	// it only perform a network request if there is no cached data (or if it's invalid)
	if ( data->parseRequest->getCacheControl() == QParse::AlwaysCache && isRequestCached(endpoint) ) {
		// automatically reply with cached data
		fillWithCachedData( endpoint, data->parseReply );
		emit (data->parseReply->finished(data->parseReply));
	} else {
		// create the netRequest
		QNetworkRequest* request = new QNetworkRequest(endpoint);
		request->setRawHeader("X-Parse-Application-Id", appId.toLatin1());
		request->setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
		if ( user ) {
			// if there is a user logged in, send also the session token
			request->setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
		}
		data->netRequest = request;
		// send the net request to PARSE
		QJsonDocument jsonDoc(data->dataToPost);
		switch(data->netMethod) {
		case QParse::OperationData::GET:
			operationsPending[net->get( *request )] = data;
		break;
		case QParse::OperationData::POST:
			request->setRawHeader("Content-Type", "application/json");
			operationsPending[net->post( *request, jsonDoc.toJson(QJsonDocument::Compact) )] = data;
		break;
		case QParse::OperationData::PUT:
			request->setRawHeader("Content-Type", "application/json");
			operationsPending[net->post( *request, jsonDoc.toJson(QJsonDocument::Compact) )] = data;
		break;
		}
	}
	return;
}

void QParse::loadCacheInfoData() {
	QSettings cacheSets( cacheDir+"/"+cacheIni, QSettings::IniFormat, this );
	cacheSets.setIniCodec("UTF-8");
	int size = cacheSets.beginReadArray("caches");
	for( int i=0; i<size; i++ ) {
		cacheSets.setArrayIndex(i);
		QUrl url = cacheSets.value("url").toUrl();
		CacheData cacheData;
		cacheData.isJson = cacheSets.value("isJson").toBool();
		cacheData.localFile = QUrl::fromLocalFile( cacheDir+"/"+cacheSets.value("localFile").toString() );
		cacheData.createdAt = cacheSets.value("createdAt").toDateTime();
		cache[url] = cacheData;
	}
	cacheSets.endArray();
}

void QParse::updateCache( QNetworkReply* reply, QParse::OperationData* opdata ) {
	CacheData cacheData;
	// !! opdata is NULL when QParse call this method for caching Parse App config
	if ( !opdata ) {
		cacheData.isJson = true;
	} else {
		cacheData.isJson = opdata->parseReply->getIsJson();
	}
	cacheData.createdAt = QDateTime::currentDateTime();
	QString cacheFilename;
	if ( cacheData.isJson ) {
		cacheFilename = getUniqueCacheFilename();
	} else {
		cacheFilename = cacheDir+"/"+opdata->parseRequest->getParseFile()->getName();
	}
	QFile cacheFile( cacheFilename );
	if ( !cacheFile.open( QIODevice::WriteOnly | QIODevice::Truncate ) ) {
		qDebug() << "Destination" << cacheFilename;
		qFatal( "Cannot write on cache directory !!" );
	}
	cacheFile.write( reply->readAll() );
	cacheFile.flush();
	cacheFile.close();
	cacheData.localFile = QUrl::fromLocalFile( cacheFilename );
	cache[reply->url()] = cacheData;
	// write down on settings on disk
	QSettings cacheSets( cacheDir+"/"+cacheIni, QSettings::IniFormat, this );
	cacheSets.setIniCodec("UTF-8");
	int size = cacheSets.beginReadArray("caches");
	cacheSets.endArray();
	cacheSets.beginWriteArray("caches");
	cacheSets.setArrayIndex(size);
	cacheSets.setValue("url", reply->url());
	cacheSets.setValue("isJson", cacheData.isJson);
	cacheSets.setValue("localFile", cacheData.localFile.fileName());
	cacheSets.setValue("createdAt", cacheData.createdAt);
	cacheSets.endArray();
}

bool QParse::isRequestCached( QUrl url ) {
	return cache.contains(url);
}

void QParse::fillWithCachedData( QUrl url, QParseReply* reply ) {
	CacheData cacheData = cache[url];
	if ( reply->getIsJson() ) {
		QFile cacheFile( cacheData.localFile.toLocalFile() );
		if ( !cacheFile.open( QIODevice::ReadOnly ) ) {
			qFatal( "Cannot read on cache directory !!");
		}
		QJsonObject json = QJsonDocument::fromJson( cacheFile.readAll() ).object();
		cacheFile.close();
		reply->setJson( json );
	} else {
		reply->setLocalUrl( cacheData.localFile );
	}
}

QJsonObject QParse::getCachedJson( QUrl url ) {
	CacheData cacheData = cache[url];
	QFile cacheFile( cacheData.localFile.toLocalFile() );
	if ( !cacheFile.open( QIODevice::ReadOnly ) ) {
		qDebug() << "LOCATION: " << cacheData.localFile << cacheData.localFile.toLocalFile() << cacheDir;
		qFatal( "Cannot read on cache directory !!");
	}
	QJsonObject json = QJsonDocument::fromJson( cacheFile.readAll() ).object();
	cacheFile.close();
	return json;
}

QString QParse::getUniqueCacheFilename() {
	int lenght = 8;
	while( true ) {
		QString str = cacheDir+"/";
		for( int i=0; i<lenght; i++ ) {
			str.append( QChar(97+qrand()%25) );
		}
		str.append(".json");
		if ( !QFile::exists(str) ) {
			return str;
		}
	}
}
