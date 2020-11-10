obj-m := linked-list.o

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.ko
	rm -f *.mod.
	rm -f .*.cmd
	rm -f *.o
	rm -f modules.order Module.symvers
