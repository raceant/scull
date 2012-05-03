/*
 * pipe.c -- fifo driver for scull
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>	/* printk(), min() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#include "scull.h"

struct scull_pipe {
	wait_queue_head_t inq, outq;    // read and write queues
	char *buffer, *end;		// begin of buf, end of buf
	int buffersize;			// used in pointer arithmetic
	char *rp, *wp;			// where to read, where to write
	int nreaders, nwriters;		// number of openings for r/w
	struct fasync_struct *async_queue;// asynchronous readers
	struct semaphore sem;		// mutual exclusion semaphore
	struct cdev cdev;		// Char device structure
};

/*
 * parameters
 */
static int scull_p_nr_devs = SCULL_P_NR_DEVS;	// number of pipe devices
int scull_p_buffer = SCULL_P_BUFFER;
dev_t scull_p_devno;		// Our first device number

module_param(scull_p_nr_devs, int, 0);
module_param(scull_p_buffer, int, 0);

static struct scull_pipe *scull_p_devices;

static int scull_p_fasync(int fd, struct file *file, int mode);
static int spacefree(struct scull_pipe *dev);

static int scull_p_open(struct inode *inode, struct file *filp)
{
	struct scull_pipe *dev;

	dev = container_of(inode->i_cdev, struct scull_pipe, cdev);
	file->private_data = dev;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (!dev->buffer) {
		dev->buffer = kmalloc(scull_p_buffer, GFP_KERNEL);
		if (!dev->buffer) {
			up(&dev->sem);
			return -ENMEM;
		}
	}
	dev->buffersize = scull_p_buffer;
	dev->end = dev->buffer + dev->buffersize;
	dev->rp = dev->wp = dev->buffer;

	if (filp->f_mode & FMODE_READ)
		dev->nreaders++;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters++;
	up(&dev->sem);

	return nonseekable_open(inode, filp);
}

static int scull_p_release(struct inode *inode, struct file *filp)
{
	struct scull_pipe *dev = filp->private_data;

	// remove this filp from the asynchronously notified filp's
	scull_p_fasync(-1, filp, 0);
	down(&dev->sem);

	if (filp->f_mode & FMODE_READ)
		dev->nreaders--;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters--;
	if (dev->nreaders + dev->nwriters == 0) {
		kfree(dev->buffer);
		dev->buffer = NULL;
	}
	up(&dev->sem);
	return 0;
}

static int spacefree(struct scull_pipe *dev)
{
	if (dev->rp == dev->wp)
		return dev->buffersize - 1;
	return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) -1;
}

struct file_operations scull_pipe_fops = {
	.owner	= THIS_MODULE,
	.llseek = no_llseek,
	.read	= scull_p_read,
	.write	= scull_p_write,
	.poll	= scull_p_poll,
	.ioctl	= scull_ioctl,
	.open	= scull_p_open,
	.release= scull_p_release,
	.fasync	= scull_p_fasync,
};
