/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

package org.alohastats.lib;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

import java.io.File;
import java.util.Map;
import java.util.UUID;

public class Statistics {

  static public void setup(final String serverUrl, Activity activity) {
    String storagePath = activity.getFilesDir().getAbsolutePath();
    if (!storagePath.endsWith("/"))
      storagePath += "/";
    // Native code expects valid existing writable dir.
    (new File(storagePath)).mkdirs();
    setupCPP(HttpTransport.class, serverUrl, storagePath, getInstallationId(activity));
  }

  native static public void logEvent(String eventName);

  native static public void logEvent(String eventName, String eventValue);

  // eventDictionary is a key,value,key,value array.
  native static public void logEvent(String eventName, String[] eventDictionary);

  static public void logEvent(String eventName, Map<String, String> eventDictionary) {
    // For faster native processing pass array of strings instead of a map.
    final String[] array = new String[eventDictionary.size() * 2];
    int index = 0;
    for (final Map.Entry<String, String> entry : eventDictionary.entrySet()) {
      array[index++] = entry.getKey();
      array[index++] = entry.getValue();
    }
    logEvent(eventName, array);
  }

  static public void onResume(Activity activity) {
    logEvent("$onResume", activity.getLocalClassName());
  }

  static public void onPause(Activity activity) {
    logEvent("$onPause", activity.getLocalClassName());
  }

  // http://stackoverflow.com/a/7929810
  private static String uniqueID = null;
  private static final String PREF_UNIQUE_ID = "ALOHALYTICS_UNIQUE_ID";

  public synchronized static String getInstallationId(final Context context) {
    if (uniqueID == null) {
      final SharedPreferences sharedPrefs = context.getSharedPreferences(
          PREF_UNIQUE_ID, Context.MODE_PRIVATE);
      uniqueID = sharedPrefs.getString(PREF_UNIQUE_ID, null);
      if (uniqueID == null) {
        // "A:" means Android. It will be different for other platforms, for convenience debugging.
        uniqueID = "A:" + UUID.randomUUID().toString();
        final SharedPreferences.Editor editor = sharedPrefs.edit();
        editor.putString(PREF_UNIQUE_ID, uniqueID);
        editor.apply();
      }
    }
    return uniqueID;
  }

  // Initialize internal C++ statistics engine.
  native static private void setupCPP(final Class httpTransportClass,
                                      final String serverUrl,
                                      final String storagePath,
                                      final String installationId);

  static {
    System.loadLibrary("alohastats");
  }

}
