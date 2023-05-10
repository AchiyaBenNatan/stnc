CC=gcc
CFLAGS=-Wall
STNC_A=stnc.c
STNC_A_EXE=stnc
.PHONY: all clean
all: $(STNC_A_EXE)

$(STNC_A_EXE): $(STNC_A)
	$(CC) $(CFLAGS) -o $@ $^
clean:
	rm -f $(STNC_A_EXE)
