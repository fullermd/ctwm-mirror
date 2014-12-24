BASE=ctwm.1

all: ${BASE}.html

clean:
	rm -f ${BASE}.*

${BASE}.txt: ../ctwm.man convert.pl
	./convert.pl < ../ctwm.man > ${BASE}.txt

${BASE}.html: ${BASE}.txt
	asciidoc -atoc -anumbered -o ${BASE}.html ${BASE}.txt

${BASE}: ${BASE}.txt
	a2x -vv --doctype manpage --format manpage ${BASE}.txt
