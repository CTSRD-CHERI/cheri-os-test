# Builds the cheriostest-* test binaries: purecap for Linux or CheriBSD, plus
# hybrid and (Morello-only) purecap-benchmark ABI variants for CheriBSD.

if(CHERIOSTEST_TARGET_ARCH STREQUAL "riscv64")
    set(CHERIOSTEST_MD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/riscv")
elseif(CHERIOSTEST_TARGET_ARCH STREQUAL "morello")
    set(CHERIOSTEST_MD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/arm64")
else()
    message(FATAL_ERROR "Unsupported CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

find_package(LibXo REQUIRED)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(LibBSD REQUIRED)
endif()

# CAPREVOKE is currently only supported on CheriBSD.
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(CHERIOSTEST_HAVE_CHERI_CAPREVOKE TRUE)
endif()

set(CHERIOSTEST_COMMON_SRCS
    cheriostest.c
    cheriostest_bounds_globals.c
    cheriostest_util.c
    cheriostest_util_vm.c
    cheriostest_bounds_globals_x.c
    cheriostest_bounds_heap.c
    cheriostest_bounds_stack.c
    cheriostest_cidcap.c
    cheriostest_context.c
    cheriostest_fault.c
    cheriostest_flag_captured.c
    cheriostest_fs.c
    cheriostest_ifunc.c
    cheriostest_kbounce.c
    cheriostest_internal.c
    cheriostest_ipc.c
    cheriostest_local_global.c
    cheriostest_longjmp.c
    cheriostest_printf.c
    cheriostest_ptrace.c
    cheriostest_registers.c
    cheriostest_sealcap.c
    cheriostest_signal.c
    cheriostest_string.c
    cheriostest_tls.c
    cheriostest_syscall.c
    cheriostest_vm.c
    cheriostest_strfcap.c
)

set(CHERIOSTEST_PURECAP_SRCS
    cheriostest_bounds_subobject.c
    cheriostest_malloc.c
    cheriostest_bounds_varargs.c
    cheriostest_sentries.c
    cheriostest_cheriabi.c
    cheriostest_cheriabi_libc.c
    cheriostest_cheriabi_open.c
)

# Sub-object bounds are only meaningful (and only enabled) for this one test.
set_source_files_properties(cheriostest_bounds_subobject.c PROPERTIES
    COMPILE_OPTIONS "-cheri-bounds=subobject-safe")

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    list(APPEND CHERIOSTEST_COMMON_SRCS cheriostest_zlib.c)
endif()

# list(APPEND CHERIOSTEST_COMMON_SRCS cheriostest_compartments.c)

# name: variant suffix, e.g. "purecap" -> target cheriostest-purecap.
# ABI: purecap (default), hybrid, or purecap-benchmark.
# DYNAMIC: also build/link against libcheriostest_dynamic and its tests.
# THREADS: add the pthread-interposition test variant.
function(add_cheriostest_variant name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "DYNAMIC;THREADS" "ABI" "")
    if(NOT ARG_ABI)
        set(ARG_ABI purecap)
    endif()

    set(target "cheriostest-${name}")
    add_executable(${target} ${CHERIOSTEST_COMMON_SRCS})
    if(NOT ARG_ABI STREQUAL "hybrid")
        target_sources(${target} PRIVATE ${CHERIOSTEST_PURECAP_SRCS})
    endif()
    # BEFORE so we use cheri/cheric.h from this directory rather than the one
    # in the CheriBSD sysroot.
    target_include_directories(${target} BEFORE PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CHERIOSTEST_MD_DIR}"
    )
    target_compile_definitions(${target} PRIVATE
        PROG="${target}"
        PROG_SUFFIX="-${name}"
    )
    target_link_libraries(${target} PRIVATE util LibXo::xo)

    cheriostest_target_set_abi(${target} ${ARG_ABI})

    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        # For static variants we have to manually list libprocstat's own dependencies
        #(_DP_procstat in CheriBSD's src.libnames.mk) since they aren't pulled in automatically.
        target_link_libraries(${target} PRIVATE procstat kvm elf z)
        if(CHERIOSTEST_HAVE_CHERI_CAPREVOKE AND NOT ARG_ABI STREQUAL "hybrid")
            target_link_libraries(${target} PRIVATE cheri_caprevoke)
            target_compile_definitions(${target} PRIVATE CHERIOSTEST_CHERI_REVOKE_TESTS)
        endif()
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${target} PRIVATE LibBSD::bsd)
        target_compile_definitions(${target} PRIVATE CHERIOSTEST_CHERI_REVOKE_TESTS)
    endif()

    if(ARG_DYNAMIC)
        target_compile_definitions(${target} PRIVATE
            CHERIBSD_DYNAMIC_TESTS
            LIBM_SONAME="libm.so.5"
        )
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libcheriostest_dynamic)
        target_sources(${target} PRIVATE
            cheriostest_fptr_canon.c
            cheriostest_lazy_bind.c
        )
        if(ARG_ABI STREQUAL "hybrid")
            target_link_libraries(${target} PRIVATE cheriostest_dynamic_hybrid)
        else()
            target_link_libraries(${target} PRIVATE cheriostest_dynamic)
        endif()
    else()
        target_link_options(${target} PRIVATE -static)
    endif()

    if(ARG_THREADS)
        target_compile_definitions(${target} PRIVATE CHERIBSD_THREAD_TESTS)
        target_sources(${target} PRIVATE cheriostest_tls_threads.c)
        target_link_libraries(${target} PRIVATE pthread)
    endif()

    install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endfunction()

add_subdirectory(libcheriostest_dynamic)

add_cheriostest_variant(purecap)
add_cheriostest_variant(purecap-mt THREADS)
add_cheriostest_variant(purecap-dynamic DYNAMIC)
add_cheriostest_variant(purecap-dynamic-mt DYNAMIC THREADS)

# Hybrid and purecap-benchmark are CheriBSD only for now.
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_cheriostest_variant(hybrid ABI hybrid)
    add_cheriostest_variant(hybrid-mt ABI hybrid THREADS)
    add_cheriostest_variant(hybrid-dynamic ABI hybrid DYNAMIC)
    add_cheriostest_variant(hybrid-dynamic-mt ABI hybrid DYNAMIC THREADS)

    # Purecap benchmark ABI is Morello only.
    if(CHERIOSTEST_TARGET_ARCH STREQUAL "morello")
        add_cheriostest_variant(purecap-benchmark ABI purecap-benchmark)
        add_cheriostest_variant(purecap-benchmark-mt ABI purecap-benchmark THREADS)
    endif()
endif()

add_subdirectory(malloc_early_constructor)

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_subdirectory(malloc_revoke_enabled)
endif()
