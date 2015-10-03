#ifndef QPARSEREQUEST_H
#define QPARSEREQUEST_H

#include <QObject>

class QParseObject;

/*! It represent a request to perform to PARSE.
 *  It can be a query, posting a file or object, retrieving single object
 *
 *  Submit a request to QParse singleton and use the QParseReply to collect results
 *
 */
class QParseRequest : public QObject {
	Q_OBJECT
public:
	//! constructor
	QParseRequest( QString parseClassName, QParseObject* parent );
	/*! add the option and its value to the request
	 *  \param name is the name of option (like 'include', 'where', etc)
	 *  \param value is the value of the option to send
	 */
	void addOption( QString name, QString value );
private:
	//! this is used to create the correct endpoint to network request
	QString parseClassName;
	//! these are used for get network requests
	QList< QPair<QString,QString> > params;
};

#endif // QPARSEREQUEST
