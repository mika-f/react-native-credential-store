import type { TurboModule } from "react-native";
import { TurboModuleRegistry } from "react-native";
export interface Spec extends TurboModule {
  setItem(
    key: string,
    value: string,
    service: string,
    applicationId: string,
  ): Promise<void>;
  getItem(
    key: string,
    service: string,
    applicationId: string,
  ): Promise<string | null>;
  removeItem(
    key: string,
    service: string,
    applicationId: string,
  ): Promise<void>;
  hasItem(
    key: string,
    service: string,
    applicationId: string,
  ): Promise<boolean>;
  getDefaultApplicationId(): Promise<string>;
}
export default TurboModuleRegistry.getEnforcing<Spec>("RNSecureStore");
