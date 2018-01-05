MAKEFLAGS+=" -r "

.PHONY: build clean

build: exer002.pdf

%.pdf: %.tex
	pdflatex "$*"
	# bibtex "$*"
	pdflatex "$*"
	# pdflatex "$*"

clean:
	rm -f \
		exer002.aux \
		exer002.bbl \
		exer002.bcf \
		exer002.blg \
		exer002.log \
		exer002.out \
		exer002.pdf \
		exer002.run.xml \

