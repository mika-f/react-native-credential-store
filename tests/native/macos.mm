#import <Foundation/Foundation.h>
#import "RNSecureStoreBackend.h"
#import "RNSecureStoreNamespace.h"
#include <cstdio>
#include <stdexcept>
#include <string>
#include <initializer_list>
#include <atomic>

static void check(bool condition, const char *label) {
  if (!condition)
    throw std::runtime_error(label);
}
static void noError(NSError *error) {
  if (error) {
    // Only diagnostic codes, never values.
    fprintf(stderr, "native error: %s (%ld)\n", [error.userInfo[@"code"] UTF8String],
            (long)error.code);
    throw std::runtime_error("unexpected native error");
  }
}

static void unitTests() {
  NSArray *vectors = @[
    @[ @"AZaz09-._~", @"AZaz09-._~" ], @[ @"/:% !'()*", @"%2F%3A%25%20%21%27%28%29%2A" ],
    @[ @"認証", @"%E8%AA%8D%E8%A8%BC" ], @[ @"🔑", @"%F0%9F%94%91" ]
  ];
  for (NSArray *pair in vectors) {
    NSError *error = nil;
    check([RNSecureStoreEncodeComponent(pair[0], &error) isEqualToString:pair[1]],
          "percent encoding");
    noError(error);
  }
  NSError *error = nil;
  check(![RNSecureStoreEncodeComponent(@"é", &error)
            isEqualToString:RNSecureStoreEncodeComponent(@"e\u0301", &error)],
        "Unicode normalization");
  check(![RNSecureStoreEncodeComponent(@"Token", &error)
            isEqualToString:RNSecureStoreEncodeComponent(@"token", &error)],
        "case preservation");
  check([RNSecureStoreServiceNamespace(@"com.example.app", @"oauth/google", &error)
            isEqualToString:@"rnssecurestore://v0/com.example.app/oauth%2Fgoogle"],
        "service namespace");
  unichar surrogate = 0xd800;
  NSString *invalid = [NSString stringWithCharacters:&surrogate length:1];
  check(RNSecureStoreUTF8(invalid, &error) == nil, "invalid UTF-16");
  check([error.userInfo[@"code"] isEqualToString:@"E_ENCODING"], "encoding code");
  error = nil;
  check(![RNSecureStoreBackend setItem:@"key"
                                 value:[@"あ" stringByPaddingToLength:854
                                                           withString:@"あ"
                                                      startingAtIndex:0]
                               service:@"service"
                         applicationId:@"app"
                                 error:&error],
        "oversized value");
  check([error.userInfo[@"code"] isEqualToString:@"E_VALUE_TOO_LARGE"], "size code");
  for (NSArray *tuple in @[
         @[ @"", @"service", @"key", @"E_INVALID_APPLICATION_ID" ],
         @[ @"app", @"", @"key", @"E_INVALID_SERVICE" ],
         @[ @"app", @"service", @"", @"E_INVALID_KEY" ]
       ]) {
    error = nil;
    [RNSecureStoreBackend hasItem:tuple[2] service:tuple[1] applicationId:tuple[0] error:&error];
    check([error.userInfo[@"code"] isEqualToString:tuple[3]], "native identity validation");
  }
  check([RNSecureStoreBackend defaultApplicationId].length > 0, "default identity");
  check([[RNSecureStoreBackend defaultApplicationId]
            isEqualToString:[RNSecureStoreBackend defaultApplicationId]],
        "cached identity");
  puts("macOS native namespace and validation tests passed");
}

