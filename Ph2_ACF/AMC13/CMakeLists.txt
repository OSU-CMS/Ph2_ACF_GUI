#includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${UHAL_AMC13_INCLUDE_PREFIX})
#library directory
link_directories(${UHAL_AMC13_LIB_PREFIX})

set(LIBS ${LIBS} cactus_amc13_amc13 CACTUS::uhal)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__AMC13__")

#find source files
file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cc)

#add the library
add_library(Ph2_Amc13 STATIC ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(Ph2_Amc13 ${LIBS})
