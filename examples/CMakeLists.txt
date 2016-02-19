CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

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


ADD_SUBDIRECTORY(LoadModel)
ADD_SUBDIRECTORY(ShadowMap)
ADD_SUBDIRECTORY(CrytekSponza)
ADD_SUBDIRECTORY(AlphaBlend)
