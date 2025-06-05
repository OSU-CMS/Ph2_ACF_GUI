 if(NOT DEFINED ENV{OTSDAQ_CMSTRACKER_DIR})

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/Utils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    ############
    # Includes #
    ############
    include_directories(${PROJECT_SOURCE_DIR})

    ###########
    # Library #
    ###########

    #####################
    # Find source files #
    #####################
    file(GLOB HEADERS *.h)
    file(GLOB SOURCES *.cc)

    ###################
    # Add the library #
    ###################
    add_library(Ph2_Utils STATIC ${SOURCES} ${HEADERS})
    set(LIBS ${LIBS} pugixml boost_serialization boost_thread boost_date_time boost_iostreams boost_filesystem rt MessageUtils)
    TARGET_LINK_LIBRARIES(Ph2_Utils ${LIBS})

    #################################
    # Boost also needs to be linked #
    #################################
    set(LIBS ${LIBS} ${Boost_ITERATOR_LIBRARY})

    #################################
    # Find root and link against it #
    #################################
    if(${ROOT_FOUND})
        include_directories(${ROOT_INCLUDE_DIRS})
        set(LIBS ${LIBS} ${ROOT_LIBRARIES})
    endif()

    ###########################
    # Check for ZMQ installed #
    ###########################
    if(ZMQ_FOUND)
        if(PH2_USBINSTLIB_FOUND)
            include_directories(${PH2_USBINSTLIB_INCLUDE_DIRS})
            include_directories(${ZMQ_INCLUDE_DIRS})
            link_directories(${PH2_USBINSTLIB_LIBRARY_DIRS})

            set(LIBS ${LIBS} ${ZMQ_LIBRARIES} ${PH2_USBINSTLIB_LIBRARIES})
            TARGET_LINK_LIBRARIES(Ph2_Utils ${LIBS})
            set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} $ENV{ZmqFlag} $ENV{USBINSTFlag}")
        endif(PH2_USBINSTLIB_FOUND)
    else(ZMQ_FOUND)
            list(REMOVE_ITEM SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/UsbUtilities.cc)
            list(REMOVE_ITEM HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/UsbUtilities.h)
    endif(ZMQ_FOUND)

    ################
    ## EXECUTABLES #
    ################

    file(GLOB BINARIES RELATIVE ${PROJECT_SOURCE_DIR}/Utils *.cc *.cxx)

    message("--     ${BoldCyan}#### Building the following executables: ####${Reset}")
    foreach( sourcefile ${BINARIES} )
        string(REPLACE ".cc" "" name ${sourcefile})
        message(STATUS "    ${name}")
    endforeach(sourcefile ${BINARIES})
    message("--     ${BoldCyan}#### End ####${Reset}")

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}UTILS${Reset} [stand-alone]: [${BoldCyan}Ph2_ACF/Utils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

else()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS "    ${BoldYellow}UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/Utils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}")
    MESSAGE(STATUS " ")

    cet_make(LIBRARY_NAME Ph2_Utils
             LIBRARIES
             pthread
             Boost::serialization
             Boost::thread
             Boost::date_time
             Boost::iostreams
             Boost::filesystem
             EXCLUDE UsbUtilities.h UsbUtilities.cc
    )

    install_headers()
    install_source()

    MESSAGE(STATUS " ")
    MESSAGE(STATUS "    ${BoldYellow}UTILS${Reset} [otsdaq]: [${BoldCyan}Ph2_ACF/Utils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}")
    MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}")
    MESSAGE(STATUS " ")

endif()
