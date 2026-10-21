cmake_minimum_required(VERSION 3.20)

# A plain `cmake -B build -S .` defaults to Linux RISC-V.
# For other targets, use the cmake presets (e.g. `cmake --preset freebsd-morello`)
# or build with cheribuild.
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cheri-purecap-toolchain.cmake" CACHE FILEPATH "toolchain")
endif()

# Defer enabling C until the flags below are in place; project(... NONE) still
# loads the toolchain file, so CMAKE_SYSTEM_NAME is already known here.
project(cheri-os-test NONE)

if(NOT DEFINED CHERIOSTEST_TARGET_ARCH)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^riscv")
        set(CHERIOSTEST_TARGET_ARCH "riscv64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64|morello)")
        set(CHERIOSTEST_TARGET_ARCH "morello")
    else()
        message(FATAL_ERROR "Unsupported CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Linux|FreeBSD)$")
    message(FATAL_ERROR "cheri-os-test only supports cross-compiling for Linux or CheriBSD, "
                         "got CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
endif()


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Avoid depending on libgcc_s: the cheribuild Linux sysroots don't have it
    # and the aarch64 compiler driver links -lgcc_s by default.
    # Avoid using it during initial compiler configuration by setting
    # CMAKE_TRY_COMPILE_TARGET_TYPE to avoid the link step.
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
    add_link_options(--unwindlib=none)
endif()

enable_language(C)

include(CheckCCompilerFlag)
include(GNUInstallDirs)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CheriostestAbi)

# Set this flag to avoid using incorrect #includes "cheri/cheric.h" instead of <cheri/cheric.h>.
check_c_compiler_flag("-Winclude-next-absolute-path" CHERIOSTEST_HAVE_WNO_INCLUDE_NEXT_ABSOLUTE_PATH)
if(CHERIOSTEST_HAVE_WNO_INCLUDE_NEXT_ABSOLUTE_PATH)
    add_compile_options(-Werror=include-next-absolute-path)
endif()

# Mirror the installed layout in the build tree. This is needed since the
# tests exec() their helper relative to their own location.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}")
set(CHERIOSTEST_LIBEXEC_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBEXECDIR}")

# Note cheribuild can currently only actually run these when cross-compiling
# for CheriBSD, since that is the only target it has a script for to run them
# in a VM; there is no such script for Linux yet.
option(BUILD_TESTING "Register the cheriostest binaries with CTest" ON)
if(BUILD_TESTING)
    enable_testing()
endif()

add_subdirectory(cheriostest)
