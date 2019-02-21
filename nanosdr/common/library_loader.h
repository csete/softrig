#pragma once

#ifdef WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

/* FIXME: Implement load_lib() as function that tries
 *        different locations.
 */
/* clang-format off */
#ifdef WINDOWS
#define lib_handle_t       HMODULE
#define load_library(name) LoadLibrary(L(name) ".dll")  // FIXME: not sure if it works?
#define close_library(lib) FreeLibrary(lib)
#define get_symbol(l, s)   GetProcAddress(l, s)
#elif MACOSX
#define lib_handle_t       void *
#define load_library(name) dlopen("lib" name ".dylib", RTLD_NOW)
#define close_library(lib) dlclose(lib)
#define get_symbol(l, s)   dlsym(l, s)
#else
#define lib_handle_t       void *
#define load_library(name) dlopen("lib" name ".so", RTLD_NOW)
#define close_library(lib) dlclose(lib)
#define get_symbol(l, s)   dlsym(l, s)
#endif
/* clang-format on */
