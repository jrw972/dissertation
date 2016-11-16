all: slides.pdf

slides.pdf: slides.tex

%.pdf: %.tex slides.bib
	pdflatex $<
	bibtex slides
	pdflatex $<
	pdflatex $<

clean:
	-rm slides.pdf
