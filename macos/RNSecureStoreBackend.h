#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
// Synchronous OS operations. The TurboModule dispatches these to a worker queue.
@interface RNSecureStoreBackend : NSObject
+ (NSString *)defaultApplicationId;
+ (BOOL)setItem:(NSString *)key
            value:(NSString *)value
          service:(NSString *)service
    applicationId:(NSString *)applicationId
            error:(NSError **)error;
+ (nullable NSString *)getItem:(NSString *)key
                       service:(NSString *)service
                 applicationId:(NSString *)applicationId
                         error:(NSError **)error;
+ (BOOL)removeItem:(NSString *)key
           service:(NSString *)service
     applicationId:(NSString *)applicationId
             error:(NSError **)error;
+ (BOOL)hasItem:(NSString *)key
          service:(NSString *)service
    applicationId:(NSString *)applicationId
            error:(NSError **)error;
@end
NS_ASSUME_NONNULL_END
