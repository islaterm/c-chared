# No olvide definir la variable NSYSTEM con la ubicacion de nSystem:
# NSYSTEM=...
# export NSYSTEM

CFLAGS= -ggdb -Wall -pedantic -I"/home/islaterm/projects/Escuela/SitaMateu/CShared/include"
LFLAGS= -ggdb

LIBNSYS= /home/islaterm/projects/Escuela/SitaMateu/CShared/src/libnSys.a

all: test-share

test-share.o: test-share.c $(LIBNSYS)
	gcc $(CFLAGS) -c test-share.c

test-share: test-share.o $(LIBNSYS)
	gcc $(LFLAGS) test-share.o $(LIBNSYS) -o test-share

clean:
	rm -f *.o *~ test-share
