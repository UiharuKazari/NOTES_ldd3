#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/param.h>
#include <linux/fs.h>
MODULE_LICENSE("GPL");

static int scull_major = 0;
static int scull_minor = 0;
static int scull_nr_devs = 1;
module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int ,S_IRUGO);
module_param(scull_nr_devs, int ,S_IRUGO);

dev_t dev;

static int __init scull_init(void)
{
	int result;

	if(scull_major)
	{
		dev = MKDEV(scull_major, scull_minor);
		result = register_chrdev_region(dev, scull_nr_devs, "scull");
	} else {
		result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
		scull_major = MAJOR(dev);
	}

	if(result < 0)
	{
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return result;
	}

	printk(KERN_INFO "scull init ok: major=%d\n", scull_major);
	return 0;
}

static void __exit scull_exit(void)
{
	unregister_chrdev_region(dev, scull_nr_devs);
	printk(KERN_INFO "scull exit\n");
}

module_init(scull_init);
module_exit(scull_exit);
