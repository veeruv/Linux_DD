KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m := CDD.o

all: run

run: CDD CDDapp
	echo "Hello World" > /dev/CDD;
	cat < /dev/CDD;
	echo "Hello World" > /dev/CDD;
	cat < /dev/CDD;
	./CDDapp;

load: CDD.o 
	-su -c "insmod ./CDD.ko CDDparm=0;";
	# -su -c "insmod ./CDD.ko CDDparm=32;";

CDD: load 
	-su -c "mknod -m 666 /dev/CDD c $(shell grep CDD /proc/devices | sed 's/CDD//') 0;"


CDD.o:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

###
###  Alternatively, you may want to use the early 2.6 syntax of
###  $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
###

CDDapp:
	-gcc -o CDDapp CDDapp.c;

unload:
	-su -c "rmmod CDD; rm /dev/CDD;"

clean: unload
	-@rm -fr *.o CDD*.o CDD*.ko .CDD*.* CDD*.*.* CDDapp .tmp_versions [mM]odule*

