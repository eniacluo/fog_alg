all: peer

peer: peer.c peer.h
	gcc peer.c -g -L/usr/local/lib -lgsl -lgslcblas -lm -o peer

clean:
	rm -f peer
