/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

#include <cassert>
#include <jni.h>
#include <memory>
#include <stdexcept>
#include <string>

#include "../../alohalytics.h"
#include "../../http_client.h"
#include "../../logger.h"

using std::string;
using std::unique_ptr;

using namespace alohalytics;

// Implemented in jni_main.cc, you can use your own impl if necessary.
extern JavaVM * GetJVM();

namespace {

static constexpr double kDefaultAndroidVerticalAccuracy = 0.0;

template <typename POINTER, typename DELETER>
unique_ptr<POINTER, DELETER> MakePointerScopeGuard(POINTER * x, DELETER t) {
  return unique_ptr<POINTER, DELETER>(x, t);
}

template <typename F>
class ScopeGuard final {
  F f_;
  ScopeGuard(const ScopeGuard &) = delete;
  void operator=(const ScopeGuard &) = delete;

 public:
  explicit ScopeGuard(const F & f) : f_(f) {}
  ScopeGuard(ScopeGuard && other) : f_(std::move(other.f_)) {}
  ~ScopeGuard() { f_(); }
};

template <typename F>
ScopeGuard<F> MakeScopeGuard(F f) {
  return ScopeGuard<F>(f);
}

// Cached class and methods for faster access from native code
static jclass g_httpTransportClass = 0;
static jmethodID g_httpTransportClass_run = 0;
static jclass g_httpParamsClass = 0;
static jmethodID g_httpParamsConstructor = 0;

// JNI helper, returns empty string if str == 0.
string MakeString(JNIEnv * env, jstring jstr) {
  string result;
  if (jstr) {
    char const * utfBuffer = env->GetStringUTFChars(jstr, 0);
    if (utfBuffer) {
      result = utfBuffer;
      env->ReleaseStringUTFChars(jstr, utfBuffer);
    }
  }
  return result;
}

// keyValuePairs can be null!
TStringMap FillMapHelper(JNIEnv * env, jobjectArray keyValuePairs) {
  TStringMap map;
  if (keyValuePairs) {
    const jsize count = env->GetArrayLength(keyValuePairs);
    string key;
    for (jsize i = 0; i < count; ++i) {
      const jstring jni_string = static_cast<jstring>(env->GetObjectArrayElement(keyValuePairs, i));
      if ((i + 1) % 2) {
        key = MakeString(env, jni_string);
        map[key] = "";
      } else {
        map[key] = MakeString(env, jni_string);
      }
      if (jni_string) {
        env->DeleteLocalRef(jni_string);
      }
    }
  }
  return map;
}

class ScopedJNIEnv final {
  bool auto_detach_;
  JNIEnv * env_;
  JavaVM * vm_;
  ScopedJNIEnv(bool auto_detach, JNIEnv * env, JavaVM * vm)
    : auto_detach_(auto_detach), env_(env), vm_(vm) {}
public:
  JNIEnv * operator->() { return env_; }

  ~ScopedJNIEnv() {
    if (auto_detach_) {
      vm_->DetachCurrentThread();
    }
  }

  static ScopedJNIEnv GetJNIEnv(JavaVM * vm) {
    JNIEnv * env;
    switch (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6))
    {
      case JNI_EVERSION: throw std::runtime_error("ERROR JNI version is not supported.");
      case JNI_EDETACHED: {
        if (JNI_OK == vm->AttachCurrentThread(&env, nullptr)) {
          return ScopedJNIEnv(true, env, vm);
        }
        throw std::runtime_error("ERROR while trying to attach JNI thread.");
      }
    }
    // case JNI_OK:
    return ScopedJNIEnv(false, env, vm);
  }

  inline string ToStdString(jstring jstr) {
    return MakeString(env_, jstr);
  }
};

}  // namespace

