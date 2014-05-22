# This just shortcuts stuff through to cmake
all build ctwm install clean: build/Makefile
	( cd build && ${MAKE} ${@} )

build/Makefile cmake: CMakeLists.txt
	( cd build && cmake .. )

allclean:
	rm -rf build/*
