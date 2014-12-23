all: manual.html

clean:
	rm -f manual.html manual.txt

manual.html: Makefile

manual.txt: ../ctwm.man convert.pl
	./convert.pl < ../ctwm.man > manual.txt

.SUFFIXES: .txt .html

.txt.html:
	asciidoc -atoc -anumbered -o ${.TARGET} ${.IMPSRC}
