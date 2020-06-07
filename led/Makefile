KERN_DIR = /root/workstation/embedded-linux/kernel/kernel
obj-m += s5pled.o

all: leddriver ledapp

leddriver:
	make -C $(KERN_DIR) M=`pwd` modules

ledapp: 
	arm-none-linux-gnueabi-gcc ledapp.c -o ledapp 

clean:
	make -C $(KERN_DIR) M=`pwd` clean
	rm -f a.out ledapp

cp:
	cp *.ko ledapp /nfs
