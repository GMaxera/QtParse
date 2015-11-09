
#include "qparse.h"
#include "qparsetypes.h"
#include "qparsereply.h"
#include "qparserequest.h"
#include <QDebug>

QParseDate::QParseDate()
	: json()
	, dateTime() {
}

QParseDate::QParseDate( const QParseDate& src )
	: json(src.json)
	, dateTime(src.dateTime) {
}

QParseDate::QParseDate( QJsonObject fromParse )
	: json()
	, dateTime() {
	if ( !fromParse.contains("__type") || !fromParse.contains("iso") ) {
		qDebug("QParseDate - wrong Json format passed to constructor");
		return;
	}
	json = fromParse;
	dateTime = QDateTime::fromString( json["iso"].toString(), "yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
}

QParseDate::QParseDate( QString isoParse )
	: json()
	, dateTime() {
	dateTime = QDateTime::fromString( isoParse, "yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
	json["__type"] = "Date";
	json["iso"] = dateTime.toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
}

QParseDate::QParseDate( QDateTime fromDateTime )
	: json()
	, dateTime() {
	dateTime = fromDateTime;
	json["__type"] = "Date";
	json["iso"] = dateTime.toString("yyyy-MM-dd'T'HH:mm:ss.zzz'Z'");
}

bool QParseDate::operator==( QParseDate const& other ) const {
	return (dateTime == other.dateTime);
}

bool QParseDate::operator!=( QParseDate const& other ) const {
	return (dateTime != other.dateTime);
}

QJsonObject QParseDate::toJson() const {
	return json;
}

QDateTime QParseDate::toDateTime() const {
	return dateTime;
}

QString QParseDate::toISO() {
	return json["iso"].toString();
}

QParseFile::QParseFile( QObject* parent )
	: QObject(parent)
	, json()
	, url()
	, name()
	, localUrl()
	, status(NotValid) {
}

QParseFile::QParseFile( const QParseFile& src, QObject* parent )
	: QObject(parent)
	, json(src.json)
	, url(src.url)
	, name(src.name)
	, localUrl(src.localUrl)
	, status(src.status) {
}

QParseFile::QParseFile( QJsonObject fromParse, QObject* parent )
	: QObject(parent)
	, json()
	, url()
	, name()
	, localUrl()
	, status(NotCached) {
	// there are two possible Json format used by PARSE
	// one containing the __type and one not containing the __type
	// so, check only the presence of name and url
	if ( !fromParse.contains("url") || !fromParse.contains("name") ) {
		qDebug() << "FROM PARSE:" << fromParse;
		qDebug("QParseFile - wrong Json format passed to constructor");
		status = NotValid;
		return;
	}
	json = fromParse;
	if ( !json.contains("__type") ) {
		json["__type"] = "File";
	}
	url = QUrl( json["url"].toString() );
	name = json["name"].toString();
	QUrl localCache = QParse::instance()->getCachedUrlOf( url );
	if ( localCache.isLocalFile() ) {
		localUrl = localCache;
		status = Cached;
	}
}

QParseFile::QParseFile( QUrl localFile, QObject* parent )
	: QObject(parent)
	, json()
	, url()
	, name()
	, localUrl()
	, status(ToUpload) {
	// it must be a local file
	if ( !localFile.isLocalFile() ) {
		qFatal("QParseFile - only local file are accepted by the constructor");
	}
	// it create a local and invalid representation for PARSE
	// because it need to be uploaded to PARSE before to get
	// a valid representation of a PARSE file
	json["__type"] = "Invalid";
	url = localFile;
	name = localFile.fileName();
	localUrl = localFile;
}

QUrl QParseFile::getUrl() const {
	return url;
}

QString QParseFile::getName() const {
	return name;
}

QJsonObject QParseFile::toJson() {
	return json;
}

bool QParseFile::isValid() {
	return (status != NotValid);
}

QUrl QParseFile::getLocalUrl() const {
	return localUrl;
}

void QParseFile::setLocalUrl(const QUrl &value) {
	localUrl = value;
	emit localUrlChanged( localUrl );
}

QParseFile::Status QParseFile::getStatus() const {
	return status;
}

void QParseFile::setStatus(const Status &value) {
	status = value;
	emit statusChanged( status );
}

void QParseFile::pull() {
	if ( status == NotCached ) {
		setStatus( Caching );
		QParseRequest* request = new QParseRequest(this);
		QParseReply* reply = QParse::instance()->get( request );
		connect( reply, &QParseReply::finished, [this](QParseReply* reply) {
			setLocalUrl( reply->getLocalUrl() );
			setStatus( Cached );
			emit cached( localUrl );
		});
	}
	if ( status == Cached ) {
		// directly emit to notify who rely on this signal
		emit cached( localUrl );
	}
}
