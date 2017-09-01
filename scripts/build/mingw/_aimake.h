#ifndef AIMAKE_HEADER_INCLUDED
# define AIMAKE_HEADER_INCLUDED
# define AIMAKE_BUILDOS_MSWin32 1
extern const char *aimake_get_option(const char *);
# ifndef AIMAKE_OVERRIDE_DEFINES
#  define AIMAKE_ABI_VERSION(x)
#  define AIMAKE_IMPORT(x) __declspec(dllimport) x
#  define AIMAKE_REVERSE_IMPORT(x) __declspec(dllimport) x
#  define AIMAKE_EXPORT(x) __declspec(dllexport) x
#  define AIMAKE_REVERSE_EXPORT(x) __declspec(dllexport) x
# endif
#endif