extern "C" {
JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_logEvent__Ljava_lang_String_2(JNIEnv * env,
                                                                                     jclass,
                                                                                     jstring eventName) {
  LogEvent(MakeString(env, eventName));
}

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_logEvent__Ljava_lang_String_2Ljava_lang_String_2(
    JNIEnv * env, jclass, jstring eventName, jstring eventValue) {
  LogEvent(MakeString(env, eventName), MakeString(env, eventValue));
}

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_logEvent__Ljava_lang_String_2_3Ljava_lang_String_2(
    JNIEnv * env, jclass, jstring eventName, jobjectArray keyValuePairs) {
  LogEvent(MakeString(env, eventName), FillMapHelper(env, keyValuePairs));
}

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_logEvent__Ljava_lang_String_2_3Ljava_lang_String_2ZJDDFZDZFZFB(
    JNIEnv * env,
    jclass,
    jstring eventName,
    jobjectArray keyValuePairs,
    jboolean hasLatLon,
    jlong timestamp,
    jdouble lat,
    jdouble lon,
    jfloat accuracy,
    jboolean hasAltitude,
    jdouble altitude,
    jboolean hasBearing,
    jfloat bearing,
    jboolean hasSpeed,
    jfloat speed,
    jbyte source) {
  alohalytics::Location l;
  if (hasLatLon) {
    l.SetLatLon(timestamp, lat, lon, accuracy);
    l.SetSource((alohalytics::Location::Source)source);
  }
  if (hasAltitude) {
    l.SetAltitude(altitude, kDefaultAndroidVerticalAccuracy);
  }
  if (hasBearing) {
    l.SetBearing(bearing);
  }
  if (hasSpeed) {
    l.SetSpeed(speed);
  }

  LogEvent(MakeString(env, eventName), FillMapHelper(env, keyValuePairs), l);
}

#define CLEAR_AND_RETURN_FALSE_ON_EXCEPTION \
  if (env->ExceptionCheck()) {              \
    env->ExceptionDescribe();               \
    env->ExceptionClear();                  \
    return false;                           \
  }

#define RETURN_ON_EXCEPTION    \
  if (env->ExceptionCheck()) { \
    env->ExceptionDescribe();  \
    return;                    \
  }

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_setupCPP(
    JNIEnv * env, jclass, jclass httpTransportClass, jstring serverUrl, jstring storagePath, jstring installationId) {
  g_httpTransportClass = static_cast<jclass>(env->NewGlobalRef(httpTransportClass));
  RETURN_ON_EXCEPTION
  g_httpTransportClass_run = env->GetStaticMethodID(
      g_httpTransportClass, "run", "(Lorg/alohalytics/HttpTransport$Params;)Lorg/alohalytics/HttpTransport$Params;");
  RETURN_ON_EXCEPTION
  g_httpParamsClass = env->FindClass("org/alohalytics/HttpTransport$Params");
  RETURN_ON_EXCEPTION
  g_httpParamsClass = static_cast<jclass>(env->NewGlobalRef(g_httpParamsClass));
  RETURN_ON_EXCEPTION
  g_httpParamsConstructor = env->GetMethodID(g_httpParamsClass, "<init>", "(Ljava/lang/String;)V");
  RETURN_ON_EXCEPTION

  // Initialize statistics engine at the end of setup function, as it can use globals above.
  Stats::Instance()
      .SetClientId(MakeString(env, installationId))
      .SetServerUrl(MakeString(env, serverUrl))
      .SetStoragePath(MakeString(env, storagePath));
}

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_debugCPP(JNIEnv * env, jclass, jboolean enableDebug) {
  Stats::Instance().SetDebugMode(enableDebug);
}

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_forceUpload(JNIEnv * env, jclass) { Stats::Instance().Upload(); }

JNIEXPORT void JNICALL Java_org_alohalytics_Statistics_enableCPP(JNIEnv * env, jclass, jboolean enable) {
  Stats & s = Stats::Instance();
  if (enable) {
    s.Enable();
    s.LogEvent("$statisticsEnabled");
  } else {
    s.LogEvent("$statisticsDisabled");
    s.Upload();
    s.Disable();
  }
}

}  // extern "C"

