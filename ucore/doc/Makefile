V	?=	@

SRC		:=	$(shell find . -name "*.rst")
MAN		:=	${SRC:.rst=.man}
HTML	:=	${SRC:.rst=.html}
TEX		:=	${SRC:.rst=.tex}
ARGS:=	--title=UCORE

.DEFAULT_GOAL	:=	man
.PHONY: man html tex

man: ${MAN}

html: ${HTML}

tex: ${TEX}

${MAN}:%.man:%.rst
	@echo + MAN $<
	${V}rst2man ${ARGS} $< $@

${HTML}:%.html:%.rst
	@echo + HTML $<
	${V}rst2html ${ARGS} $< $@

${TEX}:%.tex:%.rst
	@echo + TEX $<
	${V}rst2latex ${ARGS} $< $@

clean:
	${V}rm -rf ${MAN} ${HTML} ${TEX}
