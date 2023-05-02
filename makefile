CC=gcc
CFLAGS=-Wall

STNC_A=stnc.c
STNC_B=stncB.c

STNC_A_EXE=stncA
STNC_B_EXE=stncB

.PHONY: all clean

all: $(STNC_A_EXE) $(STNC_B_EXE)

$(STNC_A_EXE): $(STNC_A)
	$(CC) $(CFLAGS) -o $@ $^

$(STNC_B_EXE): $(STNC_B)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(STNC_A_EXE) $(STNC_B_EXE)
