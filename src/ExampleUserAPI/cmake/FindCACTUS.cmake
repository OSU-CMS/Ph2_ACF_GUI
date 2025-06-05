# - Try to find cactus

cmake_policy(PUSH)

# Avoid false warning about INTERFACE IMPORTS (bug in some cmake versions
IF (POLICY CMP1111)
cmake_policy(SET CMP1111 OLD)
ENDIF()


#macro(find_cactus_in_extern arg)
# find_path(CACTUS_ROOT uhal.hpp
#    HINTS ${PROJECT_SOURCE_DIR}/extern/cactus ${arg})
#endmacro()

#find_cactus_in_extern("")

#set(CACTUS_ROOT ${PROJECT_SOURCE_DIR}/extern/cactus )

file(GLOB_RECURSE uhal_include $ENV{CACTUSROOT}/include/*uhal.hpp)
if(uhal_include)
    set(CACTUS_ROOT "$ENV{CACTUSROOT}")
    set(CACTUS_FOUND TRUE)
    #MESSAGE(STATUS "Found uHAL in ${CACTUS_ROOT}")
    file(GLOB_RECURSE amc13_include ${CACTUS_ROOT}/amc13/*AMC13.hh)
    if(amc13_include)
        set(CACTUS_AMC13_FOUND TRUE)
        #message(STATUS "Found AMC13 compoenent in ${CACTUS_ROOT}")
    else(amc13_include)
        set(CACTUS_AMC13_FOUND FALSE)
        #message(STATUS "Could NOT find AMC13 compoenent")
    endif(amc13_include)
else(uhal_include)
    file(GLOB_RECURSE extern_file ${PROJECT_SOURCE_DIR}/extern/*uhal.hpp)
    if (extern_file)
        # strip the file and 'include' path away:
        get_filename_component(extern_lib_path "${extern_file}" PATH)
        get_filename_component(extern_lib_path "${extern_lib_path}" PATH)
        get_filename_component(extern_lib_path "${extern_lib_path}" PATH)
        #MESSAGE(STATUS "Found CACTUS package in 'extern' subfolder: ${extern_lib_path}")
        set(CACTUS_ROOT ${extern_lib_path})
        set(CACTUS_FOUND TRUE)
    endif(extern_file)
endif(uhal_include)


# could not find the package at the usual locations -- try to copy from AFS if accessible
if (NOT CACTUS_ROOT)
    MESSAGE(ERROR "Could not find CACTUS package required by Ph2_ACF. Please refer to the documentation on how to obtain the software.")
  set(CACTUS_FOUND FALSE)
endif()

set(CACTUS_LIBDIR "${CACTUS_ROOT}/lib")
set(CACTUS_INCLUDEDIR "${CACTUS_ROOT}/include")

#set(EXTERN_ERLANG_PREFIX ${CACTUS_ROOT}/lib/erlang )
#set(EXTERN_ERLANG_BIN_PREFIX ${EXTERN_ERLANG_PREFIX}/bin )

#set(EXTERN_BOOST_PREFIX ${CACTUS_ROOT} )
#set(EXTERN_BOOST_INCLUDE_PREFIX ${EXTERN_BOOST_PREFIX}/include )
#set(EXTERN_BOOST_LIB_PREFIX ${EXTERN_BOOST_PREFIX}/lib )

#set(EXTERN_PUGIXML_PREFIX ${CACTUS_ROOT}  )
#set(EXTERN_PUGIXML_INCLUDE_PREFIX ${EXTERN_PUGIXML_PREFIX}/include/pugixml )
#set(EXTERN_PUGIXML_LIB_PREFIX ${EXTERN_PUGIXML_PREFIX}/lib )

#FIXME: are these variables needed?

set(UHAL_GRAMMARS_PREFIX ${CACTUS_ROOT} )
set(UHAL_GRAMMARS_INCLUDE_PREFIX ${CACTUS_INCLUDEDIR} )
set(UHAL_GRAMMARS_LIB_PREFIX ${CACTUS_LIBDIR} )

set(UHAL_LOG_PREFIX ${CACTUS_ROOT} )
set(UHAL_LOG_INCLUDE_PREFIX ${CACTUS_INCLUDEDIR} )
set(UHAL_LOG_LIB_PREFIX ${CACTUS_LIBDIR} )

set(UHAL_UHAL_PREFIX ${CACTUS_ROOT} )
set(UHAL_UHAL_INCLUDE_PREFIX ${CACTUS_INCLUDEDIR} )
set(UHAL_UHAL_LIB_PREFIX ${CACTUS_LIBDIR} )


######################
# Add targets for uhal
######################
if (CACTUS_FOUND)
    add_library(CACTUS::headers INTERFACE IMPORTED)
    set_target_properties(CACTUS::headers PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${CACTUS_INCLUDEDIR}"
        INTERFACE_LINK_LIBRARIES "Boost::boost"
    )

    macro ( add_lib_target uhal_name uhal_suffix )
        set(CACTUS_LIBS ${CACTUS_LIBS} CACTUS_UHAL_${uhal_name})
        find_library (CACTUS_UHAL_${uhal_name}
            NAMES "cactus_uhal_${uhal_suffix}"
            PATHS "${CACTUS_LIBDIR}"
             )
        if ( CACTUS_UHAL_${uhal_name} )
            mark_as_advanced(CACTUS_UHAL_${uhal_name})
            add_library(CACTUS::${uhal_name} SHARED IMPORTED)
            set_target_properties(CACTUS::${uhal_name} PROPERTIES
                IMPORTED_LOCATION "${CACTUS_UHAL_${uhal_name}}"
                INTERFACE_LINK_DIRECTORIES "${CACTUS_LIBDIR}"
                INTERFACE_LINK_LIBRARIES "CACTUS::headers;$<$<STREQUAL:${uhal_suffix},uhal>:CACTUS::uhal_grammars>"
                )
        else()
            message(ERROR "Library ${uhal_name} not found")
        endif()
    endmacro()

    add_lib_target(uhal_grammars grammars)
    add_lib_target(uhal uhal)
    add_lib_target(uhal_log log)
endif()


if(${CACTUS_AMC13_FOUND})
    set(UHAL_AMC13_PREFIX ${CACTUS_ROOT})
    set(UHAL_AMC13_INCLUDE_PREFIX $ENV{CACTUSINCLUDE}/amc13 )
    set(UHAL_AMC13_LIB_PREFIX $ENV{CACTUSLIB} )
endif(${CACTUS_AMC13_FOUND})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZESTSC1_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CACTUS  DEFAULT_MSG CACTUS_ROOT ${CACTUS_LIBS})
find_package_handle_standard_args(AMC13 REQUIRED_VARS CACTUS_AMC13_FOUND NAME_MISMATCHED)

mark_as_advanced(CACTUS_ROOT UHAL_AMC13_PREFIX)
unset(CACTUS_LIBS)

cmake_policy(POP)
