#ifndef QPARSEREPLY_H
#define QPARSEREPLY_H

#include <QObject>

class QParseRequest;
class QParse;

/*! This object is returned by QParse when a QParseRequest has been submitted.
 *
 *  Bind the signal finished to the proper slot delegate to handle the data returned
 *  by PARSE
 *
 *  \warning Never create by yourself
 */
class QParseReply : public QObject {
	Q_OBJECT
	//! The request corresponding to this reply
	Q_PROPERTY( QParseRequest* request MEMBER request CONSTANT )
	/*! If it's true, then the returned data is a JSON object
	 *  \warning it can be an error, because in case of error Parse.com return a JSON message
	 */
	Q_PROPERTY( bool isJSON MEMBER isJSON )
public:
	/*! Constructor
	 *  the parent is always QParse singleton instance because
	 *  this object is created by QParse singleton
	 */
	QParseReply( QParseRequest* request, QParse* parent );
signals:
	//! emitted when the reply has been arrived and prepared to be processed
	void finished();
private:
	//! the request
	QParseRequest* request;
	//! true means the data is a JSON object
	bool isJSON;
};

#endif // QPARSEREPLY
