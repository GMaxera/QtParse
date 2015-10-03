#ifndef QPARSEOBJECT_H
#define QPARSEOBJECT_H

#include <QObject>
#include <QUrl>
#include <QStringList>
#include <QDateTime>

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
	//! if true means that new data will be downloaded from cloud and the current are not valid anymore
	Q_PROPERTY( bool updating READ isUpdating NOTIFY updatingChanged )
public:
	//! empty constructor for QML compliance
	QParseObject();
	//! construct a user object with only the id setted without email and token
	QParseObject( QString id );
	/*! the update is finished \internal */
	void updateDone();
public slots:
	QString getId();
	//! indicate if there is an updating operation ongoing
	bool isUpdating();
	//! force a new update of all data
	void forceUpdate();
	//! update all data if the pulledAt is before the datetime passed
	void updateIfOld( QDateTime validTime );
signals:
	void updatingChanged( bool updating );
	void updatingFinished();
protected:
	virtual QString parseClassName() = 0;
private:
	QString id;
	bool updating;
	//! when the data has been pulled from PARSE
	QDateTime pulledAt;
};

#endif // QPARSEOBJECT_H
