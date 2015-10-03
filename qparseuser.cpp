#include "userobject.h"
#include "cloudinterface.h"
#include <QDebug>

UserObject::UserObject() :
	// no parent in order to let QML to take ownership
	QObject(),
	tapesCount(0),
	friendsCount(0),
	inboxCount(0),
	updating(true) {
}

UserObject::UserObject(QString id , bool updateNow) :
	// parent to cloud interface to prevent the QML to take ownership
	QObject(CloudInterface::instance()),
	id(id),
	facebookId(),
	token(),
	tapesCount(0),
	friendsCount(0),
	inboxCount(0),
	updating(true) {
	if ( !id.isEmpty() && updateNow ) {
		CloudInterface::instance()->downloadUserData( this );
	}
}

UserObject::UserObject(QString id, QString email, QString token , QString facebookId) :
	// parent to cloud interface to prevent the QML to take ownership
	QObject(CloudInterface::instance()),
	id(id),
	facebookId(facebookId),
	token(token),
	email(email),
	tapesCount(0),
	inboxCount(0),
	updating(true) {
	CloudInterface::instance()->downloadUserData( this );
}

void UserObject::updateDone() {
	updating = false;
	pulledAt = QDateTime::currentDateTime();
	emit updatingChanged( updating );
	emit updatingFinished();
}

void UserObject::incrementTapesCount() {
	tapesCount++;
	emit tapesCountChanged( tapesCount );
}

void UserObject::setTapes( QStringList tapes ) {
	this->tapes = tapes;
	emit tapesChanged( this->tapes );
	tapesCount = this->tapes.size();
	emit tapesCountChanged( tapesCount );
}

void UserObject::setInbox( QStringList inbox ) {
	this->inbox = inbox;
	emit inboxChanged( this->inbox );
	inboxCount = this->inbox.size();
	emit inboxCountChanged( inboxCount );
}

void UserObject::setImageFile(QUrl imageFile) {
	if ( this->imageFile == imageFile ) return;
	this->imageFile = imageFile;
	emit imageFileChanged( this->imageFile );
}

void UserObject::setFacebookId( QString facebookId ) {
	this->facebookId = facebookId;
}

void UserObject::setToken(QString token) {
	this->token = token;
}

void UserObject::setEmail(QString email) {
	this->email = email;
}

void UserObject::setFriends(QStringList friends) {
	this->friends = friends;
	emit friendsChanged( this->friends );
	friendsCount = this->friends.size();
	emit friendsCountChanged( friendsCount );
}

void UserObject::setRequestLists( QStringList received, QStringList sent ) {
	requestsReceived = received;
	requestsSent = sent;
}

QString UserObject::getId() {
	return id;
}

QString UserObject::getFacebookId() {
	return facebookId;
}

QString UserObject::getToken() {
	return token;
}

QString UserObject::getEmail() {
	return email;
}

QString UserObject::getDisplayName() {
	return displayName;
}

void UserObject::setDisplayName( QString displayName ) {
	if ( this->displayName == displayName ) return;
	this->displayName = displayName;
	emit displayNameChanged( this->displayName );
}

void UserObject::setDescription(QString description) {
	if ( this->description == description ) return;
	this->description = description;
	emit descriptionChanged( this->description );
}

QStringList UserObject::getTapes() {
	return tapes;
}

int UserObject::getTapesCount() {
	return tapesCount;
}

QStringList UserObject::getInbox() {
	return inbox;
}

int UserObject::getInboxCount() {
	return inboxCount;
}

QUrl UserObject::getImageFile() {
	return imageFile;
}

QString UserObject::getDescription() {
	return description;
}

QStringList UserObject::getFriends() {
	return friends;
}

int UserObject::getFriendsCount() {
	return friendsCount;
}

bool UserObject::isUpdating() {
	return updating;
}

void UserObject::forceUpdate() {
	if ( id.isEmpty() ) return;
	if ( updating ) return;
	updating = true;
	CloudInterface::instance()->downloadUserData( this );
}

void UserObject::updateIfOld( QDateTime validTime ) {
	if ( pulledAt < validTime ) {
		forceUpdate();
	}
}

int UserObject::friendshipStatusWith( QString userId ) {
	if ( friends.contains(userId) ) {
		return 3;
	}
	if ( requestsReceived.contains(userId) ) {
		return 2;
	}
	if ( requestsSent.contains(userId) ) {
		return 1;
	}
	return 0;
}
