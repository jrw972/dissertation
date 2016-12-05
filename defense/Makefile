all: slides.pdf

%.pdf: %.tex model.tex slides.bib
	pdflatex $<
	bibtex slides
	pdflatex $<
	pdflatex $<

clean:
	-rm slides.pdf
