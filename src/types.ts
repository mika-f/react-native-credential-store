export interface SecureStoreOptions {
  service?: string;
}

export interface SecureStoreConfiguration {
  applicationId?: string;
}

export interface SecureStore {
  configure(configuration: SecureStoreConfiguration): void;
  setItem(
    key: string,
    value: string,
    options?: SecureStoreOptions,
  ): Promise<void>;
  getItem(key: string, options?: SecureStoreOptions): Promise<string | null>;
  removeItem(key: string, options?: SecureStoreOptions): Promise<void>;
  hasItem(key: string, options?: SecureStoreOptions): Promise<boolean>;
}
