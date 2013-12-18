include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckFunctionExists)

# FIXME: Make this changeable!
# khtml svg support
set(SVG_SUPPORT 1)

check_include_files(malloc.h      HAVE_MALLOC_H) 
check_include_files(alloca.h      HAVE_ALLOCA_H)

check_function_exists(getpagesize      HAVE_GETPAGESIZE)
check_function_exists(mmap             HAVE_MMAP)
