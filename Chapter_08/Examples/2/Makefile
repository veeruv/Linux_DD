KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

OBJ := main

obj-m := $(OBJ).o

all: run clean
	dmesg | tail | grep 2470 | egrep "8.2"
	tail /var/log/messages | grep 2470 | egrep "8.2"

run:  load 
	cat /proc/MYDEV/hello
	echo 123 > /proc/MYDEV/hello
	cat /proc/MYDEV/hello
	cat /proc/MYDEV/hello

compile:
	@$(MAKE) -C $(KDIR) M=$(PWD) modules 

###  
###  Alternatively, you may want to use the early 2.6 syntax of
###  $(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules 
###  
###

load:	compile
	@echo "try \"tail -f /var/log/messages\" in another window(as root) ..";
	-su -c "insmod ./$(OBJ).ko";

unload: 
	-su -c "rmmod $(OBJ)"

clean: unload
	-@rm -fr $(OBJ).o $(OBJ).ko $(OBJ).*.* .$(OBJ).* .tmp_versions* [mM]odule*
