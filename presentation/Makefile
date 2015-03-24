
all: $(patsubst %.diag,images/%.png,$(wildcard *.diag))

FONT=-f /home/kneufeld/.fonts/SourceCodePro-Regular.ttf
SHOW=open

images/%.png: %.diag
	seqdiag $(FONT) -a -o $@ $<
	$(SHOW) $@
