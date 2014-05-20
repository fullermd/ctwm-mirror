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
# First find various stuff we need, if enabled.
#

# First things first.  If we don't have X, we're going nowhere.
find_package(X11)
if(NOT X11_FOUND)
	# This just isn't feasible...
	message(FATAL_ERROR "Can't find X libs.")
endif(NOT X11_FOUND)

include_directories(${X11_INCLUDE_DIR})
set(CTWMLIBS ${CTWMLIBS} ${X11_LIBRARIES})
set(CTWMLIBS ${CTWMLIBS} ${X11_Xmu_LIB})
set(CTWMLIBS ${CTWMLIBS} ${X11_Xt_LIB})
if(X11_Xpm_FOUND AND NOT NO_XPM)
	set(CTWMLIBS ${CTWMLIBS} ${X11_Xpm_LIB})
	add_definitions(-DXPM)
endif(X11_Xpm_FOUND AND NOT NO_XPM)

set(TWMDIR ${X11_LIBRARY_DIR}/X11/twm)
set(CONFDIR ${TWMDIR})
set(PIXMAPDIR ${TWMDIR}/images)


# libjpeg is pretty common
find_package(JPEG)
if(JPEG_FOUND AND NOT NO_JPEG)
	include_directories(${JPEG_INCLUDE_DIR})
	set(CTWMLIBS ${CTWMLIBS} ${JPEG_LIBRARIES})
	add_definitions(-DJPEG)
endif(JPEG_FOUND AND NOT NO_JPEG)


# m4 is on by default too
if(NOT NO_M4)
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
endif(NOT NO_M4)


# rplay off by default
if(USE_SOUND)
	set(CTWMSRC ${CTWMSRC} sound.c)
	add_definitions(-DSOUNDS)
	find_library(LIBRPLAY NAMES rplay)
	if(NOT LIBRPLAY)
		message(FATAL_ERROR "Can't find librplay.")
	endif(NOT LIBRPLAY)
	set(CTWMLIBS ${CTWMLIBS} ${LIBRPLAY})
endif(USE_SOUND)


# Do build Gnome support bits by default
if(NOT NO_GNOME)
	set(CTWMSRC ${CTWMSRC} gnome.c)
	add_definitions(-DGNOME)
endif(NOT NO_GNOME)


#
# Next find some build tools
#
# flex shoudl be everywhere
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
set_property(SOURCE parse.c ctwm.c APPEND
	PROPERTY COMPILE_DEFINITIONS PIXMAP_DIRECTORY="${PIXMAPDIR}"
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
INSTALL(DIRECTORY xpm/
	DESTINATION ${PIXMAPDIR}
)
INSTALL(FILES system.ctwmrc
	DESTINATION ${CONFDIR}
)
INSTALL(FILES ${MANPAGE}
	DESTINATION man/man1
)
