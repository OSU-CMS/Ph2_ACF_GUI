#includes
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

#find source files
file(GLOB HEADERS include/*.h include/*.hh)
file(GLOB SOURCES src/*.cpp)

#add the library
add_library(RootWeb STATIC ${SOURCES} ${HEADERS})

if(${ROOT_FOUND})
    include_directories(${ROOT_INCLUDE_DIRS})
    #link_directories(${ROOT_LIBRARIES})
    set(LIBS ${LIBS} ${ROOT_LIBRARIES})
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ROOT_DEFINITIONS}")
endif()

if(${Boost_FOUND})
    #need to find local boost
    #    include_directories(${Boost_INCLUDE_DIRS})
    #link_directories(${Boost_LIBRARY_DIRS})
    set(LIBS ${LIBS} ${Boost_LIBRARIES})
endif()

TARGET_LINK_LIBRARIES(RootWeb ${LIBS})

