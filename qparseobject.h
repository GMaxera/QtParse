#ifndef QPARSEOBJECT_H
#define QPARSEOBJECT_H

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>
#include "qparsetypes.h"

class QParseReply;

/*! This class contain all common and generic operation than
 *  can be done on all objects stored on PARSE
 *
 *  This class is abstract, and in order to use you have to
 *  subclass and implement the protected method parseClassName
 *
 *  If you need a customized operation on your particular object
 *  subclass this one and implement your operation
 */
class QParseObject : public QObject {
	Q_OBJECT
	//! the object id on PARSE
	Q_PROPERTY( QString objectId READ getObjectId CONSTANT )
	/*! the createdAt on PARSE
	 *  The only case on which the createdAtChanged will be fired is at the creation of object on PARSE
	 */
	Q_PROPERTY( QParseDate createdAt READ getCreatedAt NOTIFY createdAtChanged )
	//! the updatedAt on PARSE
	Q_PROPERTY( QParseDate updatedAt READ getUpdatedAt NOTIFY updatedAtChanged )
	//! if true means that new data will be downloaded from cloud and the current may not valid anymore
	Q_PROPERTY( bool updating READ isUpdating NOTIFY updatingChanged )
	/*! if true means that there is a pending request of saving data
	 *  \note Any update request during save will be ignored
	 *  \warning Avoid to change any data of the object during save operation
	 */
	Q_PROPERTY( bool saving READ isSaving NOTIFY savingChanged )
public:
	/*! constructor of a new object
	 *  The object will have a empty id, and calling a save with empty id
	 *  will result into a creation of new object in PARSE
	 *
	 *  \warning When created from C++ side always set the parent to QParse::instance(),
	 *		this will avoid QML to take ownership of object and destroy when not expected
	 */
	QParseObject( QObject* parent=0 );
	/*! construct a new object initializing the properties using Json data from PARSE
	 *
	 *  \warning When created from C++ side always set the parent to QParse::instance(),
	 *		this will avoid QML to take ownership of object and destroy when not expected
	 */
	Q_INVOKABLE QParseObject( QJsonObject jsonData, QObject* parent=0 );
	//! return the class name used on PARSE for this object
	virtual QString parseClassName() { return QString(); }
	//! return the list of properties used on PARSE for this object
	virtual QStringList parseProperties() { return QStringList(); }
public slots:
	QString getObjectId();
	QParseDate getCreatedAt() const;
	QParseDate getUpdatedAt() const;
	//! indicate if there is an updating operation ongoing
	bool isUpdating();
	/*! update the data getting them from PARSE
	 *  \note this does not necessary means it will do a real network request
	 *        because depending on the QParse caching settings, the data might
	 *        be retrieved from the network cache
	 */
	void update();
	//! indicate if there is an updating operation ongoing
	bool isSaving();
	//! save the data to PARSE
	void save();
	/*! return a JSON object representing the object for PARSE
	 *  \param onlyChanged if true return a partial representation of the object with
	 *			only the properties changed since the last saving
	 */
	QJsonObject toJson( bool onlyChanged=false );
	//! return the JSON pointer-to-object for pointer types on PARSE
	QJsonObject getJsonPointer();
signals:
	void updatingChanged( bool updating );
	void updatingDone();
	void savingChanged( bool saving );
	void savingDone();
	void createdAtChanged(QParseDate createdAt);
	void updatedAtChanged(QParseDate updatedAt);
private slots:
	/*! handle the completion of update request */
	void onUpdateReply( QParseReply* reply );
	/*! handle the completion of save request */
	void onSaveReply( QParseReply* reply );
private:
	QString objectId;
	QParseDate createdAt;
	QParseDate updatedAt;
	bool updating;
	bool saving;
};

#endif // QPARSEOBJECT_H
