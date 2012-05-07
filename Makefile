#if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
DEBUG = y

ifeq ($(DEBUG),y)
		DEBFLAGS = -O -g -DSCULL_DEBUG
else
		DEBFLAGS = -O2
endif

CFLAGS += $(DEBFLAGS)

ifneq ($(KERNELRELEASE),)
		scull-objs := main.o pipe.o 
		obj-m := scull.o 

# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
		KERNELDIR ?= /lib/modules/$(shell uname -r)/build
		PWD  := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
		rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions

