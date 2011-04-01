obj-m := easy_slow_down_manager.o samsung-backlight.o

ifneq ($(KERNELRELEASE),)

KVERSION = $(shell uname -r)

all:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

endif
