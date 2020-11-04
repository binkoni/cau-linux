#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int simple_init(void)
{
	printk("simple module\n");
	return 0;
}

static void simple_exit(void)
{
}

module_init(simple_init);
module_exit(simple_exit);
MODULE_LICENSE("GPL");
