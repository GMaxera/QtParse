# QtParse

## Setting PARSE Installation on Android
For using the Installation class (and notification) from Android, you must first setup the GCM service on the app:
This involve some steps that are listed into this page: https://developers.google.com/cloud-messaging/android/client

* Create an Android source directory with gradle files
* "Get a Configuration File" for GCM and put into the Android source directory; During this step keep not of the senderId and ApiKey for using the GCM service
* Set up the gradle file as indicated into the page: https://developers.google.com/cloud-messaging/android/client
* Set up the AndroidManifest.xml as indicated in https://developers.google.com/cloud-messaging/android/client


