# Standalone toolchain file for cross-compiling cheri-os-test for CHERI
# RISC-V or Morello purecap (Linux or CheriBSD) without going through
# cheribuild, which otherwise passes its own generated toolchain file instead.
#
# Usage:
#   cmake -B build -S . \
#       -DCMAKE_TOOLCHAIN_FILE=cmake/cheri-purecap-toolchain.cmake \
#       [-DCHERI_SDK_ROOT=/path/to/cheri/output] \
#       [-DCHERIOSTEST_TARGET_OS=Linux|FreeBSD] \
#       [-DCHERIOSTEST_TARGET_ARCH=riscv64|morello]

# Ubuntu < 24.04 carries a downstream patch in Modules/Platform/Linux.cmake (LP #1876814,
# dropped in Ubuntu 24.04 / CMake 3.28) that appends " -latomic" directly to the
# non-cache CMAKE_C/CXX_STANDARD_LIBRARIES variables for RISC-V, which breaks linking
# against our compiler-rt + musl sysroot. Because Platform/Linux.cmake is included after
# the toolchain file and shadows any cache variable, use variable_watch() to strip
# -latomic as soon as Platform/Linux.cmake modifies those variables.
if(CMAKE_VERSION VERSION_LESS "3.28")
    function(_cheribuild_strip_latomic var access value current_list_file stack)
        if(access STREQUAL "MODIFIED_ACCESS" AND value MATCHES "-latomic")
            string(REGEX REPLACE "(^| )-latomic" "" _cleaned "${value}")
            set(${var} "${_cleaned}" PARENT_SCOPE)
        endif()
    endfunction()
    variable_watch(CMAKE_C_STANDARD_LIBRARIES _cheribuild_strip_latomic)
    variable_watch(CMAKE_CXX_STANDARD_LIBRARIES _cheribuild_strip_latomic)
endif()

set(CHERI_SDK_ROOT "$ENV{HOME}/cheri/output" CACHE PATH
    "Root of the cheribuild 'output' directory containing the SDK and sysroots")

set(CHERIOSTEST_TARGET_OS "Linux" CACHE STRING "Target OS: Linux or FreeBSD")
set_property(CACHE CHERIOSTEST_TARGET_OS PROPERTY STRINGS Linux FreeBSD)

set(CHERIOSTEST_TARGET_ARCH "riscv64" CACHE STRING
    "Target architecture: riscv64 or morello")
set_property(CACHE CHERIOSTEST_TARGET_ARCH PROPERTY STRINGS riscv64 morello)

if(CHERIOSTEST_TARGET_OS STREQUAL "Linux")
    set(CMAKE_SYSTEM_NAME Linux)
elseif(CHERIOSTEST_TARGET_OS STREQUAL "FreeBSD")
    set(CMAKE_SYSTEM_NAME FreeBSD)
else()
    message(FATAL_ERROR "CHERIOSTEST_TARGET_OS must be 'Linux' or 'FreeBSD', got '${CHERIOSTEST_TARGET_OS}'")
endif()

if(CHERIOSTEST_TARGET_ARCH STREQUAL "riscv64")
    set(CMAKE_SYSTEM_PROCESSOR "riscv64")
    set(_cheriostest_arch_flags "-mabi=l64pc128d -mno-relax")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_cheriostest_target_triple "riscv64-linux-musl")
        set(_cheriostest_sdk_bindir "${CHERI_SDK_ROOT}/cheri-alliance-sdk/bin")
        set(_cheriostest_sysroot "${CHERI_SDK_ROOT}/cheri-alliance-sdk/linux/linux-riscv64zcheri093-purecap")
        string(PREPEND _cheriostest_arch_flags "-march=rv64imafdczcherihybrid_zcherilevels "
            "-Xclang -target-feature -Xclang +cheri-bounded-vararg "
            "-Xclang -target-feature -Xclang +cheri-bounded-memarg-caller "
            "-Xclang -target-feature -Xclang +cheri-bounded-memarg-callee ")
    else()
        set(_cheriostest_target_triple "riscv64-unknown-freebsd15")
        set(_cheriostest_sdk_bindir "${CHERI_SDK_ROOT}/sdk/bin")
        set(_cheriostest_sysroot "${CHERI_SDK_ROOT}/rootfs-riscv64-purecap")
        string(PREPEND _cheriostest_arch_flags "-march=rv64imafdcxcheri ")
    endif()