static void integrationTests() {
  NSString *app = [@"rnssecurestore.tests." stringByAppendingString:NSUUID.UUID.UUIDString];
  NSMutableArray<NSArray *> *created = [NSMutableArray array];
  auto set = [&](NSString *key, NSString *value, NSString *service, NSString *identity) {
    [created addObject:@[ key, service, identity ]];
    NSError *error = nil;
    [RNSecureStoreBackend setItem:key
                            value:value
                          service:service
                    applicationId:identity
                            error:&error];
    noError(error);
  };
  auto get = [&](NSString *key, NSString *service, NSString *identity) {
    NSError *error = nil;
    NSString *value = [RNSecureStoreBackend getItem:key
                                            service:service
                                      applicationId:identity
                                              error:&error];
    noError(error);
    return value;
  };
  auto remove = [&](NSString *key) {
    NSError *error = nil;
    [RNSecureStoreBackend removeItem:key service:@"default" applicationId:app error:&error];
    noError(error);
  };
  auto has = [&](NSString *key) {
    NSError *error = nil;
    BOOL exists = [RNSecureStoreBackend hasItem:key
                                        service:@"default"
                                  applicationId:app
                                          error:&error];
    noError(error);
    return exists;
  };
  // @finally also runs when a C++ exception unwinds Objective-C++ code.
  @try {
    check(get(@"missing", @"default", app) == nil, "missing get");
    check(!has(@"missing"), "missing has");
    remove(@"missing");
    set(@"token", @"one", @"default", app);
    check([get(@"token", @"default", app) isEqualToString:@"one"], "set/get");
    check(has(@"token"), "existing has");
    set(@"token", @"two", @"default", app);
    check([get(@"token", @"default", app) isEqualToString:@"two"], "upsert");
    for (NSString *value in @[ @"", @"認証🔑", [NSString stringWithFormat:@"a%Cb", (unichar)0] ]) {
      set(@"value", value, @"default", app);
      check([get(@"value", @"default", app) isEqualToString:value] && has(@"value"),
            "value round trip");
    }
    for (NSUInteger length : {2559u, 2560u}) {
      NSString *value = [@"あ" stringByPaddingToLength:853 withString:@"あ" startingAtIndex:0];
      if (length == 2560)
        value = [value stringByAppendingString:@"x"];
      set(@"boundary", value, @"default", app);
      check([get(@"boundary", @"default", app) isEqualToString:value], "byte boundary");
    }
    NSArray *tuples = @[
      @[ @"token", @"default", app ], @[ @"Token", @"default", app ],
      @[ @"token", @"Default", app ], @[ @"token", @"default", app.uppercaseString ],
      @[ @"é", @"認証", app ], @[ @"e\u0301", @"認証", app ]
    ];
    for (NSUInteger i = 0; i < tuples.count; i++) {
      NSArray *t = tuples[i];
      set(t[0], [@(i) stringValue], t[1], t[2]);
    }
    for (NSUInteger i = 0; i < tuples.count; i++) {
      NSArray *t = tuples[i];
      check([get(t[0], t[1], t[2]) isEqualToString:[@(i) stringValue]], "namespace isolation");
    }
    set(@"concurrent", @"initial", @"default", app);
    std::atomic<bool> concurrentFailure{false};
    auto *failure = &concurrentFailure;
    dispatch_apply(32, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^(size_t i) {
      @autoreleasepool {
        NSError *error = nil;
        [RNSecureStoreBackend setItem:@"concurrent"
                                value:[@(i) stringValue]
                              service:@"default"
                        applicationId:app
                                error:&error];
        if (error)
          failure->store(true);
      }
    });
    check(!concurrentFailure.load() && has(@"concurrent"), "concurrent writes");
    remove(@"token");
    remove(@"token");
    check(get(@"token", @"default", app) == nil && !has(@"token"), "removal");
  } @finally {
    for (NSArray *tuple in created) {
      NSError *error = nil;
      [RNSecureStoreBackend removeItem:tuple[0]
                               service:tuple[1]
                         applicationId:tuple[2]
                                 error:&error];
      if (error)
        fprintf(stderr, "Test credential cleanup failed: %ld\n", (long)error.code);
    }
  }
  puts("macOS Keychain integration tests passed");
}

int main(int argc, const char *argv[]) {
  @autoreleasepool {
    try {
      unitTests();
      if (argc > 1 && std::string(argv[1]) == "--integration")
        integrationTests();
    } catch (const std::exception &error) {
      fprintf(stderr, "FAIL: %s\n", error.what());
      return 1;
    }
  }
  return 0;
}
