#ifndef OD_EXPORT_H
#define OD_EXPORT_H

#if defined(_WIN32) || defined(__CYGWIN__)
  #ifdef OD_CORE_EXPORTS
    #define OD_API __declspec(dllexport)
  #else
    #define OD_API __declspec(dllimport)
  #endif
#else
  #if __GNUC__ >= 4
    #define OD_API __attribute__ ((visibility ("default")))
  #else
    #define OD_API
  #endif
#endif

#endif
