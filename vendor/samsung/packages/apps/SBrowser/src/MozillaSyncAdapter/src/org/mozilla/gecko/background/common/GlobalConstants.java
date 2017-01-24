/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.background.common;

/**
 * Preprocessed class for storing preprocessed values common to all
 * Android services.
 */
public class GlobalConstants {
  public static final String ANDROID_CPU_ARCH = "@ANDROID_CPU_ARCH@";

  // One of 'beta', 'aurora', 'nightly', 'default'.
  // If this is an official build, 'default' means 'release'.
  // Otherwise, it means 'dev'.
  public static final String MOZ_UPDATE_CHANNEL = "@MOZ_UPDATE_CHANNEL@";
//#ifdef MOZ_OFFICIAL_BRANDING
//  public static final boolean MOZ_OFFICIAL_BRANDING = true;
//#else
  public static final boolean MOZ_OFFICIAL_BRANDING = false;
//#endif

  public static final long BUILD_TIMESTAMP_SECONDS = 1L;
  public static final long BUILD_TIMESTAMP_MSEC = 1000L;

  public static final String MOZ_APP_DISPLAYNAME = "Firefox";
  public static final String MOZ_APP_VERSION = "@MOZ_APP_VERSION@";
  public static final String BROWSER_INTENT_PACKAGE = "com.sec.android.app.sbrowser";
  public static final String BROWSER_INTENT_CLASS = BROWSER_INTENT_PACKAGE + ".SBrowserMainActivity";

  /**
   * Bug 800244: this signing-level permission protects broadcast intents that
   * should be received only by the Firefox versions with the given Android
   * package name.
   */
  public static final String PER_ANDROID_PACKAGE_PERMISSION = "@ANDROID_PACKAGE_NAME@.permission.PER_ANDROID_PACKAGE";

  public static final int SHARED_PREFERENCES_MODE = 0;

  // These are used to ask Fennec (via reflection) to send
  // us a pref notification. This avoids us having to guess
  // Fennec's prefs branch and pref name.
  // Eventually Fennec might listen to startup notifications and
  // do this automatically, but this will do for now. See Bug 800244.
  public static String GECKO_PREFERENCES_CLASS = "org.mozilla.gecko.preferences.GeckoPreferences";
  public static String GECKO_BROADCAST_ANNOUNCEMENTS_PREF_METHOD  = "broadcastAnnouncementsPref";
  public static String GECKO_BROADCAST_HEALTHREPORT_UPLOAD_PREF_METHOD  = "broadcastHealthReportUploadPref";
  public static String GECKO_BROADCAST_HEALTHREPORT_PRUNE_METHOD = "broadcastHealthReportPrune";

  // Common time values.
  public static final long MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
  public static final long MILLISECONDS_PER_SIX_MONTHS = 180 * MILLISECONDS_PER_DAY;
}