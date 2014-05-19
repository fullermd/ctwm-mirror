cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Various standard sources
set(CTWMSRC
	gram.tab.c lex.c deftwmrc.c add_window.c gc.c list.c ctwm.c
	parse.c menus.c events.c resize.c util.c version.c iconmgr.c
	cursor.c icons.c workmgr.c windowbox.c clicktofocus.c
	vscreen.c session.c mwmhints.c
)

# Libs
set(CTWMLIBS)


# Do build Gnome stuff by default
if(NOT NO_GNOME)
	set(CTWMSRC ${CTWMSRC} gnome.c)
	add_definitions(-DGNOME)
endif(NOT NO_GNOME)

# Don't build sound stuff
if(USE_SOUND)
	set(CTWMSRC ${CTWMSRC} sound.c)
	add_definitions(-DSOUNDS)
	find_library(LIBRPLAY NAMES rplay)
	if(NOT LIBRPLAY)
		message(FATAL_ERROR "Can't find librplay.")
	endif(NOT LIBRPLAY)
	set(CTWMLIBS ${CTWMLIBS} ${LIBRPLAY})
endif(USE_SOUND)

# m4 by default
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


# Standard defs
add_definitions(
	-Wall
	-Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef
	-Wredundant-decls -Wcast-align -Wcast-qual -Wchar-subscripts
	-Winline -Wnested-externs -Wmissing-declarations
)



# Include source and generated files
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Better have X
find_package(X11)
if(X11_FOUND)
	include_directories(${X11_INCLUDE_DIR})
	set(CTWMLIBS ${CTWMLIBS} ${X11_Xpm_LIB})
	set(CTWMLIBS ${CTWMLIBS} ${X11_LIBRARIES})
	set(CTWMLIBS ${CTWMLIBS} ${X11_Xmu_LIB})
	set(CTWMLIBS ${CTWMLIBS} ${X11_Xt_LIB})
	if(X11_Xpm_FOUND)
		set(CTWMLIBS ${CTWMLIBS} ${X11_Xpm_LIB})
		add_definitions(-DXPM)
	endif(X11_Xpm_FOUND)

	set(TWMDIR ${X11_LIBRARY_DIR}/X11/twm)
	set(CONFDIR ${TWMDIR})
	set(PIXMAPDIR ${TWMDIR}/images)
else(X11_FOUND)
	# This just isn't feasible...
	message(FATAL_ERROR "Can't find X libs.")
endif(X11_FOUND)

# Betcha we've got JPEG
find_package(JPEG)
if(JPEG_FOUND)
	include_directories(${JPEG_INCLUDE_DIR})
	set(CTWMLIBS ${CTWMLIBS} ${JPEG_LIBRARIES})
	add_definitions(-DJPEG)
endif(JPEG_FOUND)

# This should work everywhere
find_package(FLEX)
if(FLEX_FOUND)
	FLEX_TARGET(ctwm_lexer lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.c)
else(FLEX_FOUND)
	message(FATAL_ERROR "Can't find flex.")
endif(FLEX_FOUND)

# This only finds bison, not yacc.  There doesn't seem to be a standard
# module for yacc, so hand-code it.
find_package(BISON)
if(BISON_FOUND)
	BISON_TARGET(ctwm_parser gram.y ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
		COMPILE_FLAGS -d -b gram)
else(BISON_FOUND)
	find_program(YACC yacc)
	if(NOT YACC)
		message(FATAL_ERROR "Can't find yacc.")
	endif(NOT YACC)
	add_custom_command(OUTPUT gram.tab.c
		DEPENDS gram.y
		COMMAND ${YACC} -d -b gram ${CMAKE_CURRENT_SOURCE_DIR}/gram.y
	)
endif(BISON_FOUND)


# Magic to build deftwmrc.c
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


set_property(SOURCE parse.c APPEND
	PROPERTY COMPILE_DEFINITIONS SYSTEM_INIT_FILE="${CONFDIR}/system.ctwmrc"
)
set_property(SOURCE parse.c ctwm.c APPEND
	PROPERTY COMPILE_DEFINITIONS PIXMAP_DIRECTORY="${PIXMAPDIR}"
)


# We're building ctwm
add_executable(ctwm ${CTWMSRC})
target_link_libraries(ctwm ${CTWMLIBS})


# Allow building the libctwm stuff too.  Don't do it by default though.
# The old imake setup build libctwm, but not demolib, and it doesn't
# serve any purpose at the moment.  Maybe we can do something with it in
# the future.
if(DO_LIBCTWM)
	add_library(ctwmc SHARED libctwm.c)
	add_executable(demolib demolib.c)
	target_link_libraries(demolib ctwmc ${CTWMLIBS})
endif(DO_LIBCTWM)


# Setup man page
add_custom_command(OUTPUT ctwm.1.gz
	DEPENDS ctwm.man
	COMMAND gzip -nc ${CMAKE_CURRENT_SOURCE_DIR}/ctwm.man > ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1.gz
	COMMENT "Building ctwm.1"
)


# Installing stuff
INSTALL(TARGETS ctwm
	DESTINATION "bin"
)
INSTALL(DIRECTORY "xpm/"
	DESTINATION ${PIXMAPDIR}
)
INSTALL(FILES ctwm.1.gz
	DESTINATION ${INSTALL_MAN_DIR}/man1
)
