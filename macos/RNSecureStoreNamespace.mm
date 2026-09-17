#import "RNSecureStoreNamespace.h"

NSString *const RNSecureStoreErrorDomain = @"RNSecureStore";

NSError *RNSecureStoreMakeError(NSString *code, NSNumber *nativeCode) {
  NSMutableDictionary *info =
      [@{NSLocalizedDescriptionKey : @"Secure credential store operation failed.", @"code" : code}
          mutableCopy];
  if (nativeCode)
    info[@"nativeCode"] = nativeCode;
  return [NSError errorWithDomain:RNSecureStoreErrorDomain
                             code:nativeCode.integerValue
                         userInfo:info];
}

NSData *RNSecureStoreUTF8(NSString *value, NSError **error) {
  NSData *data = [value isKindOfClass:NSString.class]
                     ? [value dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:NO]
                     : nil;
  if (!data && error)
    *error = RNSecureStoreMakeError(@"E_ENCODING", nil);
  return data;
}

NSString *RNSecureStoreEncodeComponent(NSString *value, NSError **error) {
  NSData *data = RNSecureStoreUTF8(value, error);
  if (!data)
    return nil;
  const unsigned char *bytes = static_cast<const unsigned char *>(data.bytes);
  NSMutableString *result = [NSMutableString string];
  for (NSUInteger i = 0; i < data.length; i++) {
    unsigned char c = bytes[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
        c == '.' || c == '_' || c == '~') {
      [result appendFormat:@"%c", c];
    } else {
      [result appendFormat:@"%%%02X", c];
    }
  }
  return result;
}

NSString *RNSecureStoreServiceNamespace(NSString *applicationId, NSString *service,
                                        NSError **error) {
  NSString *app = RNSecureStoreEncodeComponent(applicationId, error);
  if (!app)
    return nil;
  NSString *name = RNSecureStoreEncodeComponent(service, error);
  return name ? [NSString stringWithFormat:@"rnssecurestore://v0/%@/%@", app, name] : nil;
}
