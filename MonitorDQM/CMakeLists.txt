if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR DQM${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/MonitorDQM/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_ITERATOR_LIBRARY})

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

    set(LIBS ${LIBS} Ph2_Utils Ph2_RootUtils Ph2_Parser)

    # Find source files
    file(GLOB SOURCES *.cc)
    add_library(Ph2_MonitorDQM STATIC ${SOURCES})
    target_link_libraries(Ph2_MonitorDQM ${LIBS})

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/RootUtils *.cc)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")
    foreach( sourcefile ${BINARIES} )
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})
    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR DQM${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/MonitorDQM/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR DQM${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/MonitorDQM/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")
    
    cet_make(LIBRARY_NAME Ph2_MonitorDQM
             LIBRARIES
	         ROOT::Core
    )

    install_headers()
    install_source()
    
    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MONITOR DQM${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/MonitorDQM/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
