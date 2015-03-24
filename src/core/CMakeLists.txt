CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PIPELINE_SRC
	Shader.cpp
	vs_example.cpp
	PrimitiveProcessor.cpp
	Rasterizer.cpp
)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(softpipe SHARED ${PIPELINE_SRC})

#INCLUDE_DIRECTORIES(/usr/include/glm)
