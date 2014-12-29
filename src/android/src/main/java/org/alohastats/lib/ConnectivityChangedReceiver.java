package org.alohastats.lib;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;

public class ConnectivityChangedReceiver extends BroadcastReceiver {
  @Override
  public void onReceive(Context context, Intent intent) {
    final NetworkInfo networkInfo = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
    if (networkInfo != null
        && networkInfo.getType() == ConnectivityManager.TYPE_WIFI
        && networkInfo.isConnected()) {
      onWiFiConnected();
    }
  }

  public void onWiFiConnected() {
    org.alohastats.lib.Statistics.forceUpload();
  }
}
