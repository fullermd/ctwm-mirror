cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Our base set of sources
set(CTWMSRC
	gram.tab.c lex.c deftwmrc.c add_window.c gc.c list.c ctwm.c
	parse.c menus.c events.c resize.c util.c version.c iconmgr.c
	cursor.c icons.c workmgr.c windowbox.c clicktofocus.c
	vscreen.c session.c mwmhints.c
)

# Libs
set(CTWMLIBS)


#
# Define options
# These can be set at the command line; e.g., "cmake -DUSE_JPEG=OFF"
#
option(USE_XPM   "Enable XPM support"                 ON )
option(USE_JPEG  "Enable libjpeg support"             ON )
option(USE_M4    "Enable m4 support"                  ON )
option(USE_SOUND "Enable librplay sound support"      OFF)
option(USE_GNOME "Enable GNOME1 support"              ON )


#
# First find various stuff we need, if enabled.
#

# First things first.  If we don't have X, we're going nowhere.
find_package(X11)
if(NOT X11_FOUND)
	# This just isn't feasible...
	message(FATAL_ERROR "Can't find X libs.")
endif(NOT X11_FOUND)

include_directories(${X11_INCLUDE_DIR})
list(APPEND CTWMLIBS ${X11_LIBRARIES})
list(APPEND CTWMLIBS ${X11_Xmu_LIB})
list(APPEND CTWMLIBS ${X11_Xt_LIB})

if(USE_XPM)
	if(NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "Couldn't find XPM libs")
	endif(NOT X11_Xpm_FOUND)

	list(APPEND CTWMLIBS ${X11_Xpm_LIB})
	add_definitions(-DXPM)
	set_property(SOURCE parse.c ctwm.c APPEND
		PROPERTY COMPILE_DEFINITIONS PIXMAP_DIRECTORY="${PIXMAPDIR}"
	)
	message(STATUS "Enabling XPM support.")
endif(USE_XPM)

set(TWMDIR ${X11_LIBRARY_DIR}/X11/twm)
set(CONFDIR ${TWMDIR})
# This actually only has meaning if we're building with XPM
set(PIXMAPDIR ${TWMDIR}/images)


# libjpeg is pretty common
if(USE_JPEG)
	find_package(JPEG)
	if(NOT JPEG_FOUND)
		message(FATAL_ERROR "Couldn't find libjpeg")
	endif()

	include_directories(${JPEG_INCLUDE_DIR})
	list(APPEND CTWMLIBS ${JPEG_LIBRARIES})
	add_definitions(-DJPEG)
	message(STATUS "Enabling libjpeg support.")
endif(USE_JPEG)


# m4 is on by default too
if(USE_M4)
	if(NOT M4_CMD)
		find_program(M4_CMD m4)
	endif(NOT M4_CMD)
	if(NOT M4_CMD)
		message(FATAL_ERROR "Can't find m4 program: try setting M4_CMD.")
	endif(NOT M4_CMD)
	add_definitions(-DUSEM4)
	set_property(SOURCE parse.c APPEND
		PROPERTY COMPILE_DEFINITIONS M4CMD="${M4_CMD}"
	)
	message(STATUS "Enabling m4 support.")
endif(USE_M4)


# rplay off by default
if(USE_SOUND)
	find_library(LIBRPLAY NAMES rplay)
	if(NOT LIBRPLAY)
		message(FATAL_ERROR "Can't find librplay.")
	endif(NOT LIBRPLAY)

	list(APPEND CTWMSRC sound.c)
	add_definitions(-DSOUNDS)
	list(APPEND CTWMLIBS ${LIBRPLAY})
	message(STATUS "Enabling librplay sound support.")
endif(USE_SOUND)


# Do build Gnome support bits by default
if(USE_GNOME)
	list(APPEND CTWMSRC gnome.c)
	add_definitions(-DGNOME)
	message(STATUS "Enabling GNOME1 support.")
endif(USE_GNOME)


