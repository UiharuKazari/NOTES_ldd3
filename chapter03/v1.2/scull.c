#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h> // container_of
#include <linux/slab.h> //kfree
#include <asm/uaccess.h> // copy_to_user, copy_from_user
MODULE_LICENSE("GPL");

static int scull_major;
static int scull_minor;
static int scull_quantum = 1; // I don't know what's this
static int scull_qset = 1; // I don't know what's this

dev_t dev;

struct scull_qset { // copy it from example
	void **data;
	struct scull_qset *next;
};

struct scull_dev {
	struct scull_qset *data;	/* Pointer to first quantum set */
	int quantum;			/* the current quantum size */
	int qset;			/* the current array size */
	unsigned long size;		/* amount of data stored here */
	unsigned int access_key;	/* used by sculluid and scullpriv */
	struct semaphore sem;		/* mutual exclusion semaphore */  /* TODO sth wrong with new kernel */
	struct cdev cdev;		/* Char device structure */
};

int scull_trim(struct scull_dev *dev)
{
	struct scull_qset *next, *dptr;
	int qset = dev->qset; // dev is not null
	int i;

	for (dptr = dev->data; dptr; dptr = next)
	{ // all the list itmes 
		if(dptr->data)
		{
			for(i=0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next = dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;
	return 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_dev *dev; //device information

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev; // for other methods

	/* now trim to 0 the length of the devie if open was write-only */
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
//		scull_trim(dev); //ignore errors
		
	}

	return 0;
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}


struct scull_qset *scull_follow(struct scull_dev *dev, int n) // copy from e.g.
{
        struct scull_qset *qs = dev->data;

        /* Allocate first qset explicitly if need be */
        if (! qs) {
                qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
                if (qs == NULL)
                        return NULL;  /* Never mind */
                memset(qs, 0, sizeof(struct scull_qset));
        }

        /* Then follow the list */
        while (n--) {
                if (!qs->next) {
                        qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
                        if (qs->next == NULL)
                                return NULL;  /* Never mind */
                        memset(qs->next, 0, sizeof(struct scull_qset));
                }
                qs = qs->next;
                continue;
        }
        return qs;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;	// the first listitem
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset; // how many bytes in the listitem
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;

/*
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
*/
	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	// find listitem, qset index, and offset in the quantum
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	// follow the list up to the right position (defined elsewhere)
	dptr = scull_follow(dev, item); //TODO

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out; // don't fill holes

	// read only up to the end of this quantum
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) { //TODO
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

out:
	up(&dev->sem);
	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM; // value used in "goto out" statements

/*
	if(down_interruptible(&dev_sem))
		return -ERESTARTSYS;
*/

	// find listitem, qset index and offset in the quantum
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum, q_pos = rest % quantum;

	// follow the list up to the right position
	dptr = scull_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data)
	{
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos])
	{
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
	}
	//write only up to the end of this quantum
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count))
	{
		retval = -EFAULT;
		goto out;
	}

	*f_pos += count;
	retval = count;

	//update the size
	if (dev->size < *f_pos)
		dev->size = *f_pos;

out:
	up(&dev->sem);
	return retval;
}

struct file_operations scull_fops = {
	.owner = 	THIS_MODULE,
/*
	.llseek = 	scull_llseek,
*/
	.read =		scull_read,
/*
	.write = 	scull_write,
	.ioctl = 	scull_ioctl,
*/
	.open =		scull_open,
	.release = 	scull_release,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
	int err, devno = MKDEV(scull_major, scull_minor + index);

	cdev_init(&dev->cdev, &scull_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}

static int __init scull_init(void)
{
	alloc_chrdev_region(&dev, 0, 1, "scull");
	scull_major = MAJOR(dev);
	scull_minor = MINOR(dev);
	printk(KERN_INFO "major: %d; minor: %d\n", scull_major, scull_minor);

//	struct cdev *scull_cdev = cdev_alloc();
//	scull_cdev->ops = &scull_fops;

//	cdev_init(scull_cdev, &scull_fops);

//	cdev_add(scull_cdev, MAJOR(dev), 1);
	
//	cdev_del(scull_cdev);

	struct scull_dev scull_dev01; 			// if define pointer here, you'll get lots of err in dmesg, sth like null pointer and seg fault

	scull_setup_cdev(&scull_dev01, 1);	

	return 0;
}

static void __exit scull_exit(void)
{
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "scull exit\n");
}

module_init(scull_init);
module_exit(scull_exit);
