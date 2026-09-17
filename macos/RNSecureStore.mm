#import "RNSecureStore.h"
#import "RNSecureStoreBackend.h"

static void Reject(RCTPromiseRejectBlock reject, NSError *error) {
  reject(error.userInfo[@"code"] ?: @"E_PLATFORM", @"Secure credential store operation failed.",
         error);
}

@implementation RNSecureStore {
  dispatch_queue_t _queue;
}
RCT_EXPORT_MODULE(RNSecureStore)
+ (BOOL)requiresMainQueueSetup {
  return NO;
}
- (instancetype)init {
  if ((self = [super init]))
    _queue = dispatch_queue_create("RNSecureStore.worker", DISPATCH_QUEUE_SERIAL);
  return self;
}

- (void)setItem:(NSString *)key
            value:(NSString *)value
          service:(NSString *)service
    applicationId:(NSString *)applicationId
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
  dispatch_async(_queue, ^{
    NSError *error = nil;
    [RNSecureStoreBackend setItem:key
                            value:value
                          service:service
                    applicationId:applicationId
                            error:&error];
    if (error)
      Reject(reject, error);
    else
      resolve(nil);
  });
}
- (void)getItem:(NSString *)key
          service:(NSString *)service
    applicationId:(NSString *)applicationId
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
  dispatch_async(_queue, ^{
    NSError *error = nil;
    NSString *value = [RNSecureStoreBackend getItem:key
                                            service:service
                                      applicationId:applicationId
                                              error:&error];
    if (error)
      Reject(reject, error);
    else
      resolve(value ?: (id)kCFNull);
  });
}
- (void)removeItem:(NSString *)key
           service:(NSString *)service
     applicationId:(NSString *)applicationId
           resolve:(RCTPromiseResolveBlock)resolve
            reject:(RCTPromiseRejectBlock)reject {
  dispatch_async(_queue, ^{
    NSError *error = nil;
    [RNSecureStoreBackend removeItem:key service:service applicationId:applicationId error:&error];
    if (error)
      Reject(reject, error);
    else
      resolve(nil);
  });
}
- (void)hasItem:(NSString *)key
          service:(NSString *)service
    applicationId:(NSString *)applicationId
          resolve:(RCTPromiseResolveBlock)resolve
           reject:(RCTPromiseRejectBlock)reject {
  dispatch_async(_queue, ^{
    NSError *error = nil;
    BOOL exists = [RNSecureStoreBackend hasItem:key
                                        service:service
                                  applicationId:applicationId
                                          error:&error];
    if (error)
      Reject(reject, error);
    else
      resolve(@(exists));
  });
}
- (void)getDefaultApplicationId:(RCTPromiseResolveBlock)resolve
                         reject:(RCTPromiseRejectBlock)reject {
  dispatch_async(_queue, ^{
    resolve([RNSecureStoreBackend defaultApplicationId]);
  });
}
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params {
  return std::make_shared<facebook::react::NativeSecureStoreSpecJSI>(params);
}
@end
