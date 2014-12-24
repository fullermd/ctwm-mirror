all: ctwm.1.html

clean:
	rm -f ctwm.1.*

ctwm.1.html: Makefile

ctwm.1.txt: ../ctwm.man convert.pl
	./convert.pl < ../ctwm.man > ctwm.1.txt

.SUFFIXES: .txt .html

.txt.html:
	asciidoc -atoc -anumbered -o ${.TARGET} ${.IMPSRC}

# NOTYET
ctwm.1: ctwm.1.txt
	a2x -vv --doctype manpage --format manpage ctwm.1.txt
