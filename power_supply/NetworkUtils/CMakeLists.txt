if(NOT DEFINED ENV{OTSDAQ_CMSOUTERTRACKER_DIR})


MESSAGE(STATUS " ")
MESSAGE(STATUS "    ${BoldYellow}========================================================================================================${Reset}") 
MESSAGE(STATUS "    ${BoldYellow}MIDDLEWARE${Reset} [stand-alone/middleware]: [${BoldCyan}Ph2_ACF/NetworkUtils/CMakeLists.txt${Reset}]. ${BoldRed}Begin...${Reset}") 
MESSAGE(STATUS " ")

#includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Boost also needs to be linked
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})
set(LIBS ${LIBS} ${Boost_REGEX_LIBRARY})

#find source files
file(GLOB SOURCES *.cc)
add_library(NetworkUtils STATIC ${SOURCES})


message("--     ${BoldCyan}#### End ####${Reset}")

MESSAGE(STATUS " ")
MESSAGE(STATUS "    ${BoldYellow}MIDDLEWARE${Reset} [stand-alone/middleware]: [${BoldCyan}Ph2_ACF/NetworkUtils/CMakeLists.txt${Reset}]. ${BoldGreen}DONE!${Reset}") 
MESSAGE(STATUS "    ${BoldBlue}========================================================================================================${Reset}") 
MESSAGE(STATUS " ")

else()

cet_set_compiler_flags(
 EXTRA_FLAGS -Wno-reorder -Wl,--undefined -D__OTSDAQ__
)
 
cet_make(LIBRARY_NAME NetworkUtils_${Ph2_ACF_Master}
        LIBRARIES
        )

install_headers()
install_source()
endif()


