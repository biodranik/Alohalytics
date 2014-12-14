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

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import android.provider.Settings.Global;
import android.provider.Settings.System;
import android.provider.Settings.Secure;
import android.util.DisplayMetrics;
import com.google.android.gms.ads.identifier.AdvertisingIdClient;

interface KeyValueStorage
{
  public void set(String key, String value);
  public void set(String key, float value);
  public void set(String key, boolean value);
  public void set(String key, int value);
}

public class SystemInfo
{
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
      kvs.set(Global.AIRPLANE_MODE_ON, Global.getString(cr, Global.AIRPLANE_MODE_ON)); // 1 or 0
      kvs.set(Global.ALWAYS_FINISH_ACTIVITIES, Global.getString(cr, Global.ALWAYS_FINISH_ACTIVITIES)); // 1 or 0
      kvs.set(Global.AUTO_TIME, Global.getString(cr, Global.AUTO_TIME)); // 1 or 0
      kvs.set(Global.AUTO_TIME_ZONE, Global.getString(cr, Global.AUTO_TIME_ZONE)); // 1 or 0
      kvs.set(Global.BLUETOOTH_ON, Global.getString(cr, Global.BLUETOOTH_ON)); // 1 or 0
      kvs.set(Global.DATA_ROAMING, Global.getString(cr, Global.DATA_ROAMING));  // 1 or 0
      kvs.set(Global.HTTP_PROXY, Global.getString(cr, Global.HTTP_PROXY));  // host:port
    }
    else
    {
      kvs.set(System.AIRPLANE_MODE_ON, System.getString(cr, System.AIRPLANE_MODE_ON));
      kvs.set(System.ALWAYS_FINISH_ACTIVITIES, System.getString(cr, System.ALWAYS_FINISH_ACTIVITIES));
      kvs.set(System.AUTO_TIME, System.getString(cr, System.AUTO_TIME));
      kvs.set(System.AUTO_TIME_ZONE, System.getString(cr, System.AUTO_TIME_ZONE));
      kvs.set(Secure.BLUETOOTH_ON, Secure.getString(cr, Secure.BLUETOOTH_ON));
      kvs.set(System.DATA_ROAMING, System.getString(cr, System.DATA_ROAMING));
      kvs.set(Secure.HTTP_PROXY, Secure.getString(cr, Secure.HTTP_PROXY));
    }

    kvs.set(Secure.ACCESSIBILITY_ENABLED, Secure.getString(cr, Secure.ACCESSIBILITY_ENABLED)); // 1 or 0
    kvs.set(Secure.INSTALL_NON_MARKET_APPS, Secure.getString(cr, Secure.INSTALL_NON_MARKET_APPS)); // 1 or 0

    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.JELLY_BEAN)
      kvs.set(Secure.DEVELOPMENT_SETTINGS_ENABLED, Secure.getString(cr, Secure.DEVELOPMENT_SETTINGS_ENABLED)); // 1 or 0
    else if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN)
      kvs.set(Global.DEVELOPMENT_SETTINGS_ENABLED, Global.getString(cr, Global.DEVELOPMENT_SETTINGS_ENABLED));

    kvs.set(System.DATE_FORMAT, System.getString(cr, System.DATE_FORMAT)); // dd/mm/yyyy
    kvs.set(System.SCREEN_OFF_TIMEOUT, System.getString(cr, System.SCREEN_OFF_TIMEOUT)); // milliseconds
    kvs.set(System.TIME_12_24, System.getString(cr, System.TIME_12_24)); // 12 or 24

    kvs.set(Secure.ALLOW_MOCK_LOCATION, Secure.getString(cr, Secure.ALLOW_MOCK_LOCATION)); // 1 or 0
    kvs.set(Secure.ANDROID_ID, Secure.getString(cr, Secure.ANDROID_ID));

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
      kvs.set(Secure.LOCATION_MODE, Secure.getString(cr, Secure.LOCATION_MODE)); // Int values 0 - 3
  }

  static public void tryToGetAdvertisingId(final Context context, final KeyValueStorage akv)
  {
    // Collect Google Play Advertising ID
    new Thread(new Runnable()
    {
      @Override
      public void run()
      {
        // See sample code at http://developer.android.com/google/play-services/id.html
        try
        {
          AdvertisingIdClient.Info adInfo = AdvertisingIdClient.getAdvertisingIdInfo(context.getApplicationContext());
          akv.set("GoogleAdvertisingId", adInfo.getId());
          akv.set("isLimitAdTrackingEnabled", adInfo.isLimitAdTrackingEnabled());
        } catch (java.lang.Exception e)
        {
          akv.set("GoogleAdvertisingId", null);
        }
      }
    }).start();
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