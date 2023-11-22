CC=gcc
CFLAGS=-I.
DEPS =
OBJ = hexa.o
LIBS=-lcurl -lpthread

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

hexa: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: build clean

build:
	sudo apt update
	sudo apt install libcurl4-gnutls-dev build-essential -y
	make hexa

clean:
	rm -f *.o *~ core hexa
