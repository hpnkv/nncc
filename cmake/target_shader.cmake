function( target_shader ARG_TARGET ARG_FILE )
    cmake_parse_arguments( ARG "FRAGMENT;VERTEX;COMPUTE" "OUTPUT" "" ${ARGN})

    if( ARG_FRAGMENT )
        set( TYPE "FRAGMENT" )
    elseif( ARG_VERTEX )
        set( TYPE "VERTEX" )
    elseif( ARG_COMPUTE )
        set( TYPE "COMPUTE" )
    else()
        message( SEND_ERROR "target_shader must be called with either FRAGMENT or VERTEX or COMPUTE." )
        return()
    endif()

    if( NOT ARG_OUTPUT )
        set(ARG_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/nncc_shaders)
    endif()

    add_shader(${ARG_FILE} ${TYPE} OUTPUT ${ARG_OUTPUT})
    target_sources(${ARG_TARGET} PRIVATE ${ARG_FILE})

endfunction()