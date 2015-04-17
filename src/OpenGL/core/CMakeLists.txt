CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(PIPELINE_SRC
	NameSpace.cpp
	GLContext.cpp
	BufferObject.cpp
	VertexArrayObject.cpp
	PipeStage.cpp
	DrawEngine.cpp
	VertexFetcher.cpp
	Shader.cpp
	PrimitiveAssembler.cpp
	Clipper.cpp
	PerspectiveDivider.cpp
	ScreenMapper.cpp
	FaceCuller.cpp
	Rasterizer.cpp
)

SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

ADD_LIBRARY(glsp SHARED ${PIPELINE_SRC})

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/src/include)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DPRIMITIVE_OWNS_VERTICES")
