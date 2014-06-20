cmake_minimum_required(VERSION 2.6)
project(ctwm C)

# Our base set of sources
set(CTWMSRC
	add_window.c
	clicktofocus.c
	ctwm.c
	cursor.c
	deftwmrc.c
	events.c
	gc.c
	gram.tab.c
	iconmgr.c
	icons.c
	lex.c
	list.c
	menus.c
	mwmhints.c
	otp.c
	parse.c
	resize.c
	session.c
	util.c
	version.c
	vscreen.c
	windowbox.c
	workmgr.c
)

# Libs to link in (init empty list)
set(CTWMLIBS)


# Guard against in-tree builds
string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" insrc)
if(insrc)
	message(FATAL_ERROR "Please build out of tree; don't run cmake "
			"directly in the source tree.")
endif(insrc)


#
# Define options
# These can be set at the command line; e.g., "cmake -DUSE_JPEG=OFF"
#
option(USE_XPM    "Enable XPM support"                 ON )
option(USE_JPEG   "Enable libjpeg support"             ON )
option(USE_M4     "Enable m4 support"                  ON )
option(USE_SOUND  "Enable librplay sound support"      OFF)
option(USE_GNOME  "Enable GNOME1 support"              ON )
option(USE_IMCONV "Enable imconv support"              OFF)
option(USE_SREGEX "Use regex from libc"                ON )
option(USE_EWMH   "Support some Extended Window Manager Hints"  ON )


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


#
# Setup some search paths
#
set(INCSEARCH
	"${CMAKE_INSTALL_PREFIX}/include"
	${X11_INCLUDE_DIR}
	"/usr/local/include"
	"/usr/include"
)
set(LIBSEARCH
	"${CMAKE_INSTALL_PREFIX}/lib"
	${X11_LIBRARY_DIR}
	"/usr/local/lib"
	"/usr/lib"
	"/lib"
)


# Hard to imagine xpm not being around or somebody not wanting it, but...
if(USE_XPM)
	if(NOT X11_Xpm_FOUND)
		message(FATAL_ERROR "Couldn't find XPM libs")
	endif(NOT X11_Xpm_FOUND)

	list(APPEND CTWMLIBS ${X11_Xpm_LIB})
	add_definitions(-DXPM)
	set_property(SOURCE parse.c ctwm.c APPEND
		PROPERTY COMPILE_DEFINITIONS PIXMAP_DIRECTORY="${PIXMAPDIR}"
	)
	message(STATUS "Enabling XPM support: ${X11_Xpm_LIB}.")
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
		find_program(M4_CMD m4 gm4)
	endif(NOT M4_CMD)
	if(NOT M4_CMD)
		message(FATAL_ERROR "Can't find m4 program: try setting M4_CMD.")
	endif(NOT M4_CMD)
	add_definitions(-DUSEM4)
	set_property(SOURCE parse.c APPEND
		PROPERTY COMPILE_DEFINITIONS M4CMD="${M4_CMD}"
	)
	message(STATUS "Enabling m4 support (${M4_CMD}).")
endif(USE_M4)


# rplay off by default
if(USE_SOUND)
	find_library(LIBRPLAY NAMES rplay PATHS ${LIBSEARCH})
	if(NOT LIBRPLAY)
		message(FATAL_ERROR "Can't find librplay lib.")
	endif(NOT LIBRPLAY)
	find_path(LIBRPLAY_INCLUDE_DIR NAME rplay.h PATHS ${INCSEARCH})
	if(NOT LIBRPLAY_INCLUDE_DIR)
		message(FATAL_ERROR "Can't find rplay.h.")
	endif(NOT LIBRPLAY_INCLUDE_DIR)

	list(APPEND CTWMSRC sound.c)
	add_definitions(-DSOUNDS)
	list(APPEND CTWMLIBS ${LIBRPLAY})
	include_directories(${LIBRPLAY_INCLUDE_DIR})
	message(STATUS "Enabling librplay sound support.")
endif(USE_SOUND)


# Check if the user wants EWMH support built in.
if(USE_EWMH)
	list(APPEND CTWMSRC ewmh.c)
	add_definitions(-DEWMH)
	message(STATUS "Enabling Extended Window Manager Hints support.")
	set(USE_GNOME 0)
	message(STATUS "Disabling GNOME1 support.")
endif(USE_EWMH)


# Do build Gnome support bits by default
if(USE_GNOME)
	list(APPEND CTWMSRC gnome.c)
	add_definitions(-DGNOME)
	message(STATUS "Enabling GNOME1 support.")
endif(USE_GNOME)


# imconv is pretty historical, but until we dike it out...
if(USE_IMCONV)
	find_library(LIBIMCONV NAMES im PATHS ${LIBSEARCH})
	if(NOT LIBIMCONV)
		message(FATAL_ERROR "Can't find libim for imconv")
	endif(NOT LIBIMCONV)
	find_path(LIBIMCONV_INCLUDE_DIR NAME im.h PATHS ${INCSEARCH})
	if(NOT LIBIMCONV_INCLUDE_DIR)
		message(FATAL_ERROR "Can't find im.h for imconv")
	endif(NOT LIBIMCONV_INCLUDE_DIR)

	find_library(LIBSDSC NAMES sdsc PATHS ${LIBSEARCH})
	if(NOT LIBSDSC)
		message(FATAL_ERROR "Can't find libsdsc for imconv")
	endif(NOT LIBSDSC)
	find_path(LIBSDSC_INCLUDE_DIR NAME sdsc.h PATHS ${INCSEARCH})
	if(NOT LIBSDSC_INCLUDE_DIR)
		message(FATAL_ERROR "Can't find sdsc.h for imconv")
	endif(NOT LIBSDSC_INCLUDE_DIR)

	add_definitions(-DIMCONV)
	list(APPEND CTWMLIBS ${LIBIMCONV} ${LIBSDSC})
	include_directories(${LIBIMCONV_INCLUDE_DIR}  ${LIBSDSC_INCLUDE_DIR})
	message(STATUS "Enabling imconv support.")
endif(USE_IMCONV)


# This calls itself GNU but doesn't really have anything to do with GNU.
# All sorts of non-GNU systems include it.  Most/all nowadays, probably.
if(USE_SREGEX)
	include(CheckIncludeFiles)
	include(CheckFunctionExists)
	check_include_files(regex.h HAS_REGEX_H)
	check_function_exists(regexec HAS_REGEXEC)

	if(NOT HAS_REGEX_H)
		message(FATAL_ERROR "Can't find regex.h")
	endif(NOT HAS_REGEX_H)
	if(NOT HAS_REGEXEC)
		message(FATAL_ERROR "Can't find regexec()")
	endif(NOT HAS_REGEXEC)

	add_definitions(-DUSE_GNU_REGEX)
	message(STATUS "Enabling libc regex usage.")
endif(USE_SREGEX)


#
# Next find some build tools
#
# flex should be everywhere
find_package(FLEX)
if(FLEX_FOUND)
	FLEX_TARGET(ctwm_lexer lex.l ${CMAKE_CURRENT_BINARY_DIR}/lex.c)
else()
	# See if we have a pre-built one
	find_file(LEX_C lex.c
		PATHS ${CMAKE_CURRENT_SOURCE_DIR}
		NO_DEFAULT_PATH)
	if(LEX_C)
		# Make the build process just copy it in
		message(STATUS "No flex found, using prebuilt lex.c")
		add_custom_command(OUTPUT lex.c
			DEPENDS ${LEX_C}
			COMMAND cp ${LEX_C} .
		)
	else()
		# No flex, no pre-built lex.c
		message(FATAL_ERROR "Can't find flex.")
	endif(LEX_C)
endif(FLEX_FOUND)

# This only finds bison, not yacc.  There doesn't seem to be a standard
# module for yacc, so hand-code it.
set(YFLAGS -d -b gram)
if(DO_DEBUGPARSER)
	list(APPEND YFLAGS -t -v)
	add_definitions(-DYYEBUG=1)
	message(STATUS "Enabling config parser debug.")
endif(DO_DEBUGPARSER)
find_package(BISON)
if(BISON_FOUND)
	BISON_TARGET(ctwm_parser gram.y ${CMAKE_CURRENT_BINARY_DIR}/gram.tab.c
		COMPILE_FLAGS ${YFLAGS})
