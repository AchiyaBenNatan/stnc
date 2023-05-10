CC=gcc
CFLAGS=-Wall

STNC=stnc.c
STNC_EXE=stnc

.PHONY: all clean

all: $(STNC_EXE)

$(STNC_EXE): $(STNC)
	$(CC) $(CFLAGS) -o $@ $^


clean:
	rm -f $(STNC_EXE) 
