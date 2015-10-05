
#include "qparse.h"
#include "qparseobject.h"
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

QParse::QParse(QObject *parent)
	: QObject(parent) {
	user = NULL;
	net = new QNetworkAccessManager(this);
	// SET UP CACHE ON DISK TO HAVE PERSISTENT CACHING OF DOWNLOADED FILES
	connect( net, SIGNAL(finished(QNetworkReply*)), this, SLOT(onRequestFinished(QNetworkReply*)) );
	cacheValidity = QDateTime::currentDateTime();
}

QParse* QParse::instance() {
	static QParse* singleton = new QParse();
	return singleton;
}

QParseReply* QParse::get( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	reply->setIsJSON( true );
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = parseReply;
	data->netMethod = QParse::OperationData::GET;
	operationsQueue.enqueue( data );
	return reply;
}

QParseReply* QParse::post( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	reply->setIsJSON( true );
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = parseReply;
	data->netMethod = QParse::OperationData::POST;
	data->dataToPost = request->getParseObject()->toJson();
	operationsQueue.enqueue( data );
	return reply;
}

QParseReply* QParse::put( QParseRequest* request ) {
	QParseReply* reply = new QParseReply(request, this);
	reply->setIsJSON( true );
	OperationData* data = new OperationData();
	data->parseRequest = request;
	data->parseReply = parseReply;
	data->netMethod = QParse::OperationData::PUT;
	data->dataToPost = request->getParseObject()->toJson();
	operationsQueue.enqueue( data );
	return reply;
}

void QParse::processOperationsQueue() {
	if ( operationsQueue.isEmpty() ) return;
	OperationData* data = operationsQueue.dequeue();
	// create the endpoint
	QUrl endpoint;
	QString parseClassName = data->parseRequest->getParseClassName();
	QParseObject* parseObject = data->parseRequest->getParseObject();
	QString urlPrefix = "https://api.parse.com/1/";
	if ( parseClassName == "_Users" ) {
		endpoint = QUrl( QString("%1/users/%2")
							.arg( urlPrefix )
							.arg( parseObject ? parseObject->getId() : "" ) );
	} else if (parseClassName == "login") {
		endpoint = QUrl( QString("%1/login")
							.arg( urlPrefix ) );
	} else {
		endpoint = QUrl( QString("%1/classes/%2/%3")
							.arg( urlPrefix )
							.arg( parseClassName )
							.arg( parseObject ? parseObject->getId() : "" ) );
	}
	QList< QPair<QString,QString> > options = data->parseRequest->getOptions();
	if ( options.size() > 0 ) {
		QUrlQuery query;
		query.setQueryItems( options );
		endpoint.setQuery( query );
	}
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
	switch(data->netMethod) {
	case QParse::OperationData::GET:
		operationsPending[net->get( *request )] = data;
	case QParse::OperationData::POST:
		operationsPending[net->post( *request, data->dataToPost )] = data;
	break;
	case QParse::OperationData::PUT:
		operationsPending[net->post( *request, data->dataToPost )] = data;
	break;
	}
	return;
}

void QParse::createUser( QString username, QString password, QMap<QString,QString> properties ) {
	QJsonObject userData;
	userData["username"] = username.trimmed();
	userData["password"] = password;
	foreach( QString key, properties.keys() ) {
		userData[key] = properties[key];
	}
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::CreateUser;
	opdata->isJSON = true;
	operations[post("users", userData)] = opdata;
}

void QParse::loginAs( QString username, QString password ) {
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("username"), username.trimmed()) );
	params.append( qMakePair(QString("password"), password) );
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Login;
	opdata->isJSON = true;
	operations[get("login", params)] = opdata;
}

void CloudInterface::autoLogin() {
	// auto login
	QSettings settings;
	if ( settings.childGroups().contains("user") ) {
		settings.beginGroup("user");
		user = new UserObject( settings.value("objectId").toString(), settings.value("email").toString(),
					settings.value("sessionToken").toString(), settings.value("facebookId").toString() );
		settings.endGroup();
		cachedUsers[user->getId()] = user;
		if ( !user->getFacebookId().isEmpty() ) {
			QFacebook::instance()->autoLogin();
		}
		qDebug() << "RESTORING PREVIOUS USER" << user->getId();
		emit meChanged(user);
		emit userLoggedIn();
		downloadNotificationList();
	}
}

void CloudInterface::linkToFacebook( QString facebookId, QString displayName, QString email, QString accessToken, QString expireDate) {
	QJsonObject authData;
	QJsonObject fbAuthData;
	QJsonObject facebook;
	facebook["id"] = facebookId;
	facebook["access_token"] = accessToken;
	facebook["expiration_date"] = expireDate;
	fbAuthData["facebook"] = facebook;
	authData["authData"] = fbAuthData;
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::LinkFacebookUser;
	opdata->isJSON = true;
	opdata->phase = 0;
	opdata->displayName = displayName;
	opdata->email = email;
	operations[post("users", authData)] = opdata;
}

void CloudInterface::logOut() {
	if ( user == NULL ) {
		// alreay logged out
		return;
	}
	// nothing to send on PARSE for logging out
	// clearing the notification List
	notificationList.clear();
	emit notificationListChanged(notificationList);
	updateUnreadNotifications();
	user = NULL;
	emit meChanged(user);
	emit userLoggedOut();
	QSettings settings;
	settings.remove("user");
}

UserObject* CloudInterface::getMe() {
	if ( user ) {
		user->updateIfOld( cacheValidity );
	}
	return user;
}

UserObject* CloudInterface::getUser(QString objectId) {
	if ( objectId.isEmpty() ) {
		return new UserObject();
	}
	if ( cachedUsers.contains(objectId) ) {
		cachedUsers[objectId]->updateIfOld( cacheValidity );
		return cachedUsers[objectId];
	} else {
		// create the user and dowload the data
		UserObject* cuser = new UserObject(objectId);
		cachedUsers[objectId] = cuser;
		return cuser;
	}
}

