ifneq ($(KERNELRELEASE),)

# 최종 모듈 이름
obj-m := device_driver_mod.o
device_driver_mod-y := device_driver.o ssd1306.o ds1302.o

else

KDIR := /home/ubuntu/linux
PWD  := $(shell pwd)

all:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(PWD) clean

endif