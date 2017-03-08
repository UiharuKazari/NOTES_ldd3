obj-m := printktest.o

all:
	make -C /lib/modules/${shell uname -r}/build M=${PWD} modules

setlevel:
	gcc -o setlevel setlevel.c

clean:
	make -C /lib/modules/${shell uname -r}/build M=${PWD} clean
	rm -rf setlevel 
