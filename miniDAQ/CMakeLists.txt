if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MINI DAQ${Reset} [stand-alone/middleware]: [${BoldCyan}Ph2_ACF/miniDAQ/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    # Includes
    include_directories(${PROJECT_SOURCE_DIR})

    # Library dirs
    link_directories(${PROJECT_SOURCE_DIR/lib})

    # Initial set of libraries
    set(LIBS ${LIBS} Ph2_Description Ph2_Interface Ph2_Utils Ph2_System NetworkUtils MessageUtils CACTUS::uhal)

    # Check for ZMQ installed
    if(ZMQ_FOUND)
        #here, now check for UsbInstLib
        if(PH2_USBINSTLIB_FOUND)

            #add include directoreis for ZMQ and USBINSTLIB
            include_directories(${PH2_USBINSTLIB_INCLUDE_DIRS})
            link_directories(${PH2_USBINSTLIB_LIBRARY_DIRS})
            include_directories(${ZMQ_INCLUDE_DIRS})

            #and link against the libs
            set(LIBS ${LIBS} ${ZMQ_LIBRARIES} ${PH2_USBINSTLIB_LIBRARIES})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{ZmqFlag} $ENV{USBINSTFlag}")
        endif()
    endif()

    # Check for AMC13 libraries
    if(${CACTUS_AMC13_FOUND})
        include_directories(${PROJECT_SOURCE_DIR}/AMC13)
        include_directories(${UHAL_AMC13_INCLUDE_PREFIX})
        link_directories(${UHAL_AMC13_LIB_PREFIX})
        set(LIBS ${LIBS} cactus_amc13_amc13 Ph2_Amc13)
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{Amc13Flag}")
    endif()

    # Check for AntennaDriver
    if(${PH2_ANTENNA_FOUND})
        include_directories(${PH2_ANTENNA_INCLUDE_DIRS})
        link_directories(${PH2_ANTENNA_LIBRARY_DIRS})
        set(LIBS ${LIBS} usb ${PH2_ANTENNA_LIBRARIES})
        set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{AntennaFlag}")
    endif()

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


    # Boost also needs to be linked
    #  include_directories(${Boost_INCLUDE_DIRS})
    # link_directories(${Boost_LIBRARY_DIRS})

    set(LIBS ${LIBS} ${Boost_IOSTREAMS_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY})

    # Find source files
    file(GLOB SOURCES *.cc)
    add_library(Ph2_miniDAQ STATIC ${SOURCES})
    target_link_libraries(Ph2_miniDAQ Ph2_Tools CACTUS::headers)

    ###############
    # EXECUTABLES #
    ###############

    # RunController
    if(CompileForHerd)
        add_executable(RunController RunController.cc MiddlewareController.cc MiddlewareMessageHandler.cc MiddlewareStateMachine.cc CombinedCalibrationFactory.cc)
        target_link_libraries(RunController ${LIBS} Ph2_Tools Ph2_System Ph2_miniDAQ MessageUtils)
    endif()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MINI DAQ${Reset} [stand-alone/middleware]: [${BoldCyan}Ph2_ACF/miniDAQ/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else() # ------------------------------- Compilation in the otsdaq environment ---------------------------------------------

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}MINI DAQ${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/miniDAQ/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")


    #Include dirs
    include_directories(${Protobuf_INCLUDE_DIRS})

    #Library dirs
    link_directories(${Protobuf_LIBRARIES})

    cet_make(LIBRARY_NAME Ph2_MiniDAQ
    SOURCES
    MiddlewareController.cc MiddlewareMessageHandler.cc MiddlewareStateMachine.cc CombinedCalibrationFactory.cc
    LIBRARIES
    pthread
    ${Boost_SYSTEM_LIBRARY}
)

    cet_make_exec(NAME RunController
        SOURCE
        RunController.cc
        LIBRARIES
        Ph2_MiniDAQ
        Ph2_Utils
        NetworkUtils
        Ph2_MessageUtils
        protobuf
        Boost::iostreams
        Boost::filesystem
        Boost::regex
        Boost::program_options
        Boost::headers
        CACTUS::uhal
        Ph2_Interface
        Ph2_System
        Ph2_Tools
        Ph2_MonitorUtils
    )

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}MINI DAQ${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/miniDAQ/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
