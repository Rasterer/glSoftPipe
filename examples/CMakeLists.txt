CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

AUX_SOURCE_DIRECTORY(./LoadModel/ LOADMODEL_SRC)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_GLEXT_PROTOTYPES -DMAGICKCORE_HDRI_ENABLE=0 -DMAGICKCORE_QUANTUM_DEPTH=16")
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/OpenGL/core)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/Common/include)

ADD_EXECUTABLE(load_model ${LOADMODEL_SRC})

# XCB
FIND_PACKAGE(XCB)
IF(XCB_FOUND)
	MESSAGE(STATUS      "Looking for XCB - found")
ELSE(XCB_FOUND)
	MESSAGE(FATAL_ERROR "Looking for XCB - not found")
ENDIF(XCB_FOUND)

FIND_PACKAGE(ImageMagick COMPONENTS Magick++)
IF(ImageMagick_Magick++_FOUND)
	MESSAGE(STATUS      "ImageMagick package - found!")
ELSE(ImageMagick_Magick++_FOUND)
	MESSAGE(FATAL_ERROR "ImageMagick package - not found!")
ENDIF(ImageMagick_Magick++_FOUND)

FIND_PACKAGE(assimp)
IF(assimp_FOUND)
	MESSAGE(STATUS      "Looking for assimp - found")
ELSE(assimp_FOUND)
	MESSAGE(FATAL_ERROR "Looking for assimp - not found")
ENDIF(assimp_FOUND)

FIND_PACKAGE(glm)
IF(glm_FOUND)
	MESSAGE(STATUS      "Looking for glm - found")
ELSE(glm_FOUND)
	MESSAGE(FATAL_ERROR "Looking for glm - not found")
ENDIF(glm_FOUND)

INCLUDE_DIRECTORIES(${XCB_INCLUDE_PATH}
					${XLIB_XCB_INCLUDE_PATH}
					${ImageMagick_INCLUDE_DIRS}
					${ASSIMP_INCLUDE_DIRS}
					${GLM_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(load_model
					  glsp_ogl
					  glsp_egl
					  X11
					  ${XCB_LIBRARY}
					  ${XLIB_XCB_LIBRARY}
					  ${XCB_DRI2_LIBRARY}
					  ${ImageMagick_LIBRARIES}
					  ${ASSIMP_LIBRARIES})

ADD_DEPENDENCIES(load_model glsp_egl glsp_ogl)
