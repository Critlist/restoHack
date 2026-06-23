# Toolchain file for cross-compiling to Windows using clang-cl on Linux
# Uses MinGW-w64 headers with LLVM's MSVC-compatible frontend (clang-cl)
# Usage: cmake -S . -B build-win-clang-cl -DCMAKE_TOOLCHAIN_FILE=cmake/windows-clang-cl.cmake
#
# Strategy: clang-cl for compilation (MSVC-compatible flags + COFF output)
#           host ld links COFF objects using MinGW-w64 import libraries by full path
#
# Limitations vs native MSVC:
#   - No MSVC CRT (uses MinGW runtime)
#   - .drectve sections in COFF objects produce harmless warnings from GNU ld
#   - Need full paths to Windows import libs (ld doesn't search MinGW sysroot)

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# clang-cl: LLVM's MSVC-compatible frontend for compilation
set(CMAKE_C_COMPILER clang-cl)
set(CMAKE_C_COMPILER_TARGET x86_64-w64-windows-gnu)

# MinGW-w64 sysroot for headers and import libraries
set(MINGW_SYSROOT /usr/x86_64-w64-mingw32)
set(CMAKE_SYSROOT ${MINGW_SYSROOT})
set(CMAKE_FIND_ROOT_PATH ${MINGW_SYSROOT})

# Search programs on host; libs/headers on target sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# clang-cl ignores -L search paths; pass the Windows import lib directory
# via -Wl,-L so the host linker finds libwinmm.a etc.
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-L${MINGW_SYSROOT}/lib -Wl,-subsystem,console")

# Archive tools: use MinGW cross-toolchain (handles COFF format correctly)
find_program(CROSS_AR x86_64-w64-mingw32-ar)
if(CROSS_AR)
    set(CMAKE_AR ${CROSS_AR})
endif()
find_program(CROSS_RANLIB x86_64-w64-mingw32-ranlib)
if(CROSS_RANLIB)
    set(CMAKE_RANLIB ${CROSS_RANLIB})
endif()