void CloudInterface::postTape(QString title, QStringList tags, QString parentTape, bool isCrossing, QString famousTape, bool isPrivate, QString toUserId) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::PostTape;
	opdata->tapeTitle = title;
	opdata->tapeTags = tags;
	//qDebug() << "TAGS:" << opdata->tapeTags;
	opdata->tapeDuration = Backend::instance()->getLastRecordedTime();
	opdata->tapeParent = parentTape;
	opdata->tapeIsCrossing = isCrossing;
	opdata->tapeFamous = famousTape;
	opdata->tapeIsPrivate = isPrivate;
	opdata->tapeToUser = toUserId;

	if ( isCrossing ) {
		// in this case the first phase is to download the parent tape audio
		opdata->phase = 4;
		opdata->isJSON = false;
		QUrl fileurl = cachedTapes[parentTape]->getAudio();
		operations[download(fileurl)] = opdata;
	} else if ( !famousTape.isEmpty() ) {
		// it use a famous tape has data, it's not necessary to upload audio file and image
		// it's only necessary to upload the social image file
		opdata->phase = 2;
		opdata->isJSON = true;
		TapeObject* famous = cachedTapes[famousTape];
		opdata->tapeAudio = famous->getAudio().fileName();
		opdata->tapeDuration = famous->getDuration();
		opdata->tapeImage = famous->getImage().fileName();
		operations[post(Backend::instance()->lastTapeImageSocial())] = opdata;
	} else {
		opdata->phase = 0;
		opdata->isJSON = true;
		operations[post(Backend::instance()->lastAudioFile())] = opdata;
	}
}

void CloudInterface::downloadWallList() {
	if ( !user ) {
		// called when a user is not logged in
		// this should never happens
		return;
	}
	//wallList.clear();
	//emit wallListChanged( wallList );
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadWallList;
	opdata->isJSON = true;
	QJsonObject where;
	QJsonObject notExists;
	notExists["$exists"] = false;
	where["parent"] = notExists;
	// create the array containing all pointers to visible users
	QStringList users;
	users << user->getId() << user->getFriends() << defaultChannel;
	QJsonArray usersArray;
	foreach( QString aUser, users ) {
		QJsonObject userPointer;
		userPointer["__type"] = QString::fromUtf8("Pointer");
		userPointer["className"] = QString::fromUtf8("_User");
		userPointer["objectId"] = aUser;
		usersArray.append( userPointer );
	}
	QJsonObject inArray;
	inArray["$in"] = usersArray;
	where["user"] = inArray;
	QJsonDocument jsonDoc(where);
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
	params.append( qMakePair(QString("order"), QString(wallOrderBy)) );
	params.append( qMakePair(QString("limit"), QString::number(1000)) );
	// it is not necessary to download all data here,
	// the tape data will be downloaded on demand after
	// so, we restrict to just one key because zero is not possible
	params.append( qMakePair(QString("keys"), QString("listenCount")) );
	operations[get("/classes/QTapes", params)] = opdata;
}

void CloudInterface::setWallListOrderBy( QString orderBy ) {
	wallOrderBy = orderBy;
	downloadWallList();
}

void CloudInterface::downloadUserData( UserObject* userObject ) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadUserData;
	opdata->isJSON = true;
	opdata->user = userObject;
	opdata->phase = 0;
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("include"), QString("friends")) );
	operations[get("/users/"+userObject->getId(), params)] = opdata;
}

void CloudInterface::downloadTapeData( TapeObject* tapeObject ) {
	// Check if it's a famous tape or not
	if ( famousList.contains(tapeObject->getId()) ) {
		// Download Famous tape data
		OperationData* opdata = new OperationData();
		opdata->type = OperationData::DownloadFamousData;
		opdata->isJSON = true;
		opdata->tape = tapeObject;
		operations[get(QString("/classes/QFamous/%1").arg(tapeObject->getId()))] = opdata;
	} else if ( tapeObject->isPrivate() ) {
		// It creates a query for downloading from QInbox the tape
		OperationData* opdata = new OperationData();
		opdata->type = OperationData::DownloadInboxTapeData;
		opdata->isJSON = true;
		opdata->tape = tapeObject;
		operations[get(QString("/classes/QInbox/%1").arg(tapeObject->getId()))] = opdata;
	} else {
		// It creates a query for downloading in one shot
		// the tape data and all related comments
		OperationData* opdata = new OperationData();
		opdata->type = OperationData::DownloadTapeData;
		opdata->isJSON = true;
		opdata->tape = tapeObject;
		QJsonObject idQuery;
		idQuery["objectId"] = tapeObject->getId();
		QJsonObject tapeRef;
		tapeRef["__type"] = QString::fromUtf8("Pointer");
		tapeRef["className"] = QString::fromUtf8("QTapes");
		tapeRef["objectId"] = tapeObject->getId();
		QJsonObject parentQuery;
		parentQuery["parent"] = tapeRef;
		QJsonArray queries;
		queries.append( idQuery );
		queries.append( parentQuery );
		QJsonObject orQueries;
		orQueries["$or"] = queries;
		QJsonDocument jsonDoc(orQueries);
		QList< QPair<QString,QString> > params;
		params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
		params.append( qMakePair(QString("order"), QString("-postedAt")) );
		operations[get("/classes/QTapes", params)] = opdata;
	}
}

void CloudInterface::downloadChannelList() {
	channelList.clear();
	emit channelListChanged( channelList );
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadChannelList;
	opdata->isJSON = true;
	QJsonObject query;
	query["visible"] = true;
	QJsonDocument jsonDoc(query);
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
	params.append( qMakePair(QString("order"), QString("channelNumber")) );
	params.append( qMakePair(QString("limit"), QString::number(1000)) );
	operations[get("/classes/Channels2", params)] = opdata;
}

void CloudInterface::uploadUserData(UserObject* userObject , QStringList properties) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::UploadUserData;
	if ( properties.contains("imageFile") ) {
		// The changing of image requires more commands
		opdata->phase = 1;
		opdata->user = user;
		opdata->userProperties = properties;
		opdata->isJSON = true;
		operations[post(userObject->getImageFile())] = opdata;
		return;
	}
	QJsonObject data;
	foreach( QString property, properties ) {
		data[property] = QJsonValue::fromVariant(userObject->property( property.toLatin1().data() ));
	}
	// prepare operation data
	opdata->phase = 0;
	opdata->isJSON = true;
	opdata->user = user;
	operations[put(QString("/users/")+userObject->getId(), data)] = opdata;
}

QStringList CloudInterface::getWallList() {
	return wallList;
}

QStringList CloudInterface::getChannelList() {
	return channelList;
}

bool CloudInterface::isChannel(QString userId) {
	return channelList.contains(userId);
}

QStringList CloudInterface::getNotificationList() {
	return notificationList;
}

