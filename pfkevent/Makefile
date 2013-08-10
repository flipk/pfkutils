ifneq ($(KERNELRELEASE),)

obj-m := pfktestmod.o
pfktestmod-y := pfk_event.o pfk_printdev.o pfk_eventtest.o

else

ARCH=arm

KERNEL=/home/flipk/angstrom/beaglebone/setup-scripts/build/tmp-angstrom_v2012_05-eglibc/work/beaglebone-angstrom-linux-gnueabi/linux-ti33x-psp-3.2.34-r18a+gitr720e07b4c1f687b61b147b31c698cb6816d72f01/git

CROSS_COMPILE=/usr/local/oecore-x86_64/sysroots/x86_64-angstromsdk-linux/usr/bin/armv7a-angstrom-linux-gnueabi/arm-angstrom-linux-gnueabi-

all: mod

mod:
	CROSS_COMPILE=$(CROSS_COMPILE) \
	    make -C $(KERNEL) ARCH=$(ARCH) M=$(PWD) modules

copy:
	scp pfktestmod.ko root@192.168.7.2:/tmp

clean:
	rm -f *.o *.ko *.mod.c modules.order Module.symvers

endif

