
#include "qparsetypes.h"

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
		qFatal("QParseDate - wrong Json format passed to constructor");
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

QParseFile::QParseFile()
	: json()
	, url()
	, name() {
}

QParseFile::QParseFile( const QParseFile& src )
	: json(src.json)
	, url(src.url)
	, name(src.name) {
}

QParseFile::QParseFile( QJsonObject fromParse )
	: json()
	, url()
	, name() {
	// there are two possible Json format used by PARSE
	// one containing the __type and one not containing the __type
	// so, check only the presence of name and url
	if ( !fromParse.contains("url") || !fromParse.contains("name") ) {
		qFatal("QParseFile - wrong Json format passed to constructor");
	}
	json = fromParse;
	if ( !json.contains("__type") ) {
		json["__type"] = "File";
	}
	url = QUrl( json["url"].toString() );
	name = json["name"].toString();
}

QParseFile::QParseFile( QUrl localFile )
	: json()
	, url()
	, name() {
	// it must be a local file
	if ( !localFile.isLocalFile() ) {
		qFatal("QParseFile - only local file are accepted by the constructor");
	}
	// it create a local and invalid representation for PARSE
	// because it need to be uploaded to PARSE before to get
	// a valid representation of a PARSE file
	json["__type"] = "Invalid";
	url = localFile;
	name = QString();
}

bool QParseFile::operator==( QParseFile const& other ) const {
	return (url == other.url);
}

bool QParseFile::operator!=( QParseFile const& other ) const {
	return (url != other.url);
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
