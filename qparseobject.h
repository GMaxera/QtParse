#ifndef QPARSEOBJECT_H
#define QPARSEOBJECT_H

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QDateTime>
#include <QJsonObject>

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
	Q_PROPERTY( QString id READ getId CONSTANT )
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
	 *		this will avoid QML to take ownership of object and destroy we not expected
	 */
	QParseObject( QObject* parent=0 );
	/*! construct a user object with only the id set
	 *  Call forceUpdate as soon as possible to get data from PARSE about this object
	 *
	 *  \warning When created from C++ side always set the parent to QParse::instance(),
	 *		this will avoid QML to take ownership of object and destroy we not expected
	 */
	QParseObject( QString id, QObject* parent=0 );
public slots:
	QString getId();
	//! indicate if there is an updating operation ongoing
	bool isUpdating();
	//! force a new update of all data
	void forceUpdate();
	//! update all data if the pulledAt is before the datetime passed
	void updateIfOld( QDateTime validTime );
	//! indicate if there is an updating operation ongoing
	bool isSaving();
	//! save the data to PARSE
	void save();
	/*! return a JSON object representing the object for PARSE
	 *  \param onlyChanged if true return a partial representation of the object with
	 *			only the properties changed since the last saving
	 */
	QJsonObject toJson( bool onlyChanged=false );
signals:
	void updatingChanged( bool updating );
	void updatingDone();
	void savingChanged( bool saving );
	void savingDone();
protected:
	//! return the class name used on PARSE for this object
	virtual QString parseClassName() = 0;
	//! return the list of properties used on PARSE for this object
	virtual QStringList parseProperties() = 0;
private slots:
	/*! handle the completion of update request */
	void onUpdateReply();
	/*! handle the completion of save request */
	void onSaveReply();
private:
	QString id;
	bool updating;
	bool saving;
	//! when the data has been pulled from PARSE
	QDateTime pulledAt;
};

#endif // QPARSEOBJECT_H
