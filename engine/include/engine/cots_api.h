
#ifndef COTS_API_H
#define COTS_API_H

#ifdef COTS_STATIC_DEFINE
#  define COTS_API
#  define COTS_NO_EXPORT
#else
#  ifndef COTS_API
#    ifdef engine_EXPORTS
        /* We are building this library */
#      define COTS_API __declspec(dllexport)
#    else
        /* We are using this library */
#      define COTS_API __declspec(dllimport)
#    endif
#  endif

#  ifndef COTS_NO_EXPORT
#    define COTS_NO_EXPORT 
#  endif
#endif

#ifndef COTS_DEPRECATED
#  define COTS_DEPRECATED __declspec(deprecated)
#endif

#ifndef COTS_DEPRECATED_EXPORT
#  define COTS_DEPRECATED_EXPORT COTS_API COTS_DEPRECATED
#endif

#ifndef COTS_DEPRECATED_NO_EXPORT
#  define COTS_DEPRECATED_NO_EXPORT COTS_NO_EXPORT COTS_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef COTS_NO_DEPRECATED
#    define COTS_NO_DEPRECATED
#  endif
#endif

#endif /* COTS_API_H */