//***********************************************************************
// Exported functions implementation
//***********************************************************************
namespace alohalytics {

bool HTTPClientPlatformWrapper::RunHTTPRequest() {
  auto env = ScopedJNIEnv::GetJNIEnv(::GetJVM());

  // Convenience lambda.
  const auto deleteLocalRef = [&env](jobject o) { env->DeleteLocalRef(o); };

  // Create and fill request params.
  const auto jniUrl = MakePointerScopeGuard(env->NewStringUTF(url_requested_.c_str()), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  const auto httpParamsObject =
      MakePointerScopeGuard(env->NewObject(g_httpParamsClass, g_httpParamsConstructor, jniUrl.get()), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  // Cache it on the first call.
  const static jfieldID dataField = env->GetFieldID(g_httpParamsClass, "data", "[B");
  if (!body_data_.empty()) {
    const auto jniPostData = MakePointerScopeGuard(env->NewByteArray(body_data_.size()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetByteArrayRegion(jniPostData.get(), 0, body_data_.size(),
                            reinterpret_cast<const jbyte *>(body_data_.data()));
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), dataField, jniPostData.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  assert(!http_method_.empty());
  const static jfieldID httpMethodField = env->GetFieldID(g_httpParamsClass, "httpMethod", "Ljava/lang/String;");
  {
    const auto jniHttpMethod = MakePointerScopeGuard(env->NewStringUTF(http_method_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), httpMethodField, jniHttpMethod.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  const static jfieldID contentTypeField = env->GetFieldID(g_httpParamsClass, "contentType", "Ljava/lang/String;");
  if (!content_type_.empty()) {
    const auto jniContentType = MakePointerScopeGuard(env->NewStringUTF(content_type_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), contentTypeField, jniContentType.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  const static jfieldID contentEncodingField =
      env->GetFieldID(g_httpParamsClass, "contentEncoding", "Ljava/lang/String;");
  if (!content_encoding_.empty()) {
    const auto jniContentEncoding = MakePointerScopeGuard(env->NewStringUTF(content_encoding_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), contentEncodingField, jniContentEncoding.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!user_agent_.empty()) {
    const static jfieldID userAgentField = env->GetFieldID(g_httpParamsClass, "userAgent", "Ljava/lang/String;");

    const auto jniUserAgent = MakePointerScopeGuard(env->NewStringUTF(user_agent_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), userAgentField, jniUserAgent.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!body_file_.empty()) {
    const static jfieldID inputFilePathField =
        env->GetFieldID(g_httpParamsClass, "inputFilePath", "Ljava/lang/String;");

    const auto jniInputFilePath = MakePointerScopeGuard(env->NewStringUTF(body_file_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), inputFilePathField, jniInputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!received_file_.empty()) {
    const static jfieldID outputFilePathField =
        env->GetFieldID(g_httpParamsClass, "outputFilePath", "Ljava/lang/String;");

    const auto jniOutputFilePath = MakePointerScopeGuard(env->NewStringUTF(received_file_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), outputFilePathField, jniOutputFilePath.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!basic_auth_user_.empty()) {
    const static jfieldID basicAuthUserField =
        env->GetFieldID(g_httpParamsClass, "basicAuthUser", "Ljava/lang/String;");

    const auto jniBasicAuthUser = MakePointerScopeGuard(env->NewStringUTF(basic_auth_user_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), basicAuthUserField, jniBasicAuthUser.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  if (!basic_auth_password_.empty()) {
    const static jfieldID basicAuthPasswordField =
        env->GetFieldID(g_httpParamsClass, "basicAuthPassword", "Ljava/lang/String;");

    const auto jniBasicAuthPassword =
        MakePointerScopeGuard(env->NewStringUTF(basic_auth_password_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), basicAuthPasswordField, jniBasicAuthPassword.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  const static jfieldID cookiesField = env->GetFieldID(g_httpParamsClass, "cookies", "Ljava/lang/String;");
  if (!cookies_.empty()) {
    const auto jniCookies = MakePointerScopeGuard(env->NewStringUTF(cookies_.c_str()), deleteLocalRef);
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

    env->SetObjectField(httpParamsObject.get(), cookiesField, jniCookies.get());
    CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  }

  const static jfieldID debugModeField = env->GetFieldID(g_httpParamsClass, "debugMode", "Z");
  env->SetBooleanField(httpParamsObject.get(), debugModeField, debug_mode_);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  // DO ALL MAGIC!
  // Current Java implementation simply reuses input params instance, so we don't need to
  // DeleteLocalRef(response).
  const jobject response =
      env->CallStaticObjectMethod(g_httpTransportClass, g_httpTransportClass_run, httpParamsObject.get());
  if (env->ExceptionCheck()) {
    env->ExceptionDescribe();
    // TODO(AlexZ): think about rethrowing corresponding C++ exceptions.
    env->ExceptionClear();
    return false;
  }

  const static jfieldID httpResponseCodeField = env->GetFieldID(g_httpParamsClass, "httpResponseCode", "I");
  error_code_ = env->GetIntField(response, httpResponseCodeField);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION

  const static jfieldID receivedUrlField = env->GetFieldID(g_httpParamsClass, "receivedUrl", "Ljava/lang/String;");
  const auto jniReceivedUrl =
      MakePointerScopeGuard(static_cast<jstring>(env->GetObjectField(response, receivedUrlField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniReceivedUrl) {
    url_received_ = env.ToStdString(jniReceivedUrl.get());
  }

  // contentTypeField is already cached above.
  const auto jniContentType =
      MakePointerScopeGuard(static_cast<jstring>(env->GetObjectField(response, contentTypeField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniContentType) {
    content_type_received_ = env.ToStdString(jniContentType.get());
  }

  // contentEncodingField is already cached above.
  const auto jniContentEncoding =
      MakePointerScopeGuard(static_cast<jstring>(env->GetObjectField(response, contentEncodingField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniContentEncoding) {
    content_encoding_received_ = env.ToStdString(jniContentEncoding.get());
  }

  // Note: cookies field is used not only to send Cookie header, but also to receive back Server-Cookie header.
  // cookiesField is already cached above.
  const auto jniServerCookies =
      MakePointerScopeGuard(static_cast<jstring>(env->GetObjectField(response, cookiesField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniServerCookies) {
    server_cookies_ = normalize_server_cookies(std::move(env.ToStdString(jniServerCookies.get())));
  }

  // dataField is already cached above.
  const auto jniData =
      MakePointerScopeGuard(static_cast<jbyteArray>(env->GetObjectField(response, dataField)), deleteLocalRef);
  CLEAR_AND_RETURN_FALSE_ON_EXCEPTION
  if (jniData) {
    jbyte * buffer = env->GetByteArrayElements(jniData.get(), nullptr);
    if (buffer) {
      server_response_.assign(reinterpret_cast<const char *>(buffer), env->GetArrayLength(jniData.get()));
      env->ReleaseByteArrayElements(jniData.get(), buffer, JNI_ABORT);
    }
  }
  return true;
}

}  // namespace alohalytics
