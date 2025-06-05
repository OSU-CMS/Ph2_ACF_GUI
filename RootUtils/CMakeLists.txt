if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}ROOT UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/RootUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Library dirs

    # Boost also needs to be linked
    #    include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_FILESYSTEM_LIBRARY} ${Boost_ITERATOR_LIBRARY})

    # Find source files
    file(GLOB HEADERS *.h)
    file(GLOB SOURCES *.cc)

    # Add the library
    add_library(Ph2_RootUtils STATIC ${SOURCES} ${HEADERS})

    # Find root and link against it
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
        if(NoDataShipping)
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")
        endif()

        #check for THttpServer
        if(${ROOT_HAS_HTTP})
            set(LIBS ${LIBS} ${ROOT_RHTTP_LIBRARY})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{HttpFlag}")
        endif()
    endif()

    set(LIBS ${LIBS} Ph2_Utils )
    TARGET_LINK_LIBRARIES(Ph2_RootUtils ${LIBS})

    ####################################
    ## EXECUTABLES
    ####################################

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/RootUtils *.cc)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")
    foreach( sourcefile ${BINARIES} )
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})
    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}ROOT UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/RootUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}ROOT UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/RootUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    cet_make(LIBRARY_NAME Ph2_RootUtils
            LIBRARIES
            pthread
            ${Boost_SYSTEM_LIBRARY}
	        ROOT::Core
    )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}ROOT UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/RootUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
