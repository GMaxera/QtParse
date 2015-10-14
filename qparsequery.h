#ifndef QPARSEQUERY_H
#define QPARSEQUERY_H

#include <QObject>
#include "qparse.h"
#include "qparseobject.h"
#include "qparsetypes.h"

/*! Instanced an object of this class for perform a query on PARSE
 *  and retrieving a collection of PARSE objects
 */
class QParseQuery : public QObject {
	Q_OBJECT
public:
	/*! constructor of a new query */
	template<class ParseObject>
	QParseQuery()
		: QObject(QParse::instance())
		, metaParseObject(ParseObject::staticMetaObject) {
	}
public slots:
	//! execute the query
	void query();
signals:
	//! return the all retrieved objects
	void queryResults( QList<QParseObject*> results );
protected:
private slots:
	/*! handle the completion of get request on PARSE */
	void onQueryReply();
private:
	// disable non-template constructors
	QParseQuery();
	Q_DISABLE_COPY( QParseQuery )

	//! the QMetaObject used for constructing the right QParseObject
	QMetaObject metaParseObject;
};

#endif // QPARSEQUERY_H
