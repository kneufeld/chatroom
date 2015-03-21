
all: $(patsubst %.diag,%.png,$(wildcard *.diag))

FONT=-f /home/kneufeld/.fonts/SourceCodePro-Regular.ttf
SHOW=open

%.png: %.diag
	seqdiag $(FONT) -a -o $@ $<
	$(SHOW) $@
