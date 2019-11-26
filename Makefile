obj-m += lcd13264_drv.o
KERNEL_PATH = .
all:
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- -C $(KERNEL_PATH)  SUBDIRS=$(PWD) modules
clean:
	make ARCH=arm CROSS_COMPILE=arm-none-linux-gnueabi- -C $(KERNEL_PATH)  SUBDIRS=$(PWD) clean
