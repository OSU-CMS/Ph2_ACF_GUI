if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}SYSTEM${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/System/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Library directories

    # Boost also needs to be linked
    # include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_ITERATOR_LIBRARY})

    # Find root and link against it
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
    endif()

    # Find source files
    file(GLOB HEADERS *.h)
    file(GLOB SOURCES *.cc)

    # Add the library
    add_library(Ph2_System STATIC ${SOURCES} ${HEADERS})
    set(LIBS ${LIBS} Ph2_Description Ph2_Interface Ph2_Utils Ph2_MonitorUtils NetworkUtils Ph2_Parser CACTUS::headers)
    if(NoDataShipping)
        set(LIBS ${LIBS} Ph2_MonitorDQM)
    endif()

    TARGET_LINK_LIBRARIES(Ph2_System ${LIBS})

    # Check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
      include_directories(${PH2_TCUSB_INCLUDE_DIRS})
      link_directories(${PH2_TCUSB_LIBRARY_DIRS})
      set(LIBS ${LIBS} ${PH2_TCUSB_LIBRARIES} usb)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif()

    # Check for TestCard USBDriver
    if(${PH2_TCUSB_FOUND})
      include_directories(${PH2_TCUSB_INCLUDE_DIRS})
      link_directories(${PH2_TCUSB_LIBRARY_DIRS})
      set(LIBS ${LIBS} ${PH2_TCUSB_LIBRARIES} usb)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{TCUSBFlag}")
    endif(${PH2_TCUSB_FOUND})

    if(NoDataShipping)
        if(${ROOT_FOUND})
            include_directories(${ROOT_INCLUDE_DIRS})
            set(LIBS ${LIBS} ${ROOT_LIBRARIES})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{UseRootFlag}")

            #check for THttpServer
            if(${ROOT_HAS_HTTP})
                set(LIBS ${LIBS} ${ROOT_RHTTP_LIBRARY})
                set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{HttpFlag}")
            endif()
        endif()
    endif()

    #################
    ## EXECUTABLES ##
    #################

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/System *.cc)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")
    foreach( sourcefile ${BINARIES} )
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})
    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}SYSTEM${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/System/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}SYSTEM${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/System/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    cet_make(LIBRARY_NAME Ph2_System
             LIBRARIES
             Ph2_Description
             Ph2_Interface
             Ph2_Utils
             Ph2_Parser
    )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}SYSTEM${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/System/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
