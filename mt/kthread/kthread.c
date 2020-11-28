#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/delay.h>

int my_thread(void *_arg) {
	int* arg = (int*)_arg;
	printk("argument : %d\n", *arg);
	kfree(arg);
	return 0;
}

void thread_create(void) {
	int i;
	for(i = 0; i < 10; ++i) {
		int* arg = (int*)kmalloc(sizeof(int), GFP_KERNEL);
		*arg = i;
		kthread_run(&my_thread, (void*)arg, "my_thread");
	}
}

int __init my_init(void) {
	thread_create();
	printk("kthread module init\n");
	return 0;
}

void __exit my_exit(void) {
	printk("kthread module exit\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
