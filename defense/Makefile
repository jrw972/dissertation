all: slides.pdf

slides.pdf: slides.tex basics.tex branching.tex remotes.tex history.tex rewriting.tex workflows.tex miscellaneous.tex

%.pdf: %.tex
	pdflatex $<

clean:
	-rm slides.pdf
