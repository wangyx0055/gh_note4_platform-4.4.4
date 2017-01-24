// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file is autogenerated by
//     base/android/jni_generator/jni_generator.py
// For
//     com/sec/chromium/chrome/browser/SbrWebNotificationUIManager

#ifndef com_sec_chromium_chrome_browser_SbrWebNotificationUIManager_JNI
#define com_sec_chromium_chrome_browser_SbrWebNotificationUIManager_JNI

#include <jni.h>

#include "base/android/jni_generator/jni_generator_helper.h"

// Step 1: forward declarations.
namespace {
const char kSbrWebNotificationUIManagerClassPath[] =
    "com/sec/chromium/chrome/browser/SbrWebNotificationUIManager";
// Leaking this jclass as we cannot use LazyInstance from some threads.
jclass g_SbrWebNotificationUIManager_clazz = NULL;

}  // namespace

// Step 2: method stubs.

static base::subtle::AtomicWord g_SbrWebNotificationUIManager_CreateNotification
    = 0;
static void Java_SbrWebNotificationUIManager_CreateNotification(JNIEnv* env,
    jstring iconUrl,
    jstring title,
    jstring body) {
  /* Must call RegisterNativesImpl()  */
  CHECK_CLAZZ(env, g_SbrWebNotificationUIManager_clazz,
      g_SbrWebNotificationUIManager_clazz);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, g_SbrWebNotificationUIManager_clazz,
      "CreateNotification",

"("
"Ljava/lang/String;"
"Ljava/lang/String;"
"Ljava/lang/String;"
")"
"V",
      &g_SbrWebNotificationUIManager_CreateNotification);

     env->CallStaticVoidMethod(g_SbrWebNotificationUIManager_clazz,
          method_id, iconUrl, title, body);
  jni_generator::CheckException(env);

}

static base::subtle::AtomicWord g_SbrWebNotificationUIManager_ShowNotification =
    0;
static void Java_SbrWebNotificationUIManager_ShowNotification(JNIEnv* env) {
  /* Must call RegisterNativesImpl()  */
  CHECK_CLAZZ(env, g_SbrWebNotificationUIManager_clazz,
      g_SbrWebNotificationUIManager_clazz);
  jmethodID method_id =
      base::android::MethodID::LazyGet<
      base::android::MethodID::TYPE_STATIC>(
      env, g_SbrWebNotificationUIManager_clazz,
      "ShowNotification",

"("
")"
"V",
      &g_SbrWebNotificationUIManager_ShowNotification);

     env->CallStaticVoidMethod(g_SbrWebNotificationUIManager_clazz,
          method_id);
  jni_generator::CheckException(env);

}

// Step 3: RegisterNatives.

static bool RegisterNativesImpl(JNIEnv* env) {
  g_SbrWebNotificationUIManager_clazz =
      reinterpret_cast<jclass>(env->NewGlobalRef(
      base::android::GetClass(env,
          kSbrWebNotificationUIManagerClassPath).obj()));

  return true;
}

#endif  // com_sec_chromium_chrome_browser_SbrWebNotificationUIManager_JNI