void CloudInterface::setNotificationRead(NotificationObject* notify, bool hide) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::ReplyNotUsed;
	QJsonObject data;
	data["read"] = true;
	if ( hide ) {
		data["hide"] = true;
	}
	opdata->isJSON = true;
	operations[put(QString("/classes/Notifications")+notify->getId(), data)] = opdata;
	notify->setRead( true );
	if ( hide ) {
		notificationList.removeAll(notify->getId());
		emit notificationListChanged(notificationList);
	}
	updateUnreadNotifications();
}

int CloudInterface::getUnreadNotifications() {
	return unreadNotifications;
}

TapeObject* CloudInterface::getTape( QString id, bool isPrivate ) {
	if ( cachedTapes.contains(id) ) {
		cachedTapes[id]->updateIfOld( cacheValidity );
		return cachedTapes[id];
	} else {
		// create the tape and dowload the data
		TapeObject* tape = new TapeObject(id, true, isPrivate);
		cachedTapes[id] = tape;
		return tape;
	}
}

void CloudInterface::deleteTape(QString id) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DeleteTape;
	opdata->isJSON = true;
	if ( cachedTapes.contains(id) ) {
		delete (cachedTapes.take(id));
	}
	operations[deleteObject("QTapes", id)] = opdata;
}

void CloudInterface::sendFriendRequest( QString userId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( userId );
	QJsonObject actors;
	actors["userA"] = user->getId();
	actors["userB"] = userId;
	operations[post("functions/QSendFriendRequest", actors)] = opdata;
}

void CloudInterface::removeFriendRequest( QString userId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( userId );
	QJsonObject actors;
	actors["userA"] = user->getId();
	actors["userB"] = userId;
	operations[post("functions/QRemoveFriendRequest", actors)] = opdata;
}

void CloudInterface::acceptFriendRequest( QString userId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( userId );
	QJsonObject actors;
	actors["userA"] = userId;
	actors["userB"] = user->getId();
	operations[post("functions/QAcceptFriendRequest", actors)] = opdata;
}

void CloudInterface::refuseFriendRequest( QString userId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( userId );
	QJsonObject actors;
	actors["userA"] = userId;
	actors["userB"] = user->getId();
	operations[post("functions/QRefuseFriendRequest", actors)] = opdata;
}

void CloudInterface::removeFriendship( QString userId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( userId );
	QJsonObject actors;
	actors["userA"] = user->getId();
	actors["userB"] = userId;
	operations[post("functions/QRemoveFriendship", actors)] = opdata;
}

void CloudInterface::followChannel( QString channelId ) {
	cacheValidity = QDateTime::currentDateTime();
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::Friendship;
	opdata->isJSON = true;
	opdata->user = getUser( channelId );
	QJsonObject actors;
	actors["userA"] = user->getId();
	actors["userB"] = channelId;
	operations[post("functions/QFollowChannel", actors)] = opdata;
}

void CloudInterface::searchUsers( QString terms ) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::SearchUsers;
	opdata->isJSON = true;
	QJsonObject jterms;
	jterms["terms"] = terms;
	operations[post("functions/QSearchUsers", jterms)] = opdata;
}

void CloudInterface::searchTapes( QString terms ) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::SearchUsers;
	opdata->isJSON = true;
	QJsonObject jterms;
	jterms["terms"] = terms;
	operations[post("functions/QSearchTapes", jterms)] = opdata;
}

void CloudInterface::cacheFile(QObject *refObject, QUrl url, QString dir) {
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::CacheFile;
	opdata->isJSON = false;
	opdata->refObject = refObject;
	opdata->cacheFile = url.fileName();
	opdata->cacheDir = dir;
	operations[download(url)] = opdata;
}

void CloudInterface::incrementListenCount( QString tapeId ) {
	if ( !cachedTapes.contains(tapeId) ) return;
	TapeObject* tape = cachedTapes[tapeId];
	tape->setListen( tape->getListen()+1 );
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::ReplyNotUsed;
	opdata->isJSON = true;
	QJsonObject increment;
	increment["__op"] = "Increment";
	increment["amount"] = 1;
	QJsonObject data;
	data["listenCount"] = increment;
	operations[put(QString("/classes/QTapes/")+tapeId, data)] = opdata;
}

void CloudInterface::shareTapeOnSocials( QString tapeId, bool onFacebook, bool onTwitter ) {
	TapeObject* tape;
	if ( cachedTapes.contains(tapeId) ) {
		tape = cachedTapes[tapeId];
	} else {
		tape = new TapeObject( tapeId, false );
		cachedTapes[tapeId] = tape;
	}
	// it first update the object from the PARSE backend to be sure to get right content
	// FIXME: Optimize PARSE download, and check if the content is already updated
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::ShareOnSocials;
	opdata->isJSON = true;
	opdata->tape = tape;
	opdata->shareOnFacebook = onFacebook;
	opdata->shareOnTwitter = onTwitter;
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("include"), QString("parent")) );
	operations[get("/classes/QTapes/"+tapeId, params)] = opdata;
}

QStringList CloudInterface::getFamousList() {
	return famousList;
}

void CloudInterface::downloadFamousTapes() {
	// Check if there is already a pending operation
	foreach( OperationData* pending, operations.values() ) {
		if ( pending->type == OperationData::DownloadFamousList ) {
			return;
		}
	}
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadFamousList;
	opdata->isJSON = true;
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("order"), QString("order")) );
	// it is not necessary to download all data here,
	// the tape data will be downloaded on demand after
	// so, we restrict to just one key because zero is not possible
	params.append( qMakePair(QString("keys"), QString("order")) );
	operations[get("/classes/QFamous", params)] = opdata;
}

QStringList CloudInterface::getFacebookFriendList() {
	return facebookFriendList;
}

void CloudInterface::downloadFacebookFriendList() {
	// call the facebook request for friends
	QFacebook::instance()->requestMyFriends();
}

void CloudInterface::downloadNotificationList() {
	// Check if there is already a pending operation
	foreach( OperationData* pending, operations.values() ) {
		if ( pending->type == OperationData::DownloadNotificationList ) {
			return;
		}
	}
	// if there is no logged user, it return without doing nothing
	if ( !user ) return;
	OperationData* opdata = new OperationData();
	opdata->type = OperationData::DownloadNotificationList;
	opdata->isJSON = true;
	QJsonObject query;
	// NOT HIDDEN QUERY
	QJsonObject notExists;
	notExists["$exists"] = false;
	query["hide"] = notExists;
	// TO USER QUERY
	QJsonObject userRef;
	userRef["__type"] = QString::fromUtf8("Pointer");
	userRef["className"] = QString::fromUtf8("_User");
	userRef["objectId"] = user->getId();
	query["toUser"] = userRef;
	QJsonDocument jsonDoc(query);
	QList< QPair<QString,QString> > params;
	params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
	params.append( qMakePair(QString("order"), QString("-createdAt")) );
	operations[get("/classes/Notifications", params)] = opdata;
}

NotificationObject* CloudInterface::getNotify(QString id) {
	if ( cachedNotifications.contains(id) ) {
		return cachedNotifications[id];
	}
	return new NotificationObject();
}

void CloudInterface::onRequestFinished(QNetworkReply *reply) {
	if ( !operations.contains(reply) ) {
		qDebug() << "CloudInterface Error - Not reply into operations map !!";
	}
	OperationData* opdata = operations.take(reply);
	QJsonObject data;
	if ( opdata->isJSON ) {
		data = QJsonDocument::fromJson( reply->readAll() ).object();
		//qDebug() << "CloudInterface Operation Data:" << data;
	}
	bool deleteOpData = true;
	bool hasError = false;
	QString errorMessage;
	if ( data.contains("error") ) {
		hasError = true;
		errorMessage = data["error"].toString();
	}
	if ( reply->error() != QNetworkReply::NoError ) {
		hasError = true;
		if ( data.contains("error") ) {
			errorMessage = data["error"].toString();
		} else {
			errorMessage = reply->errorString();
		}
	}
	switch( opdata->type ) {
	case OperationData::CreateUser: {
		if ( hasError ) {
			emit userCreationFailed( errorMessage );
		} else {
			user = new UserObject( data["objectId"].toString(), data["email"].toString(), data["sessionToken"].toString() );
			cachedUsers[user->getId()] = user;
			emit meChanged(user);
			emit userLoggedIn();
			emit userCreationDone();
			downloadNotificationList();
			saveLoggedUser();
		}
	}
	break;
	case OperationData::Login: {
		if ( hasError ) {
			emit userLoggingError( errorMessage );
		} else {
			QString id = data["objectId"].toString();
			if ( cachedUsers.contains(id) ) {
				user = cachedUsers[id];
				user->setEmail( data["email"].toString() );
				user->setToken( data["sessionToken"].toString() );
				user->forceUpdate();
			} else {
				user = new UserObject( data["objectId"].toString(), data["email"].toString(), data["sessionToken"].toString() );
				cachedUsers[user->getId()] = user;
			}
			emit meChanged(user);
			emit userLoggedIn();
			downloadNotificationList();
			saveLoggedUser();
		}
	}
	break;
	case OperationData::PostTape: {
		if ( hasError ) {
			emit postTapeFailed( errorMessage );
		} else {
			switch(opdata->phase) {
			case 0:
				// uploaded the audio file,
				if ( opdata->tapeParent.isEmpty() ) {
					// now uploading the image file
					opdata->tapeAudio = data["name"].toString();
					opdata->phase = 1;
					operations[post(Backend::instance()->lastTapeImage())] = opdata;
					deleteOpData = false;
				} else {
					// the tape is a comment or a crossing,
					// there is no other data to upload
					// now create the Tapes object
					opdata->phase = 3;
					opdata->tapeAudio = data["name"].toString();
					QJsonObject tapej;
					tapej["text"] = opdata->tapeTitle;
					QJsonObject parentRef;
					parentRef["__type"] = QString::fromUtf8("Pointer");
					parentRef["className"] = QString::fromUtf8("QTapes");
					parentRef["objectId"] = opdata->tapeParent;
					tapej["parent"] = parentRef;
					tapej["isCrossing"] = opdata->tapeIsCrossing;
					QJsonObject audioRef;
					audioRef["__type"] = QString::fromUtf8("File");
					audioRef["name"] = opdata->tapeAudio;
					tapej["audioFile"] = audioRef;
					tapej["duration"] = opdata->tapeDuration;
					QJsonObject userRef;
					userRef["__type"] = QString::fromUtf8("Pointer");
					userRef["className"] = QString::fromUtf8("_User");
					userRef["objectId"] = user->getId();
					tapej["user"] = userRef;
					tapej["listenCount"] = 0;
					tapej["allTags"] = QJsonArray::fromStringList(opdata->tapeTags);
					tapej["atTags"] = QJsonArray::fromStringList(opdata->tapeTags.filter("@"));
					tapej["hashTags"] = QJsonArray::fromStringList(opdata->tapeTags.filter("#"));
					QJsonObject postedDate;
					postedDate["__type"] = "Date";
					postedDate["iso"] = QDateTime::currentDateTime().toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
					tapej["postedAt"] = postedDate;
					operations[post("classes/QTapes", tapej)] = opdata;
					deleteOpData = false;
				}
			break;
			case 1:
				// uploaded the image file
				// now upload the social image file
				opdata->tapeImage = data["name"].toString();
				opdata->phase = 2;
				operations[post(Backend::instance()->lastTapeImageSocial())] = opdata;
				deleteOpData = false;
			break;
			case 2: {
				// uploaded the social image file
				// now create the Tapes object
				opdata->tapeImageSocial = data["name"].toString();
				opdata->phase = 3;
				// in common between tapes and private tapes
				QJsonObject tapej;
				tapej["text"] = opdata->tapeTitle;
				QJsonObject audioRef;
				audioRef["__type"] = QString::fromUtf8("File");
				audioRef["name"] = opdata->tapeAudio;
				tapej["audioFile"] = audioRef;
				QJsonObject imageRef;
				imageRef["__type"] = QString::fromUtf8("File");
				imageRef["name"] = opdata->tapeImage;
				tapej["imageFile"] = imageRef;
				QJsonObject imageSocialRef;
				imageSocialRef["__type"] = QString::fromUtf8("File");
				imageSocialRef["name"] = opdata->tapeImageSocial;
				tapej["socialImageFile"] = imageSocialRef;
				tapej["allTags"] = QJsonArray::fromStringList(opdata->tapeTags);
				tapej["atTags"] = QJsonArray::fromStringList(opdata->tapeTags.filter("@"));
				tapej["hashTags"] = QJsonArray::fromStringList(opdata->tapeTags.filter("#"));
				QJsonObject postedDate;
				postedDate["__type"] = "Date";
				postedDate["iso"] = QDateTime::currentDateTime().toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
				tapej["postedAt"] = postedDate;
				// not in common between tapes and private tapes
				if ( opdata->tapeIsPrivate ) {
					tapej["duration2"] = opdata->tapeDuration;
					QJsonObject fromUserRef;
					fromUserRef["__type"] = QString::fromUtf8("Pointer");
					fromUserRef["className"] = QString::fromUtf8("_User");
					fromUserRef["objectId"] = user->getId();
					tapej["fromUser"] = fromUserRef;
					QJsonObject toUserRef;
					toUserRef["__type"] = QString::fromUtf8("Pointer");
					toUserRef["className"] = QString::fromUtf8("_User");
					toUserRef["objectId"] = opdata->tapeToUser;
					tapej["toUser"] = toUserRef;
					operations[post("classes/QInbox", tapej)] = opdata;
				} else {
					tapej["duration"] = opdata->tapeDuration;
					QJsonObject userRef;
					userRef["__type"] = QString::fromUtf8("Pointer");
					userRef["className"] = QString::fromUtf8("_User");
					userRef["objectId"] = user->getId();
					tapej["user"] = userRef;
					tapej["listenCount"] = 0;
					operations[post("classes/QTapes", tapej)] = opdata;
				}
				deleteOpData = false;
			}
			break;
			case 3:
				if ( opdata->tapeParent.isEmpty() ) {
					// increment the counter of tapes for the user
					user->incrementTapesCount();
				}
				emit postTapeDone( data["objectId"].toString() );
			break;
			case 4: {
				// the parent tape of a crossing has been downloaded
				// now it perform the cross and upload the file
				QString audioParentFile = QStandardPaths::writableLocation( QStandardPaths::TempLocation )+"/crossParent.mp4";
				QFile audioParent(audioParentFile);
				audioParent.open( QIODevice::WriteOnly | QIODevice::Truncate );
				audioParent.write( reply->readAll() );
				audioParent.flush();
				audioParent.close();
				QString crossedTapeFile = Backend::instance()->crossTape(
							audioParentFile, opdata->tapeDuration );
				opdata->phase = 0;
				opdata->isJSON = true;
				operations[post(QString("file://")+crossedTapeFile)] = opdata;
				deleteOpData = false;
			}
			break;
			}
		}
	}
	case OperationData::DownloadWallList: {
		if ( hasError ) break; // ERROR NOT HANDLED
		QJsonArray results = data["results"].toArray();
		QStringList newWallList;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject tapej = results[i].toObject();
			QString id = tapej["objectId"].toString();
			newWallList << id;
		}
		wallList = newWallList;
		emit wallListChanged( wallList );
	}
	break;
	case OperationData::DownloadUserData: {
		if ( hasError ) {
			// TODO: display the error
		} else {
			switch(opdata->phase) {
			case 0: {
				if ( data.contains("email") ) {
					opdata->user->setEmail( data["email"].toString() );
				}
				opdata->user->setDisplayName( data["displayName"].toString() );
				opdata->user->setImageFile( data["imageFile"].toObject()["url"].toString() );
				opdata->user->setDescription( data["description"].toString() );
				// it use a Map to order the friends ID by displayName
				QMap<QString, QString> friendsMap;
				QJsonArray jfriends = data["friends"].toArray();
				for( int i=0; i<jfriends.count(); i++ ) {
					QJsonObject aFriend = jfriends[i].toObject();
					friendsMap[aFriend["displayName"].toString()] = aFriend["objectId"].toString();
				}
				opdata->user->setFriends( friendsMap.values() );
				if ( opdata->user == user ) {
					downloadWallList();
				}
				QJsonArray jsent = data["requestsSent"].toArray();
				QStringList reqSents;
				for( int i=0; i<jsent.count(); i++ ) {
					reqSents << jsent[i].toObject()["objectId"].toString();
				}
				QJsonArray jrec = data["requestsReceived"].toArray();
				QStringList reqRec;
				for( int i=0; i<jrec.count(); i++ ) {
					reqRec << jrec[i].toObject()["objectId"].toString();
				}
				opdata->user->setRequestLists(reqRec, reqSents);
				opdata->phase = 1;
				// download the tapes made by the user (not including the comments)
				QJsonObject where;
				QJsonObject notExists;
				notExists["$exists"] = false;
				where["parent"] = notExists;
				QJsonObject userPointer;
				userPointer["__type"] = QString::fromUtf8("Pointer");
				userPointer["className"] = QString::fromUtf8("_User");
				userPointer["objectId"] = opdata->user->getId();
				where["user"] = userPointer;
				QJsonDocument jsonDoc(where);
				QList< QPair<QString,QString> > params;
				params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
				params.append( qMakePair(QString("limit"), QString("1000") ) );
				params.append( qMakePair(QString("order"), QString("-postedAt")) );
				params.append( qMakePair(QString("keys"), QString("listenCount")) );
				operations[get("/classes/QTapes", params)] = opdata;
				deleteOpData = false;
			}
			break;
			case 1: {
				QJsonArray results = data["results"].toArray();
				QStringList userTapes;
				for( int i=0; i<results.count(); i++ ) {
					QJsonObject tape = results[i].toObject();
					userTapes << tape["objectId"].toString();
				}
				opdata->user->setTapes( userTapes );
				if ( opdata->user == this->getMe() ) {
					// for the current user, it download also the inbox tapes
					opdata->phase = 2;
					QJsonObject where;
					QJsonObject userPointer;
					userPointer["__type"] = QString::fromUtf8("Pointer");
					userPointer["className"] = QString::fromUtf8("_User");
					userPointer["objectId"] = opdata->user->getId();
					where["toUser"] = userPointer;
					QJsonDocument jsonDoc(where);
					QList< QPair<QString,QString> > params;
					params.append( qMakePair(QString("where"), QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
					params.append( qMakePair(QString("limit"), QString("1000") ) );
					params.append( qMakePair(QString("order"), QString("-postedAt")) );
					params.append( qMakePair(QString("keys"), QString("duration")) );
					operations[get("/classes/QInbox", params)] = opdata;
					deleteOpData = false;
				} else {
					opdata->user->updateDone();
				}
			}
			break;
			case 2: {
				QJsonArray results = data["results"].toArray();
				QStringList inboxTapes;
				for( int i=0; i<results.count(); i++ ) {
					QJsonObject tape = results[i].toObject();
					inboxTapes << tape["objectId"].toString();
				}
				opdata->user->setInbox( inboxTapes );
				opdata->user->updateDone();
			}
			break;
			}
		}
	}
	break;
	case OperationData::DownloadTapeData: {
		if ( hasError ) break; // ERROR NOT HANDLED
		// this is actual an array of data with the tape
		// and all its comments
		QJsonArray results = data["results"].toArray();
		QStringList allComments;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject tapej = results[i].toObject();
			TapeObject* tape;
			QString id = tapej["objectId"].toString();
			if ( cachedTapes.contains(id) ) {
				// updating the cached tape
				tape = cachedTapes[id];
			} else {
				// creating a new tape and cache it
				tape = new TapeObject(id);
				cachedTapes[id] = tape;
			}
			if ( tapej["parent"].isNull() ) {
				// main tape
				tape->setAudio( tapej["audioFile"].toObject()["url"].toString() );
				tape->setImage( tapej["imageFile"].toObject()["url"].toString() );
				tape->setImageSocial( tapej["socialImageFile"].toObject()["url"].toString() );
				tape->setTitle( tapej["text"].toString() );
				tape->setDuration( tapej["duration"].toInt() );
				tape->setListen( tapej["listenCount"].toInt() );
				QStringList tags;
				QJsonArray jtags = tapej["allTags"].toArray();
				for( int i=0; i<jtags.count(); i++ ) {
					tags << jtags[i].toString();
				}
				tape->setTags( tags );
				tape->setAuthor( tapej["user"].toObject()["objectId"].toString() );
				//qDebug() << "GET POSTEDAT" << tapej["postedAt"].toObject()["iso"].toString();
				tape->setPostedAt( QDateTime::fromString(
								tapej["postedAt"].toObject()["iso"].toString(),
								"yyyy-MM-dd'T'HH:mm:ss.zzz'Z'"));
				// do not call updateDone here ...
				// but at the end after setting the allComments properties
				//tape->updateDone();
			} else {
				// comment or crossing
				tape->setAudio( tapej["audioFile"].toObject()["url"].toString() );
				tape->setTitle( tapej["text"].toString() );
				tape->setDuration( tapej["duration"].toInt() );
				tape->setListen( tapej["listenCount"].toInt() );
				tape->setTags( tapej["allTags"].toString().split('|',QString::SkipEmptyParts) );
				tape->setAuthor( tapej["user"].toObject()["objectId"].toString() );
				tape->setParentTape( tapej["parent"].toObject()["objectId"].toString() );
				tape->setIsCrossing( tapej["isCrossing"].toBool() );
				QStringList tags;
				QJsonArray jtags = tapej["allTags"].toArray();
				for( int i=0; i<jtags.count(); i++ ) {
					tags << jtags[i].toString();
				}
				tape->setTags( tags );
				tape->setPostedAt( QDateTime::fromString(
								tapej["postedAt"].toObject()["iso"].toString(),
								"yyyy-MM-dd'T'HH:mm:ss.zzz'Z'"));
				tape->updateDone();
				allComments << tape->getId();
			}
		}
		opdata->tape->setAllComments( allComments );
		opdata->tape->updateDone();
	}
	break;
	case OperationData::DownloadChannelList: {
		if ( hasError ) break; // ERROR NOT HANDLED
		QJsonArray results = data["results"].toArray();
		QStringList newChannelList;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject userj = results[i].toObject();
			if ( !userj["visible"].toBool() ) continue;
			QString id = userj["user"].toObject()["objectId"].toString();
			newChannelList << id;
			if ( userj["isDefault"].toBool() ) {
				defaultChannel = id;
			}
		}
		channelList = newChannelList;
		emit channelListChanged( channelList );
	}
	break;
	case OperationData::DownloadNotificationList: {
		if ( hasError ) break; // ERROR NOT HANDLED
		QJsonArray results = data["results"].toArray();
		QStringList newNotificationList;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject notifj = results[i].toObject();
			QString id = notifj["objectId"].toString();
			QString fromUser = notifj["fromUser"].toObject()["objectId"].toString();
			QString toUser = notifj["toUser"].toObject()["objectId"].toString();
			bool read = notifj["read"].toBool();
			QString tape;
			if ( notifj.contains("tape") ) {
				tape = notifj["tape"].toObject()["objectId"].toString();
			}
			NotificationObject::NotificationType type;
			QString typej = notifj["type"].toString();
			if ( typej == "friendRequest" ) {
				type = NotificationObject::FriendRequest;
			} else if ( typej == "requestAccepted" ) {
				type = NotificationObject::RequestAccepted;
			} else if ( typej == "commentAdded" ) {
				type = NotificationObject::CommentAdded;
			} else if ( typej == "sentPrivateTape" ) {
				type = NotificationObject::SentPrivateTape;
			} else if ( typej == "userTagged" ) {
				type = NotificationObject::UserTagged;
			}
			newNotificationList << id;
			// check if to create a new object or update the cached notification object
			if ( cachedNotifications.contains(id) ) {
				cachedNotifications[id]->setRead( read );
			} else {
				cachedNotifications[id] = new NotificationObject(id, type, fromUser, toUser, read, tape);
			}
		}
		notificationList = newNotificationList;
		emit notificationListChanged( notificationList );
		updateUnreadNotifications();
	}
	break;
	case OperationData::LinkFacebookUser: {
		if ( hasError ) {
			emit userLoggingError( errorMessage );
		} else {
			QString id = data["objectId"].toString();
			if ( cachedUsers.contains(id) ) {
				user = cachedUsers[id];
				user->setToken( data["sessionToken"].toString() );
				user->setFacebookId( data["authData"].toObject()["facebook"].toObject()["id"].toString() );
				user->forceUpdate();
			} else {
				if ( data["displayName"].toString().isEmpty() ) {
					// the user is the first time he/she log in with Facebook
					user = new UserObject( data["objectId"].toString(), false );
					user->setToken( data["sessionToken"].toString() );
					user->setFacebookId( data["authData"].toObject()["facebook"].toObject()["id"].toString() );
					user->setEmail( opdata->email );
					user->setDisplayName( opdata->displayName );
					cachedUsers[id] = user;
					CloudInterface::uploadUserData(user, QStringList() << "email" << "displayName" << "facebookId");
				} else {
					// a returning user, but not in cache
					user = new UserObject( data["objectId"].toString() );
					user->setToken( data["sessionToken"].toString() );
					user->setFacebookId( data["authData"].toObject()["facebook"].toObject()["id"].toString() );
					cachedUsers[id] = user;
				}
			}
			emit meChanged(user);
			emit userLoggedIn();
			downloadNotificationList();
			saveLoggedUser();
		}
	}
	break;
	case OperationData::UploadUserData: {
		if ( hasError ) break; // ERROR NOT HANDLED
		// not do anything if phase is not 1
		if ( opdata->phase == 1 ) {
			// the new user image has been uploaded, use it and update all data
			QJsonObject udata;
			foreach( QString property, opdata->userProperties ) {
				if ( property == "imageFile" ) {
					QJsonObject imageRef;
					imageRef["__type"] = QString::fromUtf8("File");
					imageRef["name"] = data["name"].toString();
					udata["imageFile"] = imageRef;
				} else {
					udata[property] = QJsonValue::fromVariant(
								opdata->user->property( property.toLatin1().data()));
				}
			}
			// prepare operation data
			opdata->phase = 0;
			opdata->isJSON = true;
			operations[put(QString("/users/")+opdata->user->getId(), udata)] = opdata;
			deleteOpData = false;
		}
	}
	break;
	case OperationData::DeleteTape: {
		if ( hasError ) {
		} else {
			downloadWallList();
		}
	}
	break;
	case OperationData::Friendship: {
		if ( hasError ) break; // ERROR NOT HANDLED
		// Just update the data of the involved users
		opdata->user->forceUpdate();
		user->forceUpdate();
	}
	break;
	case OperationData::SearchUsers: {
		if ( hasError ) {
			emit searchUsersDone( QStringList() );
		} else {
			QJsonArray results = data["result"].toArray();
			QStringList usersFound;
			for( int i=0; i<results.count(); i++ ) {
				usersFound << results[i].toObject()["objectId"].toString();
			}
			emit searchUsersDone( usersFound );
		}
	}
	break;
	case OperationData::SearchTapes: {
		if ( hasError ) {
			emit searchTapesDone( QStringList() );
		} else {
			QJsonArray results = data["result"].toArray();
			QStringList tapesFound;
			for( int i=0; i<results.count(); i++ ) {
				tapesFound << results[i].toObject()["objectId"].toString();
			}
			emit searchTapesDone( tapesFound );
		}
	}
	break;
	case OperationData::CacheFile: {
		if ( hasError ) {
			// there is no error handling :-(
			qDebug() << "ERROR ON DOWNLOADING CACHE FILE";
		} else {
			QDir cacheDir( QStandardPaths::writableLocation(QStandardPaths::CacheLocation) );
			cacheDir.mkpath(opdata->cacheDir);
			QFile cacheFile( cacheDir.absoluteFilePath(opdata->cacheDir+"/"+opdata->cacheFile) );
			cacheFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
			cacheFile.write( reply->readAll() );
			cacheFile.flush();
			cacheFile.close();
			qDebug() << "CACHED FILE" << opdata->cacheFile << opdata->cacheDir;
			emit fileCached(opdata->refObject, opdata->cacheFile, opdata->cacheDir );
		}
	}
	break;
	case OperationData::ShareOnSocials: {
		if ( hasError ) {
			// there is no error handling :-(
			qDebug() << "ERROR ON Sharing on Social";
			emit shareTapeOnSocialsDone();
		} else {
			// Update the content of the tape
			opdata->tape->setAudio( data["audioFile"].toObject()["url"].toString() );
			opdata->tape->setImage( data["imageFile"].toObject()["url"].toString() );
			if ( data.contains("socialImageFile") ) {
				opdata->tape->setImageSocial( data["socialImageFile"].toObject()["url"].toString() );
				qDebug() << "HERE:" << data["socialImageFile"];
			} else {
				// take socialImageFile from parent
				opdata->tape->setImageSocial( data["parent"]
						.toObject()["socialImageFile"]
						.toObject()["url"].toString() );
				qDebug() << "THERE:" << data["parent"] << data["parent"].toObject()["socialImageFile"];
			}
			opdata->tape->setTitle( data["text"].toString() );
			opdata->tape->setDuration( data["duration"].toInt() );
			opdata->tape->setListen( data["listenCount"].toInt() );
			QStringList tags;
			QJsonArray jtags = data["allTags"].toArray();
			for( int i=0; i<jtags.count(); i++ ) {
				tags << jtags[i].toString();
			}
			opdata->tape->setTags( tags );
			opdata->tape->setAuthor( data["user"].toObject()["objectId"].toString() );
			opdata->tape->setPostedAt( QDateTime::fromString(
							data["postedAt"].toObject()["iso"].toString(),
							"yyyy-MM-dd'T'HH:mm:ss.zzz'Z'"));
			opdata->tape->updateDone();
			// Share on Facebook
			if ( opdata->shareOnFacebook ) {
				QFacebook::instance()->publishLinkViaShareDialog(
							opdata->tape->getTitle(),
							opdata->tape->getAudio().toString(),
							opdata->tape->getImageSocial().toString(),
							"",
							"Make a tape with totape.com");
			} else if ( opdata->shareOnTwitter ) {
				Backend::instance()->shareTapeOnTwitter( opdata->tape );
			}
			deleteOpData = false;
			shareSocialData = opdata;
		}
	}
	case OperationData::DownloadFamousList: {
		if ( hasError ) break; // ERROR NOT HANDLED
		QJsonArray results = data["results"].toArray();
		QStringList newFamousList;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject tapej = results[i].toObject();
			QString id = tapej["objectId"].toString();
			newFamousList << id;
		}
		famousList = newFamousList;
		emit famousListChanged( famousList );
	}
	break;
	case OperationData::DownloadFamousData: {
		if ( hasError ) break; // ERROR NOT HANDLED
		// the data of the famous tape
		opdata->tape->setAudio( data["audioFile"].toObject()["url"].toString() );
		opdata->tape->setImage( data["imageFile"].toObject()["url"].toString() );
		opdata->tape->setTitle( data["text"].toString() );
		opdata->tape->setDuration( data["duration"].toInt() );
		opdata->tape->updateDone();
	}
	break;
	case OperationData::DownloadInboxTapeData: {
		if ( hasError ) break; // ERROR NOT HANDLED
		// the data of the inbox tape
		opdata->tape->setAudio( data["audioFile"].toObject()["url"].toString() );
		opdata->tape->setImage( data["imageFile"].toObject()["url"].toString() );
		opdata->tape->setImageSocial( data["socialImageFile"].toObject()["url"].toString() );
		opdata->tape->setTitle( data["text"].toString() );
		opdata->tape->setDuration( data["duration2"].toInt() );
		opdata->tape->setAuthor( data["fromUser"].toObject()["objectId"].toString() );
		QStringList tags;
		QJsonArray jtags = data["allTags"].toArray();
		for( int i=0; i<jtags.count(); i++ ) {
			tags << jtags[i].toString();
		}
		opdata->tape->setTags( tags );
		opdata->tape->setPostedAt( QDateTime::fromString(
						data["postedAt"].toObject()["iso"].toString(),
						"yyyy-MM-dd'T'HH:mm:ss.zzz'Z'"));
		opdata->tape->updateDone();
	}
	break;
	case OperationData::DownloadFacebookFriendList: {
		if ( hasError ) break; // ERROR NOT HANDLED
		QJsonArray results = data["results"].toArray();
		QStringList newFacebookFriendList;
		for( int i=0; i<results.count(); i++ ) {
			QJsonObject userj = results[i].toObject();
			QString id = userj["objectId"].toString();
			newFacebookFriendList << id;
		}
		facebookFriendList = newFacebookFriendList;
		emit facebookFriendListChanged( facebookFriendList );
	}
	break;
	case OperationData::ReplyNotUsed:
	break;
	}
	if ( deleteOpData ) {
		delete opdata;
	}
	reply->deleteLater();
}

