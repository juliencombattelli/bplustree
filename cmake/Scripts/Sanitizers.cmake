cmake_minimum_required(VERSION 3.13)

function(target_enable_sanitizers TARGET_NAME)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(SUPPORTED_SANITIZERS "address" "memory" "undefined" "thread")
    else()
        set(SUPPORTED_SANITIZERS "")
        message(VERBOSE "Sanitizers not supported for compiler of type ${CMAKE_CXX_COMPILER_ID}")
    endif()

    if (NOT TARGET ${TARGET_NAME})
        message(FATAL_ERROR "${TARGET_NAME} is not a valid target")
    endif()

    set(SANITIZERS "")

    foreach(sanitizer IN LISTS SUPPORTED_SANITIZERS)
        option(BPLUSTREE_SANITIZE_${TARGET_NAME}_${sanitizer} "Enable sanitizer '${sanitizer}' for target ${TARGET_NAME}" OFF)
        if (BPLUSTREE_SANITIZE_${TARGET_NAME}_${sanitizer})
            list(APPEND SANITIZERS "${sanitizer}")
        endif()
    endforeach()

    list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

    if(LIST_OF_SANITIZERS)
        if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
            get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
            if(${TARGET_TYPE} STREQUAL INTERFACE_LIBRARY)
                set(COMPILE_OPTIONS_SCOPE INTERFACE)
            else()
                set(COMPILE_OPTIONS_SCOPE PUBLIC)
            endif()
            target_compile_options(${TARGET_NAME} ${COMPILE_OPTIONS_SCOPE} -fsanitize=${LIST_OF_SANITIZERS})
            target_link_options(${TARGET_NAME} ${COMPILE_OPTIONS_SCOPE} -fsanitize=${LIST_OF_SANITIZERS})
        endif()
    endif()

endfunction()
