#KERNELDIR:= /lib/modules/$(shell uname -r)/build/
#KERNELDIR:= /home/linux/kernal/kernel-3.4.39/
KERNELDIR:= /home/linux/kernal/kernel-3.4.39/
PWD:=$(shell pwd)
all:
	make -C $(KERNELDIR) M=$(PWD) modules
	arm-none-linux-gnueabi-gcc server.c -lm -L ./lib -lsqlite3 -lpthread
	cp *.ko a.out ~/nfs/rootfs/
clean:
	make -C $(KERNELDIR) M=$(PWD) clean

obj-m:=led_feng.o
