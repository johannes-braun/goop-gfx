include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(FetchContent)

# FetchContent_Declare(
#     xorstr
#     GIT_REPOSITORY https://github.com/qis/xorstr.git
#     GIT_TAG 4063249258b8014575396817ee6428a60379cafb
# )
# FetchContent_GetProperties(xorstr)
# if(NOT xorstr_POPULATED)
#   # Fetch the content using previously declared details
#   FetchContent_Populate(xorstr)
# endif()

# find_path(XORSTR_PATH xorstr.h HINTS ${xorstr_SOURCE_DIR}/include REQUIRED)

set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
macro(make_source file prefix)
    set(INCLUDE_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/include)
    set(GEN_SRC_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/gen_src)
    set(SRC_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/src)

    get_filename_component(VAR_NAME ${file} NAME)
    get_filename_component(VAR_DIR ${file} DIRECTORY)
    string(REPLACE "." "_" VAR_NAME "${VAR_NAME}")
    string(REPLACE " " "_" VAR_NAME "${VAR_NAME}")
    string(TOLOWER "${VAR_NAME}" VAR_NAME)
    string(REPLACE "." "_" VAR_DIR "${VAR_DIR}")
    string(REPLACE " " "_" VAR_DIR "${VAR_DIR}")
    string(TOLOWER "${VAR_DIR}" VAR_DIR)
    file(TO_CMAKE_PATH "${VAR_DIR}" VAR_DIR)
    string(REPLACE "/" ";" VAR_DIR "${VAR_DIR}")
    string(JOIN "::" VAR_DIR ${prefix} ${VAR_DIR})
    set(FILE_REL ${file})
    file(TO_CMAKE_PATH "${FILE_REL}" FILE_REL)
    string(REPLACE "." "_" FILE_REL_ALL "${FILE_REL}")
    string(REPLACE " " "_" FILE_REL_ALL "${FILE_REL_ALL}")
    string(REPLACE "/" ";" FILE_REL_ALL "${FILE_REL_ALL}")
    string(JOIN "_" FILE_REL_ALL ${FILE_REL_ALL})
    string(TOLOWER "${FILE_REL_ALL}" FILE_REL_ALL)
    string(TOUPPER "${prefix}" UPPER_PREFIX)
    string(JOIN "_" VAR_UNDER ${prefix} ${FILE_REL_ALL})

    set(TEMPLATE_PATH ${CURRENT_DIR}/templates)
    configure_file(${TEMPLATE_PATH}/secure_string.hpp ${SRC_DIR_ROOT}/${prefix}/secure_string.hpp COPYONLY)

    set(LIBRARY_PREFIX ${prefix})
    set(LIBRARY_PREFIX_UPPER ${UPPER_PREFIX})
    set(SHADER_NAME ${VAR_NAME})
    set(SHADER_NAME_C ${VAR_UNDER})
    set(SHADER_NAMESPACE ${VAR_DIR})
    set(SHADER_HEADER ${FILE_REL})
    set(EMPLACEMENT_HINT @EMPLACE_SOURCE_HERE@)
    configure_file(${TEMPLATE_PATH}/shader.h ${INCLUDE_DIR_ROOT}/${prefix}/${FILE_REL}.h NEWLINE_STYLE UNIX)
    configure_file(${TEMPLATE_PATH}/shader.cpp ${GEN_SRC_DIR_ROOT}/${prefix}/${FILE_REL}.cpp.gen NEWLINE_STYLE UNIX)
    
    add_custom_command(
        OUTPUT ${SRC_DIR_ROOT}/${prefix}/${FILE_REL}.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_REL} ${GEN_SRC_DIR_ROOT}/${prefix}/${FILE_REL}.cpp.gen
        COMMAND ${CMAKE_COMMAND} 
            -DFROM_FILE=${CMAKE_CURRENT_SOURCE_DIR}/${FILE_REL} 
            -DOVER_FILE=${GEN_SRC_DIR_ROOT}/${prefix}/${FILE_REL}.cpp.gen 
            -DTO_FILE=${SRC_DIR_ROOT}/${prefix}/${FILE_REL}.cpp 
            -P "${CURRENT_DIR}/GenerateSource.cmake"
        COMMENT "Generating shader source string in ${FILE_REL}.cpp"
        VERBATIM
    )
endmacro(make_source)

macro(add_shader_package target_name lib_type)
    set(REQUIRED_ARG SHARED STATIC)
    if(NOT ${lib_type} IN_LIST REQUIRED_ARG)
        message(FATAL_ERROR "Library type must be STATIC or SHARED")
    endif()

    set(INCLUDE_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/include)
    set(GEN_SRC_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/gen_src)
    set(SRC_DIR_ROOT ${CMAKE_CURRENT_BINARY_DIR}/src)

    add_library(${target_name} ${lib_type})
    set_target_properties(${target_name} PROPERTIES DEBUG_POSTFIX "d")
    set_target_properties(${target_name} PROPERTIES RELWITHDEBINFO_POSTFIX "rd")
    set_target_properties(${target_name} PROPERTIES MINSIZEREL_POSTFIX "min")

    generate_export_header(${target_name})
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${target_name}_export.h DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/include/${target_name})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${target_name}_export.h DESTINATION include/${target_name})
    target_compile_features(${target_name} PUBLIC cxx_std_17)
    target_include_directories(${target_name} PUBLIC  $<INSTALL_INTERFACE:include> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
    target_include_directories(${target_name} PRIVATE ${SRC_DIR_ROOT}/${target_name})
    foreach(SOURCE ${ARGN})
        get_filename_component(EXTENSION ${SOURCE} LAST_EXT)
        set(SOURCE_NAME ${SOURCE})

        set(FILE_REL ${SOURCE})
        if(IS_ABSOLUTE ${FILE_REL})
            file(RELATIVE_PATH FILE_REL ${CMAKE_CURRENT_SOURCE_DIR} ${FILE_REL})
        endif()

        make_source(${FILE_REL} ${target_name})
        set_source_files_properties(
            ${SOURCE}
        PROPERTIES
            HEADER_FILE_ONLY ON
        )

        get_filename_component(SOURCE_DIR ${FILE_REL}.h DIRECTORY)

        target_sources(${target_name} PUBLIC $<INSTALL_INTERFACE:include/${target_name}/${FILE_REL}.h> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/${target_name}/${FILE_REL}.h>)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${target_name}/${FILE_REL}.h DESTINATION include/${target_name}/${SOURCE_DIR})
        target_sources(${target_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src/${target_name}/${FILE_REL}.cpp)
    endforeach()
    install(TARGETS ${target_name} EXPORT ${target_name}_config
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

    export(TARGETS ${target_name}
        NAMESPACE ${target_name}::
        FILE "${CMAKE_CURRENT_BINARY_DIR}/${target_name}_config.cmake"
    )
    install(EXPORT ${target_name}_config
        DESTINATION "cmake"
        NAMESPACE ${target_name}::
    )
    add_library(${target_name}::${target_name} ALIAS ${target_name})
endmacro(add_shader_package)