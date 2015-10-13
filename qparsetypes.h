#ifndef QPARSETYPES_H
#define QPARSETYPES_H

#include <QJsonObject>
#include <QDateTime>

//! \file This file contains the classes for handle easily the PARSE data types

//! The Date type on PARSE
class QParseDate {
public:
	//! construct a QParseDate from the Json PARSE representation
	QParseDate( QJsonObject fromParse );
	//! construct a QParseDate from a QDateTime object
	QParseDate( QDateTime fromDateTime );
	//! export into Json PARSE representation
	QJsonObject toJson();
	//! export into QDateTime
	QDateTime toDateTime();
private:
	//! iso representation for PARSE
	QString iso;
	//! QDateTime representation
	QDateTime dateTime;
};

#endif // QPARSETYPES_H
