KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m := CDD.o

all: run clean

run: load CDDapp
	echo "see /usr/src/linux/Documentation/ioctl/ioctl-number.txt"
	echo "see line #1600 \"file_operations\" in /usr/src/linux/include/linux/fs.h"
	sleep 10
	echo "Hello World" > /dev/CDD;
	cat < /dev/CDD;
	echo "Hello World" > /dev/CDD;
	cat < /dev/CDD;
	-./CDDapp;

load: CDD.o
	-su -c "insmod ./CDD.ko; mknod -m 666 /dev/CDD c 32 0;"

CDD.o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

CDDapp:
	-gcc -o CDDapp CDDapp.c;

unload:
	-su -c " rmmod CDD; \
	rm /dev/CDD;"

clean: unload
	-@rm -fr *.o CDD*.o CDD*.ko .CDD*.* CDD*.*.* CDDapp .tmp_versions [mM]odule*

