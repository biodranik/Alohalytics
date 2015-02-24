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

#if ! __has_feature(objc_arc)
#error This file must be compiled with ARC. Either turn on ARC for the project or use -fobjc-arc flag
#endif

#import "../alohalytics_objc.h"
#include "../alohalytics.h"
#include "../logger.h"

#include <utility> // std::pair
#include <sys/xattr.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURL.h>
#include <TargetConditionals.h> // TARGET_OS_IPHONE

using namespace alohalytics;

namespace {
// Conversion from [possible nil] NSString to std::string.
static std::string ToStdString(NSString * nsString) {
  if (nsString) {
    return std::string([nsString UTF8String]);
  }
  return std::string();
}
// Additional check if object can be represented as a string.
static std::string ToStdStringSafe(id object) {
  if ([object isKindOfClass:[NSString class]]) {
    return ToStdString(object);
  } else if ([object isKindOfClass:[NSObject class]]) {
    return ToStdString(((NSObject *)object).description);
  }
  return "ERROR: Trying to log neither NSString nor NSObject-inherited object.";
}
// Safe conversion from [possible nil] NSDictionary.
static TStringMap ToStringMap(NSDictionary * nsDictionary) {
  TStringMap map;
  for (NSString * key in nsDictionary) {
    map[ToStdString(key)] = ToStdStringSafe([nsDictionary objectForKey:key]);
  }
  return map;
}
// Safe conversion from [possible nil] NSArray.
static TStringMap ToStringMap(NSArray * nsArray) {
  TStringMap map;
  std::string key;
  for (id item in nsArray) {
    if (key.empty()) {
      key = ToStdStringSafe(item);
      map[key] = "";
    } else {
      map[key] = ToStdStringSafe(item);
      key.clear();
    }
  }
  return map;
}
// Safe extraction from [possible nil] CLLocation to alohalytics::Location.
static Location ExtractLocation(CLLocation * l) {
  Location extracted;
  if (l) {
    if (l.horizontalAccuracy >= 0) {
      extracted.SetLatLon([l.timestamp timeIntervalSince1970] * 1000,
                           l.coordinate.latitude, l.coordinate.longitude,
                           l.horizontalAccuracy);
    }
    if (l.verticalAccuracy >= 0) {
      extracted.SetAltitude(l.altitude, l.verticalAccuracy);
    }
    if (l.speed >= 0) {
      extracted.SetSpeed(l.speed);
    }
    if (l.course >= 0) {
      extracted.SetBearing(l.course);
    }
    // We don't know location source on iOS.
  }
  return extracted;
}
} // namespace


@implementation Alohalytics

+ (std::pair<std::string, bool>)installationId {
  bool firstLaunch = false;
  NSUserDefaults * userDataBase = [NSUserDefaults standardUserDefaults];
  NSString * installationId = [userDataBase objectForKey:@"AlohalyticsInstallationId"];
  if (installationId == nil) {
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    // All iOS IDs start with I:
    installationId = [@"I:" stringByAppendingString:(NSString *)CFBridgingRelease(CFUUIDCreateString(kCFAllocatorDefault, uuid))];
    CFRelease(uuid);
    [userDataBase setValue:installationId forKey:@"AlohalyticsInstallationId"];
    [userDataBase synchronize];
    firstLaunch = true;
  }
  return std::make_pair([installationId UTF8String], firstLaunch);
}

+ (NSString *)storagePath {
  // Store files in special directory which is not backed up automatically.
  NSArray * paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  NSString * directory = [[paths firstObject] stringByAppendingString:@"/Alohalytics/"];
  NSFileManager * fm = [NSFileManager defaultManager];
  if (![fm fileExistsAtPath:directory]) {
    if (![fm createDirectoryAtPath:directory withIntermediateDirectories:YES attributes:nil error:nil]) {
      NSLog(@"Alohalytics ERROR: Can't create directory %@.", directory);
    }
#if (TARGET_OS_IPHONE > 0)
    // Disable iCloud backup for storage folder: https://developer.apple.com/library/iOS/qa/qa1719/_index.html
    const std::string storagePath = [directory UTF8String];
    if (NSFoundationVersionNumber >= NSFoundationVersionNumber_iOS_5_1) {
      CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                             reinterpret_cast<unsigned char const *>(storagePath.c_str()),
                                                             storagePath.size(),
                                                             0);
      CFErrorRef err;
      signed char valueRaw = 1; // BOOL YES
      CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberCharType, &valueRaw);
      if (!CFURLSetResourcePropertyForKey(url, kCFURLIsExcludedFromBackupKey, value, &err)) {
        NSLog(@"Alohalytics ERROR while disabling iCloud backup for directory %@", directory);
      }
      CFRelease(value);
      CFRelease(url);
    } else {
      static char const * attrName = "com.apple.MobileBackup";
      u_int8_t attrValue = 1;
      const int result = ::setxattr(storagePath.c_str(), attrName, &attrValue, sizeof(attrValue), 0, 0);
      if (result != 0) {
        NSLog(@"Alohalytics ERROR while disabling iCloud backup for directory %@", directory);
      }
    }
#endif // TARGET_OS_IPHONE
  }
  return directory;
}

+ (void)setDebugMode:(BOOL)enable {
  Stats::Instance().SetDebugMode(enable);
}

+ (void)setup:(NSString *)serverUrl {
  [Alohalytics setup:serverUrl andFirstLaunch:YES];
}

+ (void)setup:(NSString *)serverUrl andFirstLaunch:(BOOL)isFirstLaunch {
  const auto installationId = [Alohalytics installationId];
  Stats::Instance().SetClientId(installationId.first)
                   .SetStoragePath([[Alohalytics storagePath] UTF8String])
                   .SetServerUrl([serverUrl UTF8String]);
}

+ (void)logEvent:(NSString *)event {
  Stats::Instance().LogEvent(ToStdString(event));
}

+ (void)logEvent:(NSString *)event atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withValue:(NSString *)value {
  Stats::Instance().LogEvent(ToStdString(event), ToStdString(value));
}

+ (void)logEvent:(NSString *)event withValue:(NSString *)value atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStdString(value), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withKeyValueArray:(NSArray *)array {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(array));
}

+ (void)logEvent:(NSString *)event withKeyValueArray:(NSArray *)array atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(array), ExtractLocation(location));
}

+ (void)logEvent:(NSString *)event withDictionary:(NSDictionary *)dictionary {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(dictionary));
}

+ (void)logEvent:(NSString *)event withDictionary:(NSDictionary *)dictionary atLocation:(CLLocation *)location {
  Stats::Instance().LogEvent(ToStdString(event), ToStringMap(dictionary), ExtractLocation(location));
}

@end
