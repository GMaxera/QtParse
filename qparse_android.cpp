
#include "qparse.h"
#include <QNetworkAccessManager>
#include <QtAndroid>
#include <QtAndroidExtras>
#include <QtConcurrent>

void QParse::updateInstallation() {
	QAndroidJniObject activity = QtAndroid::androidActivity();

	// Retrieve the deviceToken
	QAndroidJniObject instanceId = QAndroidJniObject::callStaticObjectMethod(
				"com/google/android/gms/iid/InstanceID",
				"getInstance",
				"(Landroid/content/Context;)Lcom/google/android/gms/iid/InstanceID;", activity.object<jobject>());
	QAndroidJniObject senderId = QAndroidJniObject::fromString(gcmSenderId);
	QAndroidJniObject scope = QAndroidJniObject::fromString("GCM");
	QAndroidJniObject token = instanceId.callObjectMethod(
				"getToken",
				"(Ljava/lang/String;Ljava/lang/String;Landroid/os/Bundle;)Ljava/lang/String;",
				senderId.object<jstring>(), scope.object<jstring>(), NULL);
	deviceToken = token.toString();
	qDebug() << "TOKEN " << deviceToken;

	QAndroidJniEnvironment env;
	if (env->ExceptionCheck()) {
		// Handle exception here.
		env->ExceptionClear();
	}

	// Prepare the request
	// direct request, the reply will be handled on onRequestFinished
	QNetworkRequest request(QUrl("https://api.parse.com/1/installations"));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	request.setRawHeader("Content-Type", "application/json");
	installation["deviceType"] = "android";
	installation["pushType"] = "gcm";
	installation["deviceToken"] = deviceToken;
	installation["GCMSenderId"] = gcmSenderId;
	QJsonDocument jsonDoc(installation);
	net->post( request, jsonDoc.toJson(QJsonDocument::Compact) );
}
