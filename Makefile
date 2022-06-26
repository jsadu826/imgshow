obj-m := mod_imgshow.o # module name, check "/sys/module/[module name]" or "cat /proc/modules"
mod_imgshow-objs := imgshow.o

all: ko

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