void CloudInterface::onTwitterShareDone() {
	// FIXME: there is no error handling
	emit shareTapeOnSocialsDone();
}

void CloudInterface::onFacebookOperationDone(QString operation, QVariantMap data) {
	if ( operation == "publishLinkViaShareDialog") {
		if ( shareSocialData->shareOnTwitter ) {
			// share on twitter
			Backend::instance()->shareTapeOnTwitter( shareSocialData->tape );
		} else {
			emit shareTapeOnSocialsDone();
		}
		return;
	}
	if ( operation == "requestMyFriends" ) {
		QStringList friendList = data["friends"].toStringList();
		qDebug() << "RETURNED LIST OF FRIENDS:" << friendList;
		if ( friendList.isEmpty() ) {
			facebookFriendList = QStringList();
			emit facebookFriendListChanged( facebookFriendList );
		} else {
			// Query PARSE for getting user ids
			OperationData* opdata = new OperationData();
			opdata->type = OperationData::DownloadFacebookFriendList;
			opdata->isJSON = true;
			QJsonObject friendIds;
			friendIds["$in"] = QJsonArray::fromStringList( friendList );
			QJsonObject query;
			query["facebookId"] = friendIds;
			QJsonDocument jsonDoc(query);
			QList< QPair<QString,QString> > params;
			params.append( qMakePair(QString("where"),
									 QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Compact))) );
			params.append( qMakePair(QString("limit"), QString::number(1000)) );
			operations[get("/users", params)] = opdata;
		}
	}
}

