KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: mem

mem:
	ls -l /dev/* | egrep "^c" | egrep "[ 	][ 	]*1," | sort -k6.1n
	head /proc/devices

