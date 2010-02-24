PAPER = a4
MKHOWTO = python tools/mkhowto --$(PAPER)

default: html

all: ps html text dvi

dvi ps text: saprfc.tex
	$(MKHOWTO) --$@ $^

html: saprfc.tex
	$(MKHOWTO) --html --iconserver . $^
	mv saprfc html

clean:
	rm -rf html saprfc.dvi saprfc.ps saprfc.txt \
	    saprfc.l2h saprfc.how

.PHONY: default all html dvi ps text clean
