/**
 * Preferred flavour of C++ outputted
 * @readonly
 * @enum {number}
 */
export enum CppFlavour {
  /** C++20 with STL types (default) */
  STL20 = 0,
  /** C++20 with Unreal 5.x types */
  Unreal5,
  // Possible useful flavours to consider in the future
  // Boost,
  // Qt6,
}

/**
 * Describes an include
 */
export type Include = {
  local: boolean;
  path: string;
};
