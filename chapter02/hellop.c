#include <linux/param.h>
#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static int times=1;
static char *name="world";
module_param(times,int,S_IRUGO);
module_param(name,charp,S_IRUGO);

static int __init helloinit(void)
{
	int i;
	for(i=0; i<times; i++)
		printk(KERN_ALERT "Hello %s!\n", name);
	return 0;
}

static void __exit helloexit(void)
{
	printk(KERN_ALERT "Goodbye, %s!\n", name);
}

module_init(helloinit);
module_exit(helloexit);
