# kernel makefile
ifneq ($(KERNELRELEASE),)
	obj-m := teensy-mod.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif
