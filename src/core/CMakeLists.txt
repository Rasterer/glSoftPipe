CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PIPELINE_SRC
	NameSpace.cpp
	GLContext.cpp
	BufferObject.cpp
	Shader.cpp
	VertexArrayObject.cpp
	VertexCache.cpp
	PrimitiveProcessor.cpp
	Rasterizer.cpp
)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(glsp SHARED ${PIPELINE_SRC})

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/include/khronos)
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/include/export)
