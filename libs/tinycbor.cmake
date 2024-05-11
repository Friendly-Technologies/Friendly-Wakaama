set(TINYCBOR_SOURCES_DIR ${CMAKE_CURRENT_LIST_DIR}/tinycbor/src)
set(TINYCBOR_SOURCES
    ${TINYCBOR_SOURCES_DIR}/cbor.h
    ${TINYCBOR_SOURCES_DIR}/cborencoder.c
    ${TINYCBOR_SOURCES_DIR}/cborencoder_close_container_checked.c
    ${TINYCBOR_SOURCES_DIR}/cborencoder_float.c
    ${TINYCBOR_SOURCES_DIR}/cborerrorstrings.c
    ${TINYCBOR_SOURCES_DIR}/cborparser.c
    ${TINYCBOR_SOURCES_DIR}/cborparser_dup_string.c
    ${TINYCBOR_SOURCES_DIR}/cborparser_float.c
    ${TINYCBOR_SOURCES_DIR}/cborpretty.c
    ${TINYCBOR_SOURCES_DIR}/cborpretty_stdio.c
    ${TINYCBOR_SOURCES_DIR}/cbortojson.c
    ${TINYCBOR_SOURCES_DIR}/cborvalidation.c
)
set(TINYCBOR_ALLOC_HEADER -DCBOR_CUSTOM_ALLOC_INCLUDE="${CMAKE_CURRENT_LIST_DIR}/wakaama_tinycbor_alloc.h")

# Add tinycbor sources to an existing target.
function(target_sources_tinycbor target)
    target_sources(${target} PRIVATE ${TINYCBOR_SOURCES})
    set_source_files_properties(${TINYCBOR_SOURCES} PROPERTIES COMPILE_FLAGS -Wno-float-equal)
    target_include_directories(${target} PRIVATE ${TINYCBOR_SOURCES_DIR})
    target_compile_definitions(${target} PRIVATE ${TINYCBOR_ALLOC_HEADER})
    target_link_libraries(${target} m)
endfunction()