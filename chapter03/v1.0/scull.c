#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
MODULE_LICENSE("GPL");

int stuff_ok;
dev_t scull1;

void scull_cleanup(void)
{
	//do nothing now
	printk(KERN_ALERT "scull cleanup begin...\n");
	if(stuff_ok == 1)
	{
		unregister_chrdev_region(144, 10);
		printk(KERN_ALERT "scull cleanup 1\n");
	}
	
	printk(KERN_ALERT "scull cleanup finish\n");

}

static int __init scull_init(void)
{
	int err;

	if(register_chrdev_region(144, 10, "scull") != 0)
	{
		printk(KERN_ALERT "scull fail\n");
		goto fail;
	} else {
		printk(KERN_ALERT "scull ok\n");
		stuff_ok = 1;
	}

	err = alloc_chrdev_region(&scull1, 0, 1, "scull1");
	if(err)
	{
		printk(KERN_ALERT "scull1 alloc fail\n");	
		goto fail1;
	}
	printk(KERN_ALERT "scull1 alloc ok, MAJOR: %d, MINOR: %d\n", MAJOR(scull1), MINOR(scull1));
	stuff_ok = 2;

	return 0;

	fail:
		scull_cleanup();
		return -1;

	fail1:
		scull_cleanup();
		return -2;
}

static void __exit scull_exit(void)
{
	unregister_chrdev_region(144, 10);
	unregister_chrdev_region(MAJOR(scull1), 1);
	printk(KERN_ALERT "scull bye\n");
}

module_init(scull_init);
module_exit(scull_exit);
