function(add_malloc_early_constructor_variant name)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "THREADS" "" "")

    set(target "malloc_early_constructor-${name}")
    add_executable(${target} malloc_early_constructor.c)
    if(ARG_THREADS)
        target_link_libraries(${target} PRIVATE pthread)
    endif()
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CHERIOSTEST_LIBEXEC_OUTPUT_DIRECTORY}")
    install(TARGETS ${target} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
endfunction()

add_malloc_early_constructor_variant(purecap)
add_malloc_early_constructor_variant(purecap-mt THREADS)
add_malloc_early_constructor_variant(purecap-dynamic)
add_malloc_early_constructor_variant(purecap-dynamic-mt THREADS)
