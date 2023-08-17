#pragma once

#if defined(_MSC_VER)
#   define SOLITUDE_API_IMPORT __declspec(dllimport)
#   define SOLITUDE_API_EXPORT __declspec(dllexport)
#   define SOLITUDE_SUPPRESS_EXPORT_WARNING __pragma(warning(push)) __pragma(warning(disable: 4251 4275))
#   define SOLITUDE_RESTORE_EXPORT_WARNING __pragma(warning(pop))
#elif defined(__GNUC__)
#   define SOLITUDE_API_IMPORT
#   define SOLITUDE_API_EXPORT __attribute__((visibility("default")))
#   define SOLITUDE_SUPPRESS_EXPORT_WARNING
#   define SOLITUDE_RESTORE_EXPORT_WARNING
#else
#   define SOLITUDE_API_IMPORT
#   define SOLITUDE_API_EXPORT
#   define SOLITUDE_SUPPRESS_EXPORT_WARNING
#   define SOLITUDE_RESTORE_EXPORT_WARNING
#endif

#if defined(SOLITUDE_BUILD_SHARED)
#   ifdef SOLITUDE_EXPORT_SHARED
#       define SOLITUDE_API SOLITUDE_API_EXPORT
#   else
#       define SOLITUDE_API SOLITUDE_API_IMPORT
#   endif
#else
#   define SOLITUDE_API
#endif
