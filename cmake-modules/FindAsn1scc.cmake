find_program(ASN1SCC NAMES Asn1f2.exe)

if(ASN1SCC MATCHES ".*-NOTFOUND")
    message(FATAL_ERROR "Asn1f2.exe not found")
endif(ASN1SCC MATCHES ".*-NOTFOUND")

if(ASN1SCC)
    function(add_asn1scc_lib lib_name)
        set(ASN1_SRCS ${ARGN})
        foreach(MOD ${ASN1_SRCS})
            list(APPEND ASN1_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${MOD}.asn1)
            list(APPEND ASN1_CFILES ${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c)
        endforeach()
        set(ASN1_COMMON
            ${CMAKE_CURRENT_BINARY_DIR}/acn.c
            ${CMAKE_CURRENT_BINARY_DIR}/asn1crt.c
            ${CMAKE_CURRENT_BINARY_DIR}/real.c
            )

        add_custom_command(
            OUTPUT ${ASN1_CFILES} ${ASN1_COMMON}
            COMMAND ${ASN1SCC} -c -uPER ${ASN1_FILES}
            DEPENDS ${ASN1_FILES}
            )

        add_library(${lib_name}_asn1 STATIC
            ${ASN1_COMMON}
            ${ASN1_CFILES}
            )
        target_include_directories(${lib_name}_asn1
            PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        target_compile_options(${lib_name}_asn1
            PRIVATE "-Wno-error"
            PRIVATE "-w")
    endfunction()
endif(ASN1SCC)
