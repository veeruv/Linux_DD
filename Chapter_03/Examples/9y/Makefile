MYUSBVendorID = 090c
MYUSBProductID = 1000
MYUSBVolumeLabel = MYLDD

##  Do not edit following 3 lines
MYUSBIDString = $(MYUSBVendorID):$(MYUSBProductID)
# EP_IN=$(shell lsusb -d $(MYUSBIDString) -v | egrep "EP.*IN" | sed -e 's/.*bEndpointAddress  *//' -e 's/EP.*IN.*//')
# EP_OUT=$(shell lsusb -d $(MYUSBIDString) -v | egrep "EP.*OUT" | sed -e 's/.*bEndpointAddress  *//' -e 's/EP.*OUT.*//')

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
##  Do not edit following lines

obj-m := main.o
main-objs := CDD2.o USB.o

all: clean run 
	make -s clean
	sleep 3
	@echo "see /usr/src/linux/Documentation/usb/proc_usb_info.txt"

run: libUSBexample l2 examples_xusb

prep: 
	-umount /media/$(MYUSBVolumeLabel) 
	-{ lsmod | grep usb_storage; } &&  { modprobe -r usb-storage; }

mem:
	ls -l /dev | egrep "^c" | egrep "[ 	][ 	]*1," | sort +5.0n -6.0

libUSBexample: libUSBexample.c
	cc libUSBexample.c -lusb -o libUSBexample

l2: l2.c
	-@umount /media/MYLDD
	-modprobe -r usb-storage
	rm -f core* l2
	cc l2.c -lusb -o l2
	# -./l2
	-[ -f "core.*" ] &&  { gdb ./l2 `ls -1 core*`; }

examples_xusb: examples_xusb.c
	-@umount /media/MYLDD 
	-modprobe -r usb-storage
	rm -f core* examples_xusb
	# cc -o examples_xusb -L/lib /lib/libusb-1.0.so.0 -lusb examples_xusb.c
	cc -I /root/libusb/libusb-1.0.20/libusb \
		-o examples_xusb -L/lib \
		/usr/local/lib/libusb-1.0.so		\
		-lusb examples_xusb.c
	-./examples_xusb
	-[ -f "core.*" ] &&  { gdb ./examples_xusb `ls -1 core*`; }

reset:
	-modprobe usb-storage

clean: reset
	-@rm -fr *.ko *.o CDD2app .tmp_versions [mM]odule* *.mod.c l2 libUSBexample 3u_201* /tmp/lsusb_d_v examples_xusb
	#-@modprobe usb-storage

