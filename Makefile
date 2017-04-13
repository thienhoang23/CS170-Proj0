CC      = gcc
CFLAGS  = -O
LDFLAGS  = -O 


all: shell


shell:  shell.o
	$(CC) -o $@ $^ $(LDFLAGS)

run: 
	./shell

test: 
	./shell < ./test/*

clean:
	rm simple


.c.o:
	$(CC)  $(CFLAGS) -c $<