else()
	find_program(YACC yacc)
	if(YACC)
		# Got yacc(1), use it
		message(STATUS "Found yacc: ${YACC}")
		add_custom_command(OUTPUT gram.tab.c gram.tab.h
			DEPENDS gram.y
			COMMAND ${YACC} ${YFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/gram.y
		)
	else()
		# No bison, no yacc.  Maybe there are prebuilt files?
		find_file(GRAM_C gram.tab.c
			PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
		find_file(GRAM_H gram.tab.h
			PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
		if(GRAM_C AND GRAM_H)
			# Got prebuilt ones, use 'em
			message(STATUS "No yacc found, using prebuilt gram.tab.*")
			add_custom_command(OUTPUT gram.tab.h
				DEPENDS ${GRAM_H}
				COMMAND cp ${GRAM_H} .
			)
			add_custom_command(OUTPUT gram.tab.c
				DEPENDS ${GRAM_C}
				COMMAND cp ${GRAM_C} .
			)
		else()
			# No bison, no yacc, no prebuilt.  Boom.
			message(FATAL_ERROR "Can't find yacc.")
		endif(GRAM_C AND GRAM_H)
	endif(YACC)
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


# Where should we stick manpages?
if(MANBASE)
	# Assume the user knows what they're doing
else()
	find_file(MANBASE man
		PATHS
			"${CMAKE_INSTALL_PREFIX}/share"
			"${CMAKE_INSTALL_PREFIX}"
		NO_DEFAULT_PATH)
endif(MANBASE)

if(MANBASE)
	set(MAN1PATH "${MANBASE}/man1")
	message(STATUS "Installing manpage to ${MAN1PATH}")
else()
	# Wacky dance to make sure it's a string rather than a list, or else
	# some later uses of it (see faked install() below) wind up with
	# embedded semicolons.  Only way to define it as a plain string with
	# set() would be as a single crazy long line, so we do a string
	# replace instead.
	string(REPLACE ";" " " manbase_err
			"Can't figure out where to install manpage: "
			"try setting MANBASE to something appropriate for your system "
			"(e.g., /usr/share/man).")
	message(WARNING ${manbase_err})
endif(MANBASE)


#
# And the build targets
#

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
	message(STATUS "Building libctwmc and demolib.")
endif(DO_LIBCTWM)


# Rewrite man page
set(MANPAGE ${CMAKE_CURRENT_BINARY_DIR}/ctwm.1)
configure_file(ctwm.man ${MANPAGE} @ONLY)

# Compress (conditionally).  We could add more magic to allow different
# automatic compression, but that's probably way more involved than we
# need to bother with.  Most systems use gzip, and for the few that
# don't, the packagers can use NOMANCOMPRESS and handle it out of band.
if(NOT NOMANCOMPRESS)
	find_program(GZIP_CMD gzip)
	add_custom_command(OUTPUT "${MANPAGE}.gz"
		DEPENDS ${MANPAGE}
		COMMAND ${GZIP_CMD} -nc ${MANPAGE} > ${MANPAGE}.gz
		COMMENT "Building ${MANPAGE}.gz"
	)
	add_custom_target(man ALL DEPENDS "${MANPAGE}.gz")
	set(INSTMAN "${MANPAGE}.gz")
else()
	set(INSTMAN ${MANPAGE})
ENDif(NOT NOMANCOMPRESS)


# Cram a warning in about the MANBASE if we couldn't find it.  This is a
# particularly grody way of doing things, but we can't just add it as a
# standard target that install depends on (see
# http://public.kitware.com/Bug/view.php?id=8438).  We rely on the fact
# that install() is defined to run in order, but that only happens when
# everything is in a single directory.  This will break if/when we start
# splitting things up...
if(NOT MANBASE)
	install(CODE "message(FATAL_ERROR \"${manbase_err}\")")
endif(NOT MANBASE)

# Installing everything
install(TARGETS ctwm
	DESTINATION bin
)
if(USE_XPM)
	install(DIRECTORY xpm/
		DESTINATION ${PIXMAPDIR}
		FILES_MATCHING PATTERN "*.xpm"
	)
endif(USE_XPM)
install(FILES system.ctwmrc
	DESTINATION ${CONFDIR}
)
if(MANBASE)
	install(FILES ${INSTMAN}
		DESTINATION ${MAN1PATH}
	)
endif(MANBASE)
