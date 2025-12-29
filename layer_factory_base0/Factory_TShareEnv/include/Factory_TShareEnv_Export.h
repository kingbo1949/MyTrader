#pragma once

// 兼容 clang/gcc 的可见性属性
#if defined(_WIN32) || defined(__CYGWIN__)
  #if defined(FACTORY_LOG_EXPORTS)
    #define FACTORY_TSHAREENV_API __declspec(dllexport)
  #else
    #define FACTORY_TSHAREENV_API __declspec(dllimport)
  #endif
#else
  // macOS / clang / gcc
  #if __GNUC__ >= 4
    #define FACTORY_TSHAREENV_API __attribute__((visibility("default")))
  #else
    #define FACTORY_TSHAREENV_API
  #endif
#endif
