CC      = gcc
CFLAGS  = -O
LDFLAGS  = -O 


all: simple shell

simple:  simple_shell.o
	$(CC) -o $@ $^ $(LDFLAGS)

shell:  shell.o
	$(CC) -o $@ $^ $(LDFLAGS)

run: 
	./shell

test: 
	./shell < test/*

clean:
	rm simple


.c.o:
	$(CC)  $(CFLAGS) -c $<