elseif(CHERIOSTEST_TARGET_ARCH STREQUAL "morello")
    set(CMAKE_SYSTEM_PROCESSOR "aarch64")
    set(_cheriostest_arch_flags "-mcpu=rainier -march=morello -mabi=purecap")
    set(_cheriostest_sdk_bindir "${CHERI_SDK_ROOT}/morello-sdk/bin")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_cheriostest_target_triple "aarch64-linux-musl")
        set(_cheriostest_sysroot "${CHERI_SDK_ROOT}/cheri-alliance-sdk/linux/linux-morello-purecap")
    else()
        set(_cheriostest_target_triple "aarch64-unknown-freebsd13")
        set(_cheriostest_sysroot "${CHERI_SDK_ROOT}/rootfs-morello-purecap")
    endif()
else()
    message(FATAL_ERROR "CHERIOSTEST_TARGET_ARCH must be 'riscv64' or 'morello', got '${CHERIOSTEST_TARGET_ARCH}'")
endif()

set(CMAKE_SYSROOT "${_cheriostest_sysroot}")

# Set the runtime prefix as /opt/cheri-os-test (this is used for any CMake-generated
# RPATHs, pkg-config files, etc.), but ensure that `cmake --install`
# defaults to installing into the SDK sysroot instead of requiring DESTDIR at install time.
set(CMAKE_INSTALL_PREFIX "/opt/cheri-os-test" CACHE PATH "")
set(CMAKE_STAGING_PREFIX "${_cheriostest_sysroot}${CMAKE_INSTALL_PREFIX}" CACHE PATH
    "Directory to actually install to when cross compiling")

set(CMAKE_C_COMPILER "${_cheriostest_sdk_bindir}/clang")
set(CMAKE_CXX_COMPILER "${_cheriostest_sdk_bindir}/clang++")
set(CMAKE_ASM_COMPILER "${_cheriostest_sdk_bindir}/clang")
set(CMAKE_C_COMPILER_TARGET "${_cheriostest_target_triple}")
set(CMAKE_CXX_COMPILER_TARGET "${_cheriostest_target_triple}")
set(CMAKE_ASM_COMPILER_TARGET "${_cheriostest_target_triple}")

set(CMAKE_AR "${_cheriostest_sdk_bindir}/llvm-ar" CACHE FILEPATH "ar")
set(CMAKE_RANLIB "${_cheriostest_sdk_bindir}/llvm-ranlib" CACHE FILEPATH "ranlib")
set(CMAKE_NM "${_cheriostest_sdk_bindir}/llvm-nm" CACHE FILEPATH "nm")
set(CMAKE_STRIP "${_cheriostest_sdk_bindir}/llvm-strip" CACHE FILEPATH "strip")
set(CMAKE_LINKER "${_cheriostest_sdk_bindir}/ld.lld" CACHE FILEPATH "linker")

set(_cheriostest_common_flags
    "-target ${_cheriostest_target_triple} --sysroot=${_cheriostest_sysroot} -B${_cheriostest_sdk_bindir} ${_cheriostest_arch_flags}")

set(CMAKE_C_FLAGS_INIT "${_cheriostest_common_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_cheriostest_common_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_cheriostest_common_flags}")

set(_cheriostest_linker_flags
    "${_cheriostest_common_flags} -fuse-ld=lld --ld-path=${_cheriostest_sdk_bindir}/ld.lld")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${_cheriostest_linker_flags}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_cheriostest_linker_flags}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${_cheriostest_linker_flags}")

set(CMAKE_FIND_ROOT_PATH "${_cheriostest_sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
