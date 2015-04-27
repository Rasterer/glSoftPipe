#
# Try to find XCB library and include path.
# Once done this will define
#
# XCB_FOUND
# XCB_INCLUDE_PATH
# XLIB_XCB_INCLUDE_PATH
# XCB_LIBRARIES
# 

IF (UNIX)
	
	FIND_PATH( XCB_INCLUDE_PATH xcb/xcb.h
		/usr/include
		DOC "The directory where xcb/xcb.h resides")

	FIND_PATH( XLIB_XCB_INCLUDE_PATH X11/Xlib-xcb.h
		/usr/include
		DOC "The directory where X11/Xlib-xcb.h resides")
	

	FIND_LIBRARY( XCB_LIBRARY
		NAMES xcb
		PATHS
		/usr/lib
		DOC "The xcb library")

	FIND_LIBRARY( XCB_XLIB_LIBRARY
		NAMES xcb-xlib
		PATHS
		/usr/lib
		DOC "The xcb-xlib library")

	FIND_LIBRARY( XLIB_XCB_LIBRARY
		NAMES X11-xcb
		PATHS
		/usr/lib
		DOC "The X11-xcb library")

	FIND_LIBRARY( XCB_DRI2_LIBRARY
		NAMES xcb-dri2
		PATHS
		/usr/lib
		DOC "The xcb-dri2 library")

	SET(XCB_LIBRARIES ${XCB_LIBRARY} ${XCB_XLIB_LIBRARY} ${XLIB_XCB_LIBRARY} ${XCB_DRI2_LIBRARY})

ENDIF (UNIX)

IF (XCB_INCLUDE_PATH)
	SET( XCB_FOUND 1 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ELSE (XCB_INCLUDE_PATH)
	SET( XCB_FOUND 0 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ENDIF (XCB_INCLUDE_PATH)

MARK_AS_ADVANCED( XCB_FOUND )