// it manage the returning of Facebook sharing
void CloudInterface::onFacebookOperationError(QString operation, QString error) {
	if ( operation == "publishLinkViaShareDialog" ) {
		// FIXME: missing handling of ERROR
		emit shareTapeOnSocialsDone();
	}
	if ( operation == "requestMyFriends" ) {
		// FIXME: missing handling of ERROR
		facebookFriendList = QStringList();
		emit facebookFriendListChanged( facebookFriendList );
	}
}

void CloudInterface::updateUnreadNotifications() {
	int newCount = 0;
	foreach( QString notifyId, notificationList ) {
		if ( cachedNotifications.contains(notifyId) ) {
			if ( !cachedNotifications[notifyId]->getRead() ) {
				newCount++;
			}
		}
	}
	unreadNotifications = newCount;
	emit unreadNotificationsChanged(unreadNotifications);
}

void CloudInterface::saveLoggedUser() {
	QSettings settings;
	settings.beginGroup("user");
	settings.setValue("objectId", user->getId());
	settings.setValue("email", user->getEmail());
	settings.setValue("sessionToken", user->getToken());
	settings.setValue("facebookId", user->getFacebookId());
	settings.endGroup();
}

QNetworkReply* CloudInterface::post( QString endPoint, QJsonObject operation ) {
	QNetworkRequest request(QString("https://api.parse.com/1/"+endPoint));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	request.setRawHeader("Content-Type", "application/json");
	QJsonDocument jsonDoc(operation);
	return net->post( request, jsonDoc.toJson(QJsonDocument::Compact) );
}

