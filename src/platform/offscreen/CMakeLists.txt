CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(WS_OS_SRC os_output.cpp)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(ws_offscreen STATIC ${WS_OS_SRC})

FIND_PACKAGE(ImageMagick COMPONENTS Magick++)

IF(ImageMagick_Magick++_FOUND)
	INCLUDE_DIRECTORIES(${ImageMagick_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(ws_offscreen ${ImageMagick_LIBRARIES})
ELSE(ImageMagick_Magick++_FOUND)
	MESSAGE(FATAL_ERROR "ImageMagick package not found!")
ENDIF(ImageMagick_Magick++_FOUND)
