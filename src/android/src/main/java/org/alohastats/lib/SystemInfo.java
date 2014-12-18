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
import android.content.ContentResolver;
import android.content.res.Configuration;
import android.os.Build;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;
import android.util.Log;

import com.google.android.gms.ads.identifier.AdvertisingIdClient;

import org.json.JSONException;
import org.json.JSONObject;

import static android.provider.Settings.Secure;

interface KeyValueStorage
{
  public void set(String key, String value);
  public void set(String key, float value);
  public void set(String key, boolean value);
  public void set(String key, int value);
}

public class SystemInfo
{
  private static final String TAG = "AlohaLytics.SystemInfo";

  private static void handleException(Exception ex) {
    if (Statistics.debugMode()) {
      if (ex.getMessage() != null) {
        Log.w(TAG, ex.getMessage());
      }
      ex.printStackTrace();
    }
  }

  public static void getDeviceInfoAsync(final Activity activity) {
    // Collect all information on a separate thread, because:
    // - Google Advertising ID should be requested in a separate thread.
    // - Do not block UI thread while querying many properties.
    // Collect Google Play Advertising ID
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        collectIds(activity);
      }
    }).start();
  }

  private static void collectIds(final Activity activity) {
    // Retrieve GoogleAdvertisingId.
    // See sample code at http://developer.android.com/google/play-services/id.html
    String google_advertising_id = null;
    try
    {
      google_advertising_id = AdvertisingIdClient.getAdvertisingIdInfo(activity.getApplicationContext()).getId();
    } catch (Exception ex) {
      handleException(ex);
    }

    String android_id = null;
    try {
      android_id = Settings.Secure.getString(activity.getContentResolver(), Settings.Secure.ANDROID_ID);
      // This is a known bug workaround - https://code.google.com/p/android/issues/detail?id=10603
      if (android_id.equalsIgnoreCase("9774d56d682e549c")) {
        android_id = null;
      }
    } catch (Exception ex) {
      handleException(ex);
    }

    String device_id = null;
    String sim_serial_number = null;
    try
    {
      // This code works only if the app has READ_PHONE_STATE permission.
      final TelephonyManager tm = (TelephonyManager) activity.getBaseContext().getSystemService(activity.TELEPHONY_SERVICE);
      device_id = tm.getDeviceId();
      sim_serial_number = tm.getSimSerialNumber();
    } catch (Exception ex) {
      handleException(ex);
    }

    // This androidInfo object should match into C++ AndroidIds class.
    final JSONObject androidIds = new JSONObject();
    try {
      androidIds.put("google_advertising_id", emptyIfNull(google_advertising_id));
      androidIds.put("android_id", emptyIfNull(android_id));
      androidIds.put("device_id", emptyIfNull(device_id));
      androidIds.put("sim_serial_number", emptyIfNull(sim_serial_number));
      Statistics.logJSONEvent(androidIds);
    } catch (JSONException ex) {
      handleException(ex);
    }
  }

  public static String emptyIfNull(final String str) {
    return str == null ? "" : str;
  }

  public void getDeviceDetails(android.app.Activity activity, KeyValueStorage kvs)
  {
    final DisplayMetrics metrics = new DisplayMetrics();
    activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);
    kvs.set("display_density", metrics.density);
    kvs.set("display_density_dpi", metrics.densityDpi);
    kvs.set("display_scaled_density", metrics.scaledDensity);
    kvs.set("display_width_pixels", metrics.widthPixels);
    kvs.set("display_height_pixels", metrics.heightPixels);
    kvs.set("display_xdpi", metrics.xdpi);
    kvs.set("display_ydpi", metrics.ydpi);

    final Configuration config = activity.getResources().getConfiguration();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
      kvs.set("dpi", config.densityDpi); // int value

    kvs.set("font_scale", config.fontScale);
    kvs.set("locale_country", config.locale.getCountry());
    kvs.set("locale_language", config.locale.getLanguage());
    kvs.set("locale_variant", config.locale.getVariant());

    if (config.mcc != 0)
      kvs.set("mcc", config.mcc);
    if (config.mnc != 0)
      kvs.set("mnc", config.mnc == Configuration.MNC_ZERO ? 0 : config.mnc);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2)
    {
      kvs.set("screen_width_dp", config.screenWidthDp);
      kvs.set("screen_height_dp", config.screenHeightDp);
    }

    final ContentResolver cr = activity.getContentResolver();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
    {
      kvs.set(Settings.Global.AIRPLANE_MODE_ON, Settings.Global.getString(cr, Settings.Global.AIRPLANE_MODE_ON)); // 1 or 0
      kvs.set(Settings.Global.ALWAYS_FINISH_ACTIVITIES, Settings.Global.getString(cr, Settings.Global.ALWAYS_FINISH_ACTIVITIES)); // 1 or 0
      kvs.set(Settings.Global.AUTO_TIME, Settings.Global.getString(cr, Settings.Global.AUTO_TIME)); // 1 or 0
      kvs.set(Settings.Global.AUTO_TIME_ZONE, Settings.Global.getString(cr, Settings.Global.AUTO_TIME_ZONE)); // 1 or 0
      kvs.set(Settings.Global.BLUETOOTH_ON, Settings.Global.getString(cr, Settings.Global.BLUETOOTH_ON)); // 1 or 0
      kvs.set(Settings.Global.DATA_ROAMING, Settings.Global.getString(cr, Settings.Global.DATA_ROAMING));  // 1 or 0
      kvs.set(Settings.Global.HTTP_PROXY, Settings.Global.getString(cr, Settings.Global.HTTP_PROXY));  // host:port
    }
    else
    {
      kvs.set(Settings.System.AIRPLANE_MODE_ON, Settings.System.getString(cr, Settings.System.AIRPLANE_MODE_ON));
      kvs.set(Settings.System.ALWAYS_FINISH_ACTIVITIES, Settings.System.getString(cr, Settings.System.ALWAYS_FINISH_ACTIVITIES));
      kvs.set(Settings.System.AUTO_TIME, Settings.System.getString(cr, Settings.System.AUTO_TIME));
      kvs.set(Settings.System.AUTO_TIME_ZONE, Settings.System.getString(cr, Settings.System.AUTO_TIME_ZONE));
      kvs.set(Settings.Secure.BLUETOOTH_ON, Settings.Secure.getString(cr, Settings.Secure.BLUETOOTH_ON));
      kvs.set(Settings.System.DATA_ROAMING, Settings.System.getString(cr, Settings.System.DATA_ROAMING));
      kvs.set(Settings.Secure.HTTP_PROXY, Settings.Secure.getString(cr, Settings.Secure.HTTP_PROXY));
    }

    kvs.set(Settings.Secure.ACCESSIBILITY_ENABLED, Settings.Secure.getString(cr, Settings.Secure.ACCESSIBILITY_ENABLED)); // 1 or 0
    kvs.set(Settings.Secure.INSTALL_NON_MARKET_APPS, Settings.Secure.getString(cr, Settings.Secure.INSTALL_NON_MARKET_APPS)); // 1 or 0

    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.JELLY_BEAN)
      kvs.set(Settings.Secure.DEVELOPMENT_SETTINGS_ENABLED, Settings.Secure.getString(cr, Settings.Secure.DEVELOPMENT_SETTINGS_ENABLED)); // 1 or 0
    else if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN)
      kvs.set(Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, Settings.Global.getString(cr, Settings.Global.DEVELOPMENT_SETTINGS_ENABLED));

    kvs.set(Settings.System.DATE_FORMAT, Settings.System.getString(cr, Settings.System.DATE_FORMAT)); // dd/mm/yyyy
    kvs.set(Settings.System.SCREEN_OFF_TIMEOUT, Settings.System.getString(cr, Settings.System.SCREEN_OFF_TIMEOUT)); // milliseconds
    kvs.set(Settings.System.TIME_12_24, Settings.System.getString(cr, Settings.System.TIME_12_24)); // 12 or 24

    kvs.set(Settings.Secure.ALLOW_MOCK_LOCATION, Settings.Secure.getString(cr, Settings.Secure.ALLOW_MOCK_LOCATION)); // 1 or 0
    kvs.set(Settings.Secure.ANDROID_ID, Settings.Secure.getString(cr, Settings.Secure.ANDROID_ID));

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
      kvs.set(Settings.Secure.LOCATION_MODE, Settings.Secure.getString(cr, Settings.Secure.LOCATION_MODE)); // Int values 0 - 3
  }

  private void storeIfKnown(String key, String value, KeyValueStorage akv)
  {
    if (!value.equals(Build.UNKNOWN))
      akv.set(key, value);
  }

  public void getBuildParams(KeyValueStorage akv)
  {
    // Most params are never changed, others are changed only after firmware upgrade.
    akv.set("build_version_sdk", Build.VERSION.SDK_INT);
    storeIfKnown("build_brand", Build.BRAND, akv);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
    {
      for (int i = 0; i < Build.SUPPORTED_ABIS.length; ++i)
        storeIfKnown("build_cpu_abi" + (i + 1), Build.SUPPORTED_ABIS[i], akv);
    }
    else
    {
      storeIfKnown("build_cpu_abi1", Build.CPU_ABI, akv);
      storeIfKnown("build_cpu_abi2", Build.CPU_ABI2, akv);
    }
    storeIfKnown("build_device", Build.DEVICE, akv);
    storeIfKnown("build_display", Build.DISPLAY, akv);
    storeIfKnown("build_fingerprint", Build.FINGERPRINT, akv);
    storeIfKnown("build_hardware", Build.HARDWARE, akv);
    storeIfKnown("build_host", Build.HOST, akv);
    storeIfKnown("build_id", Build.ID, akv);
    storeIfKnown("build_manufacturer", Build.MANUFACTURER, akv);
    storeIfKnown("build_model", Build.MODEL, akv);
    storeIfKnown("build_product", Build.PRODUCT, akv);
    storeIfKnown("build_serial", Build.SERIAL, akv);
    storeIfKnown("build_tags", Build.TAGS, akv);
    akv.set("build_time", Build.TIME);
    storeIfKnown("build_type", Build.TYPE, akv);
    storeIfKnown("build_user", Build.USER, akv);
  }
}