QNetworkReply* CloudInterface::get( QString endPoint, QList< QPair<QString,QString> > operation ) {
	QUrl url( QString("https://api.parse.com/1/"+endPoint) );
	if ( operation.size() > 0 ) {
		QUrlQuery query;
		query.setQueryItems( operation );
		url.setQuery( query );
	}
	QNetworkRequest request(url);
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	return net->get( request );
}

QNetworkReply* CloudInterface::post(QUrl url) {
	QFileInfo fileInfo(url.toLocalFile());
	//qDebug() << "POSTING" << url << fileInfo.fileName();
	QNetworkRequest request(QString("https://api.parse.com/1/files/"+fileInfo.fileName()));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	fileToPost = new QFile(url.toLocalFile());
	fileToPost->open( QIODevice::ReadOnly );
	request.setRawHeader("Content-Type", mimeDb.mimeTypeForData(fileToPost).name().toLatin1());
	return net->post( request, fileToPost );
}

QNetworkReply* CloudInterface::post(QString filename) {
	QUrl url(filename);
	QFileInfo fileInfo(url.toLocalFile());
	QNetworkRequest request(QString("https://api.parse.com/1/files/"+fileInfo.fileName()));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	fileToPost = new QFile(url.toLocalFile());
	fileToPost->open( QIODevice::ReadOnly );
	request.setRawHeader("Content-Type", mimeDb.mimeTypeForData(fileToPost).name().toLatin1());
	return net->post( request, fileToPost );
}

QNetworkReply* CloudInterface::download( QUrl fileurl ) {
	QNetworkRequest request( fileurl );
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	return net->get( request );
}

QNetworkReply* CloudInterface::put( QString endPoint, QJsonObject operation ) {
	QNetworkRequest request(QString("https://api.parse.com/1/"+endPoint));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	request.setRawHeader("Content-Type", "application/json");
	QJsonDocument jsonDoc(operation);
	return net->put( request, jsonDoc.toJson(QJsonDocument::Compact) );
}

QNetworkReply* CloudInterface::deleteObject( QString className, QString objectId ) {
	QNetworkRequest request(QString("https://api.parse.com/1/classes/"+className+"/"+objectId));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	if ( user ) {
		// if there is a user logged in, send also the session token
		request.setRawHeader("X-Parse-Session-Token", user->getToken().toLatin1());
	}
	return net->deleteResource( request );
}
