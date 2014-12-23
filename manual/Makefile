all: manual.html

clean:
	rm -f manual.html manual.txt ctwm.1*

manual.html: Makefile

manual.txt: ../ctwm.man convert.pl
	./convert.pl < ../ctwm.man > manual.txt

.SUFFIXES: .txt .html

.txt.html:
	asciidoc -atoc -anumbered -o ${.TARGET} ${.IMPSRC}

# NOTYET
ctwm.1: manual.txt
	cp manual.txt ctwm.1.txt
	a2x -vv --doctype manpage --format manpage ctwm.1.txt
	rm -f ctwm.1.txt
