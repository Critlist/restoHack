# Toolchain file for cross-compiling to Windows using MinGW-w64 on Linux
# Usage: cmake -S . -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/windows-mingw.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# MinGW-w64 cross-compiler (available on most Linux distros as mingw-w64)
set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER x86_64-w64-mingw32-windres)

# Sysroot for headers/libs
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)

# Search programs on host; libs/headers on target sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
