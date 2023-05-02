CC=gcc
CFLAGS=-Wall

STNC_EXE=stnc.c
STNC=stnc
.PHONY: all clean

all:$(STNC)

$(STNC): $(STNC_EXE)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(SERVER_EXE) $(STNC)
