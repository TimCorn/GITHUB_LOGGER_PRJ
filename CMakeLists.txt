
cmake_minimum_required(VERSION 2.8)
project(cmake-Logger)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.67 COMPONENTS filesystem REQUIRED)


set(SRC_PATH  src/)
set(BLD_PATH  build/)
set(PROJECT_NAME main_test )
set(LIBRARY_NAME Logger )
#set(WANTED_LIBRARIES  -lpthread  -ggdb3 )
set(WANTED_LIBRARIES  -lpthread)
set(Boost_LIBRARIES    -lboost_filesystem -lboost_system)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")



set(LIB_SRC_LIST ${SRC_PATH}Logger.cpp)
set(SRC_LIST ${SRC_PATH}main_test.cpp)

add_executable(${PROJECT_NAME} ${SRC_LIST})
add_library(${LIBRARY_NAME} SHARED ${LIB_SRC_LIST})
set_property(TARGET ${LIBRARY_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)


target_link_libraries(${PROJECT_NAME} ${LIBRARY_NAME} ${WANTED_LIBRARIES})
target_link_libraries(${LIBRARY_NAME}  ${WANTED_LIBRARIES} ${Boost_LIBRARIES})



add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy
                          "${CMAKE_SOURCE_DIR}/${SRC_PATH}test_all.sh"  "${CMAKE_BINARY_DIR}")


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                     COMMAND ${CMAKE_COMMAND} -E copy
                          "${CMAKE_SOURCE_DIR}/${SRC_PATH}*.cfg"  "${CMAKE_BINARY_DIR}"
                      COMMENT "COPY WANTED FILES TO build-dir.")




