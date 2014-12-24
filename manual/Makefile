all: ctwm.1.html

clean:
	rm -f ctwm.1.*

ctwm.1.txt: ../ctwm.man convert.pl
	./convert.pl < ../ctwm.man > ctwm.1.txt

ctwm.1.html: ctwm.1.txt
	asciidoc -atoc -anumbered -o ${@} ${>}

ctwm.1: ctwm.1.txt
	a2x -vv --doctype manpage --format manpage ctwm.1.txt
