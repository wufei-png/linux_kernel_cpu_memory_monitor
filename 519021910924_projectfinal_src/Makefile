obj-m += process_watch_kernel.o

.PHONY: all, clean, insert, remove

all: process_watch process_watch_kernel.ko insert

process_watch_kernel.ko: process_watch_kernel.c
	make ARCH=x86_64 -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

process_watch: process_watch.c
	gcc -g -O -o process_watch process_watch.c

insert:
	sudo insmod process_watch_kernel.ko

clean:
	sudo rmmod process_watch_kernel
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f process_watch
