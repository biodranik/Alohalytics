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

import java.util.Map;

public class Statistics {
  static {
    System.loadLibrary("alohastats");
  }
  // Initialize C++ engine
  native static private void setupCPP(final Class httpTransportClass, final String serverUrl, final String storagePath);

  static public void setup(final String serverUrl, String storagePath) {
    if (!storagePath.endsWith("/"))
      storagePath += "/";
    setupCPP(HttpTransport.class, serverUrl, storagePath);
  }

  native static public void logEvent(String eventName);

  native static public void logEvent(String eventName, String eventValue);
  // eventDictionary is a key,value,key,value array.
  native static public void logEvent(String eventName, String[] eventDictionary);

  static public void logEvent(String eventName, Map<String,String> eventDictionary) {
    // For faster native processing pass array of strings instead of a map.
    final String[] array = new String[eventDictionary.size() * 2];
    int index = 0;
    for (final Map.Entry<String, String> entry : eventDictionary.entrySet()) {
      array[index++] = entry.getKey();
      array[index++] = entry.getValue();
    }
    logEvent(eventName, array);
  }
}
