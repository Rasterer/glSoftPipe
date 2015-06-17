CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#SET(TESTPIPE_SRC test_vs_example.cpp)
SET(TESTSHADER_SRC test_vertex_shader.cpp)
SET(TEST_NAMESPACE_SRC test_name_space.cpp)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lX11")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/OpenGL/core)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/include)

#ADD_EXECUTABLE(test_softpipe ${TESTPIPE_SRC})
ADD_EXECUTABLE(test_shader ${TESTSHADER_SRC})
ADD_EXECUTABLE(test_name_space ${TEST_NAMESPACE_SRC})

#TARGET_LINK_LIBRARIES(test_softpipe glsp ws_offscreen)
#ADD_DEPENDENCIES(test_softpipe glsp ws_offscreen)

# XCB
FIND_PACKAGE(XCB)
IF(XCB_FOUND)
	MESSAGE(STATUS "Looking for XCB - found")
ELSE(XCB_FOUND)
	MESSAGE(STATUS "Looking for XCB - not found")
ENDIF(XCB_FOUND)

INCLUDE_DIRECTORIES(${XCB_INCLUDE_PATH}
					${XLIB_XCB_INCLUDE_PATH})

TARGET_LINK_LIBRARIES(test_shader
					  glsp_ogl
					  glsp_egl
					  ${XCB_LIBRARY}
					  ${XLIB_XCB_LIBRARY}
					  ${XCB_DRI2_LIBRARY})

ADD_DEPENDENCIES(test_shader glsp_egl glsp_ogl)

TARGET_LINK_LIBRARIES(test_name_space glsp_ogl)
ADD_DEPENDENCIES(test_name_space glsp_ogl)


FIND_PACKAGE(ImageMagick COMPONENTS Magick++)

IF(ImageMagick_Magick++_FOUND)
	INCLUDE_DIRECTORIES(${ImageMagick_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(test_shader ${ImageMagick_LIBRARIES})
ELSE(ImageMagick_Magick++_FOUND)
	MESSAGE(FATAL_ERROR "ImageMagick package not found!")
ENDIF(ImageMagick_Magick++_FOUND)
