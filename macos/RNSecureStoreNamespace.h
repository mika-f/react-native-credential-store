#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN
FOUNDATION_EXPORT NSString *const RNSecureStoreErrorDomain;
FOUNDATION_EXPORT NSError *RNSecureStoreMakeError(NSString *code, NSNumber *_Nullable nativeCode);
FOUNDATION_EXPORT NSData *_Nullable RNSecureStoreUTF8(NSString *value, NSError **error);
FOUNDATION_EXPORT NSString *_Nullable RNSecureStoreEncodeComponent(NSString *value,
                                                                   NSError **error);
FOUNDATION_EXPORT NSString *_Nullable RNSecureStoreServiceNamespace(NSString *applicationId,
                                                                    NSString *service,
                                                                    NSError **error);
NS_ASSUME_NONNULL_END
