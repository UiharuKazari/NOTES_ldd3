#include <linux/module.h>
#include <linux/init.h>
MODULE_LICENSE("GPL");

static int __init printktest_init(void)
{
	printk(KERN_INFO "printk init\n");
	printk(KERN_DEBUG "Here I am: %s:%i\n", __FILE__, __LINE__);
	printk("\001" "0" "This is emerg msg without loglevel macro\n");
	printk("\001" "1" "This is alert msg without loglevel macro\n");
	printk("\001" "2" "This is crit msg without loglevel macro\n");
	printk("\001" "3" "This is err msg without loglevel macro\n");
	printk("\001" "4" "This is warning msg without loglevel macro\n");
	printk("\001" "5" "This is notice msg without loglevel macro\n");
	printk("\001" "6" "This is info msg without loglevel macro\n");
	printk("\001" "7" "This is debug msg without loglevel macro\n");
	printk("This is msg with default loglevel\n");
	return 0;
}

static void __exit printktest_exit(void)
{
	int n=1;
	int *ptr;
	ptr=&n;
	printk(KERN_INFO "printk exit\n");
	printk(KERN_CRIT "I'm trashed; giving up on %p\n", ptr);
}

module_init(printktest_init);
module_exit(printktest_exit);
