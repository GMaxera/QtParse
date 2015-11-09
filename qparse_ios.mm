
#include "qparse.h"
#include <QNetworkAccessManager>
#include <QtConcurrent>
#import "UIKit/UIKit.h"

/*! Override the QIOSApplicationDelegate adding
 *  a category for handling the callbacks for device token for push notifications
 *  The only way to do that even if it's a bit like hacking the Qt stuff
 *  See: https://bugreports.qt-project.org/browse/QTBUG-38184
 */
@interface QIOSApplicationDelegate
@end
//! Add a category to QIOSApplicationDelegate
@interface QIOSApplicationDelegate (QParseApplicationDelegate)
@end
//! Now add method for handling the openURL from Facebook Login
@implementation QIOSApplicationDelegate (QParseApplicationDelegate)
- (void)application:(UIApplication *)app didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
	NSString *token = [[deviceToken description] stringByTrimmingCharactersInSet: [NSCharacterSet characterSetWithCharactersInString:@"<>"]];
	token = [token stringByReplacingOccurrencesOfString:@" " withString:@""];
	NSLog(@"content---%@", token);
	QString qtoken = QString::fromNSString(token);
	QParse::instance()->sendInstallationPostRequest( qtoken );
}
- (void)application:(UIApplication *)app didFailToRegisterForRemoteNotificationsWithError:(NSError *)err {
	NSString *str = [NSString stringWithFormat: @"Error: %@", err];
	NSLog(@"%@",str);
}
@end

void QParse::createInstallation() {
	if ( installation.contains("objectId") ) {
		// Nothing to do !!!
		// IN FUTURE IT SHOULD CHECK IF THE TOKEN IS STILL VALID
		return;
	}

	if ([[UIApplication sharedApplication] respondsToSelector:@selector(registerUserNotificationSettings:)]) {
		UIUserNotificationSettings *settings = [UIUserNotificationSettings settingsForTypes:
				UIUserNotificationTypeBadge |
				UIUserNotificationTypeAlert |
				UIUserNotificationTypeSound
				categories: nil];
		[[UIApplication sharedApplication] registerUserNotificationSettings:settings];
		[[UIApplication sharedApplication] registerForRemoteNotifications];
	} else {
		[[UIApplication sharedApplication] registerForRemoteNotificationTypes:
			UIRemoteNotificationTypeBadge |
			UIRemoteNotificationTypeAlert |
			UIRemoteNotificationTypeSound];
	}
	// the request on PARSE will be done on the applicatio delegate (see above) that call the below method
}

void QParse::sendInstallationPostRequest( QString token ) {
	// Prepare the request
	// direct request, the reply will be handled on onRequestFinished
	QNetworkRequest request(QUrl("https://api.parse.com/1/installations"));
	request.setRawHeader("X-Parse-Application-Id", appId.toLatin1());
	request.setRawHeader("X-Parse-REST-API-Key", restKey.toLatin1());
	request.setRawHeader("Content-Type", "application/json");
	installation["deviceType"] = "ios";
	installation["deviceToken"] = token;
	QJsonDocument jsonDoc(installation);
	net->post( request, jsonDoc.toJson(QJsonDocument::Compact) );
}

