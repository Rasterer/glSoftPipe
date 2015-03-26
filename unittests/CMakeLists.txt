CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(TESTPIPE_SRC test_vs_example.cpp)
SET(TEST_NAMESPACE_SRC test_name_space.cpp)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/core)

ADD_EXECUTABLE(test_softpipe ${TESTPIPE_SRC})
ADD_EXECUTABLE(test_name_space ${TEST_NAMESPACE_SRC})

TARGET_LINK_LIBRARIES(test_softpipe glsp ws_offscreen)
ADD_DEPENDENCIES(test_softpipe glsp ws_offscreen)

TARGET_LINK_LIBRARIES(test_name_space glsp ws_offscreen)
ADD_DEPENDENCIES(test_name_space glsp ws_offscreen)