#
# Next find some build tools
#
# flex should be everywhere
find_package(FLEX)
if(FLEX_FOUND)
	FLEX_TARGET(ctwm_lexer lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.c)
else()
	message(FATAL_ERROR "Can't find flex.")
endif(FLEX_FOUND)

# This only finds bison, not yacc.  There doesn't seem to be a standard
# module for yacc, so hand-code it.
find_package(BISON)
if(BISON_FOUND)
	BISON_TARGET(ctwm_parser gram.y ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
		COMPILE_FLAGS -d -b gram)
else()
	find_program(YACC yacc)
	if(NOT YACC)
		message(FATAL_ERROR "Can't find yacc.")
	endif(NOT YACC)
	add_custom_command(OUTPUT gram.tab.c
		DEPENDS gram.y
		COMMAND ${YACC} -d -b gram ${CMAKE_CURRENT_SOURCE_DIR}/gram.y
	)
endif(BISON_FOUND)



#
# Now setup flags etc.
#

# Standard defs
set(STD_WARNS
	-Wall
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef
	-Wredundant-decls -Wcast-align -Wcast-qual -Wchar-subscripts
	-Winline -Wnested-externs -Wmissing-declarations
)
if(NOT NO_WARNS)
	add_definitions(${STD_WARNS})
endif(NOT NO_WARNS)


# Include source and generated files
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


# Extra flags for some files
set_property(SOURCE parse.c APPEND
	PROPERTY COMPILE_DEFINITIONS SYSTEM_INIT_FILE="${CONFDIR}/system.ctwmrc"
)



# Hand-build deftwmrc.c
set(defc deftwmrc.c)
add_custom_command(OUTPUT ${defc}
	DEPENDS system.ctwmrc
	COMMAND echo '/*' > ${defc}
	COMMAND echo ' * This file is generated automatically from the default' >> ${defc}
	COMMAND echo ' * twm bindings file system.ctwmrc by the twm Imakefile.' >> ${defc}
	COMMAND echo '  */' >> ${defc}
	COMMAND echo '' >> ${defc}
	COMMAND echo 'char *defTwmrc[] = {' >> ${defc}
	COMMAND sed -e "/^#/d" -e 's/\"/\\\\\"/g' -e 's/^/    \"/' -e 's/$$/\",/' ${CMAKE_CURRENT_SOURCE_DIR}/system.ctwmrc >> ${defc}
	COMMAND echo '    (char *) 0 }\;' >> ${defc}
)


# We're building ctwm
add_executable(ctwm ${CTWMSRC})
target_link_libraries(ctwm ${CTWMLIBS})


# Allow building the libctwm stuff too.  Don't do it by default though.
# The old imake setup built libctwm, but not demolib, and it doesn't
# serve any purpose at the moment.  Maybe we can do something with it in
# the future.
if(DO_LIBCTWM)
	add_library(ctwmc SHARED libctwm.c)
	add_executable(demolib demolib.c)
	target_link_libraries(demolib ctwmc ${CTWMLIBS})
endif(DO_LIBCTWM)


# Compress man page
find_program(GZIP_CMD gzip)
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.gz)
add_custom_command(OUTPUT ${MANPAGE}
	DEPENDS ctwm.man
	COMMAND ${GZIP_CMD} -nc ${CMAKE_CURRENT_SOURCE_DIR}/ctwm.man > ${MANPAGE}
	COMMENT "Building ctwm.1"
)
add_custom_target(man ALL DEPENDS ${MANPAGE})


# Installing everything
INSTALL(TARGETS ctwm
	DESTINATION bin
)
if(USE_XPM)
	INSTALL(DIRECTORY xpm/
		DESTINATION ${PIXMAPDIR}
	)
endif(USE_XPM)
INSTALL(FILES system.ctwmrc
	DESTINATION ${CONFDIR}
)
INSTALL(FILES ${MANPAGE}
	DESTINATION man/man1
)
