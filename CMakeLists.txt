cmake_minimum_required(VERSION 3.16)
project(restoHack VERSION 1.1.1 LANGUAGES C)

# --- Supported build types: Debug | Release | RelWithDebInfo ---
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(_RESTOHACK_BUILD_TYPES "Debug;Release;RelWithDebInfo")
list(FIND _RESTOHACK_BUILD_TYPES "${CMAKE_BUILD_TYPE}" _ix)

if(_ix EQUAL -1)
    message(FATAL_ERROR "Unknown CMAKE_BUILD_TYPE='${CMAKE_BUILD_TYPE}'. Use one of: ${_RESTOHACK_BUILD_TYPES}")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Guard GCC/Clang-specific linker flags for cross-platform compatibility
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Ensure binaries carry a build-id for coredumpctl/gdb symbolication
    add_link_options(-Wl,--build-id)
endif()

# makepkg compatibility: Remove -Werror flags that cause packaging failures
# This handles system-injected CFLAGS from makepkg.conf that include -Werror=format-security
string(REPLACE "-Werror=format-security" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "-Werror[^ ]*" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(STRIP "${CMAKE_C_FLAGS}" CMAKE_C_FLAGS)

# Conditional curses library - PDCurses on Windows, ncurses on Unix
if(WIN32)
    # Check PDCursesMod submodule initialized
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/windows/pdcurses/CMakeLists.txt")
        message(FATAL_ERROR
            "PDCursesMod submodule not initialized!\n"
            "\n"
            "To initialize:\n"
            "    git submodule update --init --recursive\n"
            "\n"
            "Then re-run CMake.\n")
    endif()

    # Build PDCursesMod from vendored submodule
    # TODO: Credit PDCursesMod project (https://github.com/Bill-Gray/PDCursesMod)
    #       in README and release notes when Windows port is complete
    file(GLOB PDCURSES_CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/windows/pdcurses/pdcurses/*.c")
    file(GLOB PDCURSES_WINCON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/windows/pdcurses/wincon/*.c")

    # Build static PDCurses library
    add_library(pdcurses STATIC ${PDCURSES_CORE_SOURCES} ${PDCURSES_WINCON_SOURCES})
    target_include_directories(pdcurses PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/windows/pdcurses")
    target_compile_definitions(pdcurses PRIVATE PDC_WIDE PDC_FORCE_UTF8)

    # Set variables for linking
    set(PDCURSES_LIBRARIES pdcurses)
    set(PDCURSES_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/windows/pdcurses")

    message(STATUS "Using vendored PDCursesMod 4.5.4 (wincon port, static)")
else()
    # Unix: Use standard curses/ncurses
    if(NOT DEFINED CURSES_FOUND)
        find_package(Curses REQUIRED)
    endif()

    message(STATUS "Using ncurses for Unix terminal")
endif()

# Check for compat library (BSD-specific, might not exist on Linux)
find_library(COMPAT_LIBRARY compat)

# Feature detection for portable RNG seeding and delay_output
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckLibraryExists)

# Check for BSD-style headers and functions
check_include_file("bsd/stdlib.h" HAVE_BSD_STDLIB_H)
check_symbol_exists(arc4random "stdlib.h" HAVE_ARC4RANDOM_IN_STDLIB)
check_symbol_exists(arc4random "bsd/stdlib.h" HAVE_ARC4RANDOM_IN_BSD_STDLIB)
check_symbol_exists(arc4random_buf "stdlib.h" HAVE_ARC4RANDOM_BUF_IN_STDLIB)
check_symbol_exists(arc4random_buf "bsd/stdlib.h" HAVE_ARC4RANDOM_BUF_IN_BSD_STDLIB)

# Consolidated capability flags
if(HAVE_ARC4RANDOM_IN_STDLIB OR HAVE_ARC4RANDOM_IN_BSD_STDLIB)
    set(HAVE_ARC4RANDOM 1)
endif()

if(HAVE_ARC4RANDOM_BUF_IN_STDLIB OR HAVE_ARC4RANDOM_BUF_IN_BSD_STDLIB)
    set(HAVE_ARC4RANDOM_BUF 1)
endif()

# Check for other RNG functions
check_symbol_exists(srandomdev "stdlib.h" HAVE_SRANDOMDEV)
check_symbol_exists(getentropy "unistd.h;sys/random.h" HAVE_GETENTROPY)

# If we need libbsd for arc4random, link it
if(HAVE_BSD_STDLIB_H AND NOT HAVE_ARC4RANDOM_IN_STDLIB AND HAVE_ARC4RANDOM)
    check_library_eqxists(bsd arc4random "" HAVE_LIBBSD_ARC4)

    if(HAVE_LIBBSD_ARC4)
        set(HAVE_LIBBSD 1)
        message(STATUS "Linking libbsd for arc4random support")
    endif()
endif()

# Check if delay_output is available in curses library
set(CMAKE_REQUIRED_INCLUDES ${CURSES_INCLUDE_DIR})
check_symbol_exists(delay_output "curses.h" HAVE_DELAY_OUTPUT)

# Generate config.h with feature detection results
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
        ${CMAKE_BINARY_DIR}/generated/config.h
        @ONLY
    )
else()
    message(FATAL_ERROR "config.h.in not found at ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in")
endif()

set(HACK_SOURCES
    src/alloc.c
    src/hack.Decl.c
    src/hack.main.c
    src/hack.mkobj.c
    src/hack.invent.c
    src/hack.lock.c
    src/hack.worn.c
    src/hack.mklev.c
    src/hack.lev.c
    src/hack.pri.c
    src/hack.topl.c
    src/hack.termcap.c
    src/hack.objnam.c
    src/hack.o_init.c
    src/hack.pager.c
    src/hack.makemon.c
    src/hack.tty.c
    src/hack.fight.c
    src/hack.mon.c
    src/hack.eat.c
    src/hack.end.c
    src/hack.do_name.c
    src/hack.search.c
    src/hack.monst.c
    src/hack.cmd.c
    src/hack.c
    src/hack.do.c
    src/hack.wield.c
    src/hack.engrave.c
    src/hack.potion.c
    src/hack.read.c
    src/hack.zap.c
    src/hack.trap.c
    src/hack.apply.c
    src/hack.dog.c
    src/hack.shk.c
    src/hack.unix.c
    src/hack.save.c
    src/hack.vault.c
    src/hack.worm.c
    src/hack.mkmaze.c
    src/hack.bones.c
    src/hack.mhitu.c
    src/hack.steal.c
    src/hack.wizard.c
    src/hack.do_wear.c
    src/hack.ioctl.c
    src/hack.mkshop.c
    src/hack.rumors.c
    src/hack.rip.c
    src/hack.shknam.c
    src/hack.options.c
    src/hack.timeout.c
    src/hack.track.c
    src/hack.u_init.c
    src/hack.version.c
    src/rnd.c
)

# Windows: add win32.c shim
if(WIN32)
    list(APPEND HACK_SOURCES windows/win32.c)
    message(STATUS "Windows support injected via windows/win32.c")

    # Force-include win32.h for all source files (POSIX shims for rnd.c, etc.)
    set(WIN32_SHIM_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/windows/win32.h")
endif()

# Cross-compilation handling for hack.onames.h
if(CMAKE_CROSSCOMPILING AND WIN32)
    # Cross-compiling for Windows: use pre-generated hack.onames.h
    message(STATUS "Cross-compiling: using pre-generated windows/hack.onames.h")
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/hack.onames.h
        ${CMAKE_CURRENT_BINARY_DIR}/hack.onames.h
        COPYONLY
    )
else()
    # Native build: generate hack.onames.h using makedefs
    add_executable(makedefs src/makedefs.c)

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/hack.onames.h
        COMMAND makedefs ${CMAKE_CURRENT_SOURCE_DIR}/src/def.objects.h > ${CMAKE_CURRENT_BINARY_DIR}/hack.onames.h
        DEPENDS makedefs ${CMAKE_CURRENT_SOURCE_DIR}/src/def.objects.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating hack.onames.h with authentic 1984 object definitions"
    )
endif()

# Main hack executable
add_executable(hack ${HACK_SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/hack.onames.h)

# Target-specific include directories
target_include_directories(hack PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_BINARY_DIR}/generated
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/windows>
    $<$<PLATFORM_ID:Windows>:${PDCURSES_INCLUDE_DIRS}>
    $<$<NOT:$<PLATFORM_ID:Windows>>:${CURSES_INCLUDE_DIR}>
)

# Force-include win32.h on Windows for POSIX shims
if(WIN32)
    if(MSVC)
        target_compile_options(hack PRIVATE /FI"${WIN32_SHIM_HEADER}")
    else()
        target_compile_options(hack PRIVATE -include "${WIN32_SHIM_HEADER}")
    endif()
    message(STATUS "Force-including win32.h for POSIX compatibility shims")
endif()

# Enhanced Debug profile for better postmortem analysis
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(hack PRIVATE -fno-omit-frame-pointer)
    # ELF-specific: -rdynamic for symbol export (not applicable to Windows PE)
    if(NOT WIN32)
        target_link_options(hack PRIVATE -rdynamic)
    endif()
endif()

# Compiler flags
target_compile_options(hack PRIVATE
    $<$<C_COMPILER_ID:GNU>:-Wno-stringop-truncation>
    $<$<C_COMPILER_ID:Clang>:-Wno-string-plus-int>

    # Packaging compatibility: Suppress warnings that might be treated as errors
    -Wno-error=format-security
    -Wno-error

    # K&R vintage code compatibility: Suppress warnings for 1984 heritage code
    -Wno-deprecated-non-prototype
    -Wno-parentheses
    -Wno-unused-variable
    -Wno-misleading-indentation
    $<$<C_COMPILER_ID:Clang>:-Wno-knr-promoted-parameter>

    # Buffer overflow protection for save system safety
    $<$<C_COMPILER_ID:GNU>:-fstack-protector-strong>
    $<$<C_COMPILER_ID:Clang>:-fstack-protector-strong>

    # Enable debugging symbols for crash analysis
    -g3
)

# Fortify source for additional buffer overflow detection
# Use FORTIFY_SOURCE=3 on glibc >=2.36, fallback to =2 for compatibility
include(CheckCSourceCompiles)

# Probe whether FORTIFY=3 actually works on this libc/toolchain
set(_fortify_probe_src "
#include <features.h>
int main(void) {
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && (__GLIBC__*100+__GLIBC_MINOR__>=236)
    return 0;
#else
    #error no_fortify3
#endif
}")

set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
check_c_source_compiles("${_fortify_probe_src}" HAS_FORTIFY3)

if(HAS_FORTIFY3)
    message(STATUS "Using FORTIFY_SOURCE=3 for optimized builds (glibc >=2.36)")
else()
    message(STATUS "Using FORTIFY_SOURCE=2 (glibc <2.36 or non-glibc)")
endif()

# Optimization handled in hardening section below

# Runtime path configuration - inject absolute paths to prevent hardcoded path issues
if(DEFINED HACKDIR_OVERRIDE)
    target_compile_definitions(hack PRIVATE
        HACKDIR="${HACKDIR_OVERRIDE}"
        SAVE_VERSION="${PROJECT_VERSION}"
    )
else()
    target_compile_definitions(hack PRIVATE
        HACKDIR="${CMAKE_BINARY_DIR}/hackdir"
        SAVE_VERSION="${PROJECT_VERSION}"
    )
endif()

# Struct packing control for save system consistency
option(ENABLE_STRUCT_PACKING "Enable struct packing for save system compatibility" ON)

if(ENABLE_STRUCT_PACKING)
    # Check if compiler supports pragma pack
    include(CheckCSourceCompiles)
    check_c_source_compiles("
        #pragma pack(1)
        struct test { char a; int b; };
        #pragma pack()
        int main() { return sizeof(struct test); }
    " HAVE_PRAGMA_PACK)

    if(HAVE_PRAGMA_PACK)
        target_compile_definitions(hack PRIVATE HAVE_PRAGMA_PACK=1)
        message(STATUS "Struct packing enabled for save system consistency")
    else()
        message(WARNING "Compiler doesn't support #pragma pack - save compatibility may be affected")
    endif()
endif()

# Save system build options and debugging
option(ENABLE_SAVE_DEBUG "Enable save system debug logging" OFF)

if(ENABLE_SAVE_DEBUG)
    target_compile_definitions(hack PRIVATE SAVE_DEBUG=1)
    message(STATUS "Save system debug logging enabled")
endif()

option(ENABLE_SAVE_VALIDATION "Enable save file integrity checks" ON)

if(ENABLE_SAVE_VALIDATION)
    target_compile_definitions(hack PRIVATE SAVE_VALIDATION=1)
    message(STATUS "Save file validation enabled")
endif()

# Hardening and testing options
option(ENABLE_SANITIZERS "Address/UB sanitizers in Debug" OFF)
option(ENABLE_FUZZING "libFuzzer targets (Clang only)" OFF)

# Platform-specific compiler flags
if(MSVC)
    # MSVC hardening and warning flags
    target_compile_options(hack PRIVATE
        /W3 # Warning level 3
        /sdl # Security Development Lifecycle checks
        /guard:cf # Control Flow Guard
    )
    target_link_options(hack PRIVATE
        /DYNAMICBASE # Address Space Layout Randomization
        /NXCOMPAT # Data Execution Prevention
    )
    message(STATUS "Using MSVC hardening flags")
else()
    # GCC/Clang hardening flags
    target_compile_options(hack PRIVATE -Wall -Wextra -fstack-protector-strong)
    message(STATUS "Using GCC/Clang hardening flags")
endif()

# GCC/Clang-specific hardening (not applicable to MSVC)
if(NOT MSVC)
    # Stack clash protection (GCC feature)
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-fstack-clash-protection" HAS_SCP)

    if(HAS_SCP)
        target_compile_options(hack PRIVATE -fstack-clash-protection)
        message(STATUS "Stack clash protection enabled")
    endif()

    # Optimized configs: O2
    target_compile_options(hack PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-O2>
    )

    # ELF-specific hardening (not applicable to Windows PE format)
    if(NOT WIN32)
        target_compile_options(hack PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-fPIE>
        )
        target_link_options(hack PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack>
        )
    endif()
endif()

# Apply FORTIFY_SOURCE based on detection
if(HAS_FORTIFY3)
    target_compile_definitions(hack PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:_FORTIFY_SOURCE=3>
    )
else()
    target_compile_definitions(hack PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:_FORTIFY_SOURCE=2>
    )
endif()

# Debug builds:
if(ENABLE_SANITIZERS)
    # With sanitizers: use -O1 for better detection + FORTIFY_SOURCE compatibility
    target_compile_options(hack PRIVATE
        $<$<CONFIG:Debug>:-O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer>
    )
    target_link_options(hack PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )

    # Enable FORTIFY_SOURCE=2 in Debug with sanitizers (has -O1)
    target_compile_definitions(hack PRIVATE $<$<CONFIG:Debug>:_FORTIFY_SOURCE=2>)
    message(STATUS "Sanitizers enabled - run with: ASAN_OPTIONS=detect_stack_use_after_return=1 ./hack")
else()
    # Pure debugging: -O0 to avoid <optimized_out> in coredumps, disable FORTIFY_SOURCE
    target_compile_options(hack PRIVATE $<$<CONFIG:Debug>:-O0 -g3>)

    # Explicitly undefine FORTIFY_SOURCE first to override system CFLAGS
    target_compile_definitions(hack PRIVATE $<$<CONFIG:Debug>:_FORTIFY_SOURCE=0>)
    target_compile_options(hack PRIVATE $<$<CONFIG:Debug>:-U_FORTIFY_SOURCE>)
    message(STATUS "Debug build optimized for debugging (no <optimized_out>)")
endif()

# Fuzzing (Clang)
if(ENABLE_FUZZING AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(hack PRIVATE -fsanitize=fuzzer,address -fno-omit-frame-pointer)
    target_link_options(hack PRIVATE -fsanitize=fuzzer,address)
    message(STATUS "Fuzzing support enabled")
endif()

# Enhanced memory sanitizers specifically for save/load operations
option(ENABLE_SAVE_SANITIZERS "Enable memory sanitizers for save/load debugging" OFF)

if(ENABLE_SAVE_SANITIZERS)
    target_compile_options(hack PRIVATE -fsanitize=memory -fsanitize-memory-track-origins -g)
    target_link_options(hack PRIVATE -fsanitize=memory)
    message(STATUS "Memory sanitizers enabled for save system debugging")
endif()

# --- RelWithDebInfo profile (for production servers) ---
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(STATUS "Configuring RelWithDebInfo build (optimized with debug symbols)")

    # Perf + debuggable backtraces
    target_compile_options(hack PRIVATE -O2 -fno-omit-frame-pointer)

    # ELF-specific: PIE + RELRO/NOW + NX (not applicable to Windows PE)
    if(NOT WIN32)
        target_compile_options(hack PRIVATE -fPIE)
        target_link_options(hack PRIVATE -pie -Wl,-z,relro,-z,now -Wl,-z,noexecstack)
    endif()

    # Fortify: prefer =3 when available (fallback to 2)
    if(HAS_FORTIFY3)
        target_compile_definitions(hack PRIVATE _FORTIFY_SOURCE=3)
    else()
        target_compile_definitions(hack PRIVATE _FORTIFY_SOURCE=2)
    endif()

    # Slightly stricter but sane warnings
    target_compile_options(hack PRIVATE -Wall -Wextra -Wswitch -Wwrite-strings -Wformat=2)
endif()

# Conditional curses library linking
if(WIN32)
    # Windows static linking configuration
    # Link PDCurses and required Windows system libraries
    target_link_libraries(hack PRIVATE ${PDCURSES_LIBRARIES} winmm)
    # PDCurses compile definitions must match those used to build libpdcurses
    target_compile_definitions(hack PRIVATE PDCURSES PDC_WIDE PDC_FORCE_UTF8)


    if(MSVC)
        # MSVC: Static runtime library (eliminates vcruntime140.dll dependency)
        target_link_options(hack PRIVATE
            /SUBSYSTEM:CONSOLE
            /INCREMENTAL:NO
        )
        set_property(TARGET hack PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        message(STATUS "Configured MSVC static linking (/MT runtime)")
    else()
        # MinGW/GCC: Selectively static-link GCC runtime only
        # WHY: Full -static can break Windows system library linking
        # HOW: Link libgcc/libstdc++ statically, Windows APIs dynamically
        target_link_options(hack PRIVATE
            -static-libgcc
            -static-libstdc++
        )
        message(STATUS "Configured MinGW selective static linking (GCC runtime only)")
    endif()

    message(STATUS "Linking vendored PDCursesMod (static)")
else()
    # Unix: Link standard curses
    target_link_libraries(hack ${CURSES_LIBRARIES})
    message(STATUS "Linking ncurses for Unix")
endif()

if(COMPAT_LIBRARY)
    target_link_libraries(hack ${COMPAT_LIBRARY})
endif()

if(HAVE_LIBBSD)
    target_link_libraries(hack bsd)
endif()

# Always handy to inspect flags locally
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Common warnings for all builds (sane, not noisy)
target_compile_options(hack PRIVATE
    -Wall -Wextra -Wformat=2
    -Wno-missing-field-initializers
    -Wno-old-style-definition
    -fno-common
)

# === Profiles ===
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # General build = ship it
    target_compile_options(hack PRIVATE -O2 -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Dev build = symbols + easy backtraces, warnings don't kill flow
    target_compile_options(hack PRIVATE -O0 -g3 -fno-omit-frame-pointer -Wwrite-strings -Wno-error)
    # ELF-specific: -rdynamic for symbol export (not applicable to Windows PE)
    if(NOT WIN32)
        target_link_options(hack PRIVATE -rdynamic)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # safe, strict, but do NOT fail on warnings
    target_compile_options(hack PRIVATE
        -O2 -fno-omit-frame-pointer
        -Wwrite-strings
        -Wno-error
    )

    # Fortify (3 if toolchain supports it; harmless if ignored)
    target_compile_definitions(hack PRIVATE _FORTIFY_SOURCE=3)

    # ELF-specific hardening (not applicable to Windows PE)
    if(NOT WIN32)
        target_compile_options(hack PRIVATE -fPIE)
        target_link_options(hack PRIVATE
            -pie
            -Wl,-z,relro,-z,now
            -Wl,-z,noexecstack
        )
    endif()

    # Quiet a recent Clang nit, only if relevant
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 18)
        target_compile_options(hack PRIVATE -Wno-deprecated-non-prototype)
    endif()
endif()

# REMOVED: Conflicting sanitizer configuration that was overriding ENABLE_SANITIZERS
# All sanitizer control now handled by ENABLE_SANITIZERS option above (lines 306-322)

# Data files
set(DATA_FILES rumors help hh data)

# Versioned save system configuration
set(HACK_VERSION "${PROJECT_VERSION}")
set(SAVE_DIR "${CMAKE_BINARY_DIR}/hackdir/save")
set(VERSIONED_SAVE_DIR "${SAVE_DIR}/v${HACK_VERSION}")
set(BACKUP_SAVE_DIR "${SAVE_DIR}/backup")

option(RESTOHACK_COPY_ASSETS "Copy asset directories into build tree" ON)

add_custom_target(setup_hackdir ALL

    # Copy hackdir structure and ensure write permissions on record file
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/hackdir ${CMAKE_BINARY_DIR}/hackdir

    # Create required directories and lock files
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SAVE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VERSIONED_SAVE_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BACKUP_SAVE_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/hackdir/perm

    # Ensure record file exists and is writable (game checks this at startup)
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/hackdir/record

    # Also copy files to build root for pre-chdir access (legacy compatibility)
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/perm

    # Clean all possible lock files (HLOCK, LLOCK, record_lock in all locations)
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/record_lock ${CMAKE_BINARY_DIR}/safelock ${CMAKE_BINARY_DIR}/hackdir/record_lock ${CMAKE_BINARY_DIR}/hackdir/safelock
    COMMAND ${CMAKE_COMMAND} -E remove -f ${SAVE_DIR}/* ${VERSIONED_SAVE_DIR}/* ${BACKUP_SAVE_DIR}/*
    COMMENT "Setting up versioned save system: v${HACK_VERSION} with backup support"
    VERBATIM
)

# Copy asset files (only if they exist and copying is enabled)
if(RESTOHACK_COPY_ASSETS)
    # All assets are files, not directories
    foreach(_asset data help hh rumors)
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_asset})
            add_custom_command(TARGET setup_hackdir POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asset}
                ${CMAKE_BINARY_DIR}/hackdir/${_asset}
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/${_asset}
                ${CMAKE_BINARY_DIR}/${_asset})
        else()
            message(WARNING "Asset '${_asset}' missing in source tree.")
        endif()
    endforeach()
endif()

# Ensure hack depends on the setup
add_dependencies(hack setup_hackdir)

# Custom clean target to remove all generated files and lock files
add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/hackdir
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/perm ${CMAKE_BINARY_DIR}/data ${CMAKE_BINARY_DIR}/help ${CMAKE_BINARY_DIR}/hh ${CMAKE_BINARY_DIR}/rumors
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/record_lock ${CMAKE_BINARY_DIR}/safelock
    COMMENT "Clean all hack files and lock files"
)

# Install rules
install(TARGETS hack DESTINATION bin)
install(FILES ${DATA_FILES} DESTINATION share/hack)
install(FILES man/hack.6 DESTINATION share/man/man6)

# ---- Security Options ----
option(CLANG_STRICT "Enable draconian warnings under Clang" OFF)

if(CLANG_STRICT AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(hack PRIVATE
        -Weverything -Werror
        -Wno-poison-system-directories
        -Wno-padded -Wno-cast-align -Wno-disabled-macro-expansion
    )
endif()

# ---- Packaging (optional) ----
option(RESTOHACK_ENABLE_PACKAGE "Enable CPack packaging" ON)

# CPack configuration for distribution
set(CPACK_PACKAGE_NAME "restohack")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Authentic 1984 Hack game restoration - modernized for contemporary systems")
set(CPACK_PACKAGE_DESCRIPTION "restoHack brings the legendary 1984 Hack game back from the dead with 94.2% authentic code preservation. This isn't a reimagining - it's historical software preservation with modern compatibility. Features complete K&R to ANSI C conversion, cross-platform builds, and 100% authentic 1984 gameplay experience.")

set(CPACK_PACKAGE_VENDOR "restoHack Project")
set(CPACK_PACKAGE_CONTACT "https://github.com/Critlist/restoHack.git")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/Critlist/restoHack.git")

# Allow override via -DCPACK_RESOURCE_FILE_*; default to files in source tree.
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" CACHE FILEPATH "")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md" CACHE FILEPATH "")

# If the resources aren't present, disable packaging instead of erroring.
if(NOT EXISTS "${CPACK_RESOURCE_FILE_LICENSE}" OR NOT EXISTS "${CPACK_RESOURCE_FILE_README}")
    message(WARNING "CPack resources missing; disabling packaging for this build.")
    set(RESTOHACK_ENABLE_PACKAGE OFF)
endif()

# Source package configuration
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/\\.git/"
    "/\\.gitignore"
    "hackdir/record_lock"
    "hackdir/safelock"
    "\\.DS_Store"
    ".*~$"
)

# Platform-specific packaging #   #I'm hopeful people want to play this on their systems, so this will be implemented for later use
if(WIN32)
    # Windows NSIS installer
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "restoHack - Authentic 1984 Hack Game")
    set(CPACK_NSIS_HELP_LINK "https://github.com/Critlist/restoHack.git")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/Critlist/restoHack.git")
elseif(APPLE)
    # macOS packages
    set(CPACK_GENERATOR "TGZ;DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "restoHack")
else()
    # Linux packages (DEB, RPM, tar.gz)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")

    # Debian package configuration
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "restoHack Project <noreply@example.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libncurses5 | libncurses6")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

    # RPM package configuration
    set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
    set(CPACK_RPM_PACKAGE_LICENSE "BSD-3-Clause")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc, ncurses")
    set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
endif()

# Enable CPack only if packaging is enabled
if(RESTOHACK_ENABLE_PACKAGE)
    include(CPack)
endif()
