#ifndef USEROBJECT_H
#define USEROBJECT_H

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QDateTime>

/* SessionToken returned by PARSE never expires */
class UserObject : public QObject
{
	Q_OBJECT
	// the object id of the user on PARSE
	Q_PROPERTY( QString id READ getId CONSTANT )
	// the facebook id in case the user has been logged/signup using Facebook Login
	Q_PROPERTY( QString facebookId READ getFacebookId CONSTANT )
	// the session token returned by PARSE (it never expires)
	Q_PROPERTY( QString token READ getToken CONSTANT )
	// the email of the user
	Q_PROPERTY( QString email READ getEmail CONSTANT )
	// the name displayed on the interface and visible to other users
	Q_PROPERTY( QString displayName READ getDisplayName WRITE setDisplayName NOTIFY displayNameChanged )
	// the list of his/her tapes (not including the comments)
	Q_PROPERTY( QStringList tapes READ getTapes NOTIFY tapesChanged )
	// the number of tapes created by the user (not including the comments)
	Q_PROPERTY( int tapesCount READ getTapesCount NOTIFY tapesCountChanged )
	// the url of the avatar image file
	Q_PROPERTY( QUrl imageFile READ getImageFile WRITE setImageFile NOTIFY imageFileChanged )
	// the description
	Q_PROPERTY( QString description READ getDescription WRITE setDescription NOTIFY descriptionChanged )
	// the list of his/her friends
	Q_PROPERTY( QStringList friends READ getFriends NOTIFY friendsChanged )
	// the number of friends/followers
	Q_PROPERTY( int friendsCount READ getFriendsCount NOTIFY friendsCountChanged )
	// the list of his/her inbox tapes
	Q_PROPERTY( QStringList inbox READ getInbox NOTIFY inboxChanged )
	// the number of tapes in the inbox
	Q_PROPERTY( int inboxCount READ getInboxCount NOTIFY inboxCountChanged )
	// if true means that new data will be downloaded from cloud and the current are not valid anymore
	Q_PROPERTY( bool updating READ isUpdating NOTIFY updatingChanged )
public:
	// empty constructor for QML compliance
	UserObject();
	// construct a user object with only the id setted without email and token
	UserObject( QString id, bool updateNow=true );
	// construct a user object with full data (only logged user as all data)
	UserObject( QString id, QString email, QString token, QString facebookId=QString() );
	/*! the update is finished \internal */
	void updateDone();
	/*! increment the tapes count \internal */
	void incrementTapesCount();
	/*! set the list of tapes and tapes count \internal */
	void setTapes( QStringList tapes );
	/*! set the list of inbox tapes and tapes count \internal */
	void setInbox( QStringList tapes );
	/*! set the facebook id \internal */
	void setFacebookId( QString facebookId );
	/*! set the session token \internal */
	void setToken( QString token );
	/*! set the email of the user \internal */
	void setEmail( QString email );
	/*! set the image file \internal */
	void setImageFile( QUrl imageFile );
	/*! set the description \internal */
	void setDescription( QString description );
	/*! set the list of friends and friendsCount \internal */
	void setFriends( QStringList friends );
	/*! set the pending requests for friendships \internal */
	void setRequestLists( QStringList received, QStringList sent );
public slots:
	QString getId();
	QString getFacebookId();
	QString getToken();
	QString getEmail();
	QString getDisplayName();
	void setDisplayName( QString displayName );
	QStringList getTapes();
	int getTapesCount();
	QUrl getImageFile();
	QString getDescription();
	QStringList getFriends();
	int getFriendsCount();
	QStringList getInbox();
	int getInboxCount();
	// indicate if there is an updating operation ongoing
	bool isUpdating();
	// force a new update of all data of the user
	void forceUpdate();
	// update all data of the tape if the pulledAt is before the datetime passed
	void updateIfOld( QDateTime validTime );
	/*! return the friendship status with the user
	 *  0 == No Friends
	 *  1 == Pending Request sent to the userId
	 *  2 == Prendin Request received from the userId
	 *  3 == Friends
	 *  \warning return 0 if called on itself
	 */
	int friendshipStatusWith( QString userId );
signals:
	void updatingChanged( bool updating );
	void updatingFinished();
	void displayNameChanged( QString displayName );
	void tapesChanged( QStringList tapes );
	void tapesCountChanged( int tapesCount );
	void imageFileChanged( QUrl imageFile );
	void descriptionChanged( QString description );
	void friendsChanged( QStringList friends );
	void friendsCountChanged( int friendsCount );
	void inboxChanged( QStringList inbox );
	void inboxCountChanged( int inboxCount );
private:
	QString id;
	QString facebookId;
	QString token;
	QString email;
	QString displayName;
	QStringList tapes;
	int tapesCount;
	QUrl imageFile;
	QString description;
	QStringList friends;
	int friendsCount;
	QStringList requestsReceived;
	QStringList requestsSent;
	QStringList inbox;
	int inboxCount;
	bool updating;
	// when the data has been pulled from PARSE
	QDateTime pulledAt;
};

#endif // USEROBJECT_H
