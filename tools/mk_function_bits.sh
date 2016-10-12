#!/bin/sh
#
# $0 ../function_defs.list OUTDIR
#
# Generates output files with various defs and lookup arrays.

infile="$1"
outdir="$2"

if [ -z "${infile}" -o ! -r "${infile}" ]; then
	echo "No infile."
	exit 1
fi

if [ -z "${outdir}" -o ! -d "${outdir}" ]; then
	echo "No outdir."
	exit 1
fi

# Stripping out commands, blank lines, etc.
CLEANDAT="-e s/#.*// -e /^[[:space:]]*$/d"

# Uppercasing
TOUPPER="tr [:lower:] [:upper:]"


# We're all C here
print_header() {
	echo "/*"
	echo " * AUTOGENERATED FILE -- DO NOT EDIT"
	echo " * This file is generated automatically by $0"
	echo " * from '${infile}'"
	echo " * during the build process."
	echo " */"
	echo
}


# Getting one section out of the input file
getsect() {
	# We presume we always want to clear out the comments and blank lines
	# anyway, so stick that in here.  And I think we always end up
	# sorting too, so do that as well.
	sed -e "1,/^# START($1)/d" -e "/^# END($1)/,\$d" \
		${CLEANDAT} ${infile} | sort
}


# First off, creating the F_ defines.
gf="${outdir}/functions_defs.h"
(
	print_header
	echo "/* Definitions for functions */"
	echo
	echo "#define F_NOP 0    /* Standin */"
	echo

	counter=1

	echo "/* Standard functions */"
	while read func ctype ifdef
	do
		if [ "X${ifdef}" != "X-" ]; then
			echo "#ifdef ${ifdef}"
		fi

		cmt=" //"
		if [ "X${ctype}" = "XS" ]; then
			cmt="${cmt} string"
		fi
		if [ "X${cmt}" = "X //" ]; then
			cmt=""
		fi

		printf "#define F_%-21s %3d${cmt}\n" "${func}" "${counter}"

		if [ "X${ifdef}" != "X-" ]; then
			echo "#endif"
		fi
		counter=$((counter+1))
	done << EOF
	$(getsect main \
		| awk '{printf "%s %s %s\n", toupper($1), $2, $4;}')
EOF

	echo
	echo "/* Synthetic functions */"
	while read func
	do
		printf "#define F_%-21s %3d\n" "${func}" "${counter}"
		counter=$((counter+1))
	done << EOF
	$(getsect synthetic \
		| awk '{printf "%s\n", toupper($1)}')
EOF

) > ${gf}
echo "Generated ${gf}"