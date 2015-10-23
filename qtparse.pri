
QT += concurrent

INCLUDEPATH += $$PWD

SOURCES += \
	$$PWD/qparsetypes.cpp \
	$$PWD/qparse.cpp \
	$$PWD/qparseobject.cpp \
	$$PWD/qparseuser.cpp \
	$$PWD/qparserequest.cpp \
	$$PWD/qparsereply.cpp \
	$$PWD/qparsequery.cpp

HEADERS += \
	$$PWD/qparsetypes.h \
	$$PWD/qparse.h \
	$$PWD/qparseobject.h \
	$$PWD/qparseuser.h \
	$$PWD/qparserequest.h \
	$$PWD/qparsereply.h \
	$$PWD/qparsequery.h

android {
	QT += androidextras
	SOURCES += $$PWD/qparse_android.cpp
} else:ios {
	## the objective sources should be put in this variable
	OBJECTIVE_SOURCES += \
		$$PWD/qparse_ios.mm
}
