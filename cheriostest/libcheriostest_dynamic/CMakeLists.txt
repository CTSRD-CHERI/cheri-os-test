# Helper shared library used by the cheriostest-*-dynamic test binaries.

function(add_cheriostest_dynamic_variant target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "ABI" "")
    if(NOT ARG_ABI)
        set(ARG_ABI purecap)
    endif()

    add_library(${target} SHARED
        cheriostest_dynamic_fptr.c
        cheriostest_dynamic_identity_cap.c
        cheriostest_dynamic_tls.c
    )

    # ifunc is currently only supported on FreeBSD
    if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
        target_sources(${target} PRIVATE cheriostest_dynamic_ifunc.c)
    endif()

    cheriostest_target_set_abi(${target} ${ARG_ABI})

    install(TARGETS ${target} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endfunction()

add_cheriostest_dynamic_variant(cheriostest_dynamic)
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_cheriostest_dynamic_variant(cheriostest_dynamic_hybrid ABI hybrid)
endif()
