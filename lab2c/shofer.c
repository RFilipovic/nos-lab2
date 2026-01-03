/*
 * shofer.c -- module implementation
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kfifo.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "config.h"

static int pipe_size = PIPE_SIZE;
static int max_threads = MAX_THREADS;

module_param(pipe_size, int, S_IRUGO);
MODULE_PARM_DESC(pipe_size, "Pipe size");
module_param(max_threads, int, S_IRUGO);
MODULE_PARM_DESC(max_threads, "Maximal number of threads simultaneously using pipe");

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

static struct shofer_dev *shofer = NULL;

/* prototypes */
static int pipe_init(struct pipe *pipe, size_t pipe_size, size_t max_threads);
static void pipe_delete(struct pipe *pipe);
static struct shofer_dev *shofer_create(dev_t dev_no, struct file_operations *fops, int *retval);
static void shofer_delete(struct shofer_dev *shofer);

static int shofer_open(struct inode *, struct file *);
static int shofer_release(struct inode *, struct file *);
static ssize_t shofer_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t shofer_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations shofer_fops = {
	.owner =    THIS_MODULE,
	.open =     shofer_open,
	.release =  shofer_release,
	.read =     shofer_read,
	.write =    shofer_write
};

/* Initialize pipe structure */
int pipe_init(struct pipe *pipe, size_t pipe_size, size_t max_threads)
{
	int ret;

	ret = kfifo_alloc(&pipe->fifo, pipe_size, GFP_KERNEL);
	if (ret) {
		klog(KERN_NOTICE, "kfifo_alloc failed");
		return ret;
	}

	pipe->pipe_size = pipe_size;
	pipe->max_threads = max_threads;
	pipe->thread_cnt = 0;

	sema_init(&pipe->cs_readers, 1);
	sema_init(&pipe->cs_writers, 1);
	sema_init(&pipe->empty, 0);
	sema_init(&pipe->full, 1);
	pipe->reader_waiting = 0;
	pipe->writer_waiting = 0;

	mutex_init(&pipe->lock);

	return 0;
}

/* Delete pipe structure */
static void pipe_delete(struct pipe *pipe)
{
	kfifo_free(&pipe->fifo);
}

/* init module */
static int __init shofer_module_init(void)
{
	int retval;
	dev_t dev_no = 0;

	klog(KERN_NOTICE, "Module started initialization");

	/* get device number */
	retval = alloc_chrdev_region(&dev_no, 0, 1, DRIVER_NAME);
	if (retval < 0) {
		klog(KERN_WARNING, "Can't get major device number");
		return retval;
	}

	/* Create device */
	shofer = shofer_create(dev_no, &shofer_fops, &retval);
	if (!shofer) {
		unregister_chrdev_region(dev_no, 1);
		return retval;
	}

	klog(KERN_NOTICE, "Module initialized with major=%d, minor=%d",
		MAJOR(dev_no), MINOR(dev_no));

	return 0;
}

/* called when module exit */
static void __exit shofer_module_exit(void)
{
	klog(KERN_NOTICE, "Module started exit operation");

	if (shofer) {
		unregister_chrdev_region(shofer->dev_no, 1);
		shofer_delete(shofer);
	}

	klog(KERN_NOTICE, "Module finished exit operation");
}

module_init(shofer_module_init);
module_exit(shofer_module_exit);

/* Create and initialize a single shofer_dev */
static struct shofer_dev *shofer_create(dev_t dev_no,
	struct file_operations *fops, int *retval)
{
	struct shofer_dev *shofer = kmalloc(sizeof(struct shofer_dev), GFP_KERNEL);

	if (!shofer) {
		*retval = -ENOMEM;
		klog(KERN_WARNING, "kmalloc failed");
		return NULL;
	}

	memset(shofer, 0, sizeof(struct shofer_dev));

	/* initialize the pipe */
	if (pipe_init(&shofer->pipe, pipe_size, max_threads)) {
		*retval = -ENOMEM;
		kfree(shofer);
		return NULL;
	}

	cdev_init(&shofer->cdev, fops);
	shofer->cdev.owner = THIS_MODULE;
	shofer->cdev.ops = fops;

	*retval = cdev_add(&shofer->cdev, dev_no, 1);
	if (*retval) {
		klog(KERN_WARNING, "Error (%d) when adding device", *retval);
		pipe_delete(&shofer->pipe);
		kfree(shofer);
		return NULL;
	}

	shofer->dev_no = dev_no;

	return shofer;
}

/* Delete shofer_dev */
static void shofer_delete(struct shofer_dev *shofer)
{
	pipe_delete(&shofer->pipe);
	cdev_del(&shofer->cdev);
	kfree(shofer);
}

/* Called when the process calls "open" on this device */
static int shofer_open(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer;
	int flags = filp->f_flags & O_ACCMODE;

	shofer = container_of(inode->i_cdev, struct shofer_dev, cdev);
	filp->private_data = shofer;

	/* Check access mode - only O_RDONLY, O_WRONLY, or O_RDWR allowed */
	if (flags != O_RDONLY && flags != O_WRONLY && flags != O_RDWR) {
		klog(KERN_WARNING, "Invalid access mode");
		return -EINVAL;
	}

	/* Check if max threads limit reached */
	if (shofer->pipe.thread_cnt >= shofer->pipe.max_threads) {
		klog(KERN_WARNING, "Max threads limit reached");
		return -EBUSY;
	}

	shofer->pipe.thread_cnt++;

	klog(KERN_NOTICE, "Device opened, thread_cnt=%zu", shofer->pipe.thread_cnt);

	return 0;
}

/* Called when a process performs "close" operation */
static int shofer_release(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer = filp->private_data;

	shofer->pipe.thread_cnt--;

	klog(KERN_NOTICE, "Device closed, thread_cnt=%zu", shofer->pipe.thread_cnt);

	return 0;
}

/* Read from pipe */
static ssize_t shofer_read(struct file *filp, char __user *ubuf, size_t count,
	loff_t *f_pos /* ignoring f_pos */)
{
	struct shofer_dev *shofer = filp->private_data;
	struct pipe *pipe = &shofer->pipe;
	struct kfifo *fifo = &pipe->fifo;
	ssize_t retval = 0;
	unsigned int copied;

	/* Check if read is allowed with current access mode */
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		klog(KERN_WARNING, "Read not allowed in write-only mode");
		return -EPERM;
	}

	/* Enter critical section for readers */
	if (down_interruptible(&pipe->cs_readers))
		return -ERESTARTSYS;

	/* Enter critical section for pipe */
	while (1) {
		if (mutex_lock_interruptible(&pipe->lock)) {
			/* Waiting for critical section interrupted by signal */
			up(&pipe->cs_readers);
			return -ERESTARTSYS;
		}

		if (kfifo_is_empty(fifo)) {
			pipe->reader_waiting = 1;
			mutex_unlock(&pipe->lock);

			if (down_interruptible(&pipe->empty)) {
				up(&pipe->cs_readers);
				return -ERESTARTSYS;
			}
		} else {
			break;
		}
	}

	pipe->reader_waiting = 0;

	/* Read available data (up to count bytes) */
	retval = kfifo_to_user(fifo, ubuf, count, &copied);
	if (retval) {
		klog(KERN_WARNING, "kfifo_to_user failed");
		mutex_unlock(&pipe->lock);
		up(&pipe->cs_readers);
		return retval;
	}

	retval = copied;

	/* Wake up a writer if one is waiting */
	if (pipe->writer_waiting)
		up(&pipe->full);

	mutex_unlock(&pipe->lock);
	up(&pipe->cs_readers);

	klog(KERN_NOTICE, "Read %zd bytes", retval);

	return retval;
}

/* Write to pipe */
static ssize_t shofer_write(struct file *filp, const char __user *ubuf,
	size_t count, loff_t *f_pos /* ignoring f_pos */)
{
	struct shofer_dev *shofer = filp->private_data;
	struct pipe *pipe = &shofer->pipe;
	struct kfifo *fifo = &pipe->fifo;
	ssize_t retval = 0;
	unsigned int copied;

	/* Check if write is allowed with current access mode */
	if ((filp->f_flags & O_ACCMODE) == O_RDONLY) {
		klog(KERN_WARNING, "Write not allowed in read-only mode");
		return -EPERM;
	}

	/* Check if data is too large for pipe */
	if (count > pipe->pipe_size) {
		klog(KERN_WARNING, "Data too large for pipe");
		return -EFBIG;
	}

	/* Enter critical section for writers */
	if (down_interruptible(&pipe->cs_writers))
		return -ERESTARTSYS;

	/* Enter critical section for pipe */
	while (1) {
		if (mutex_lock_interruptible(&pipe->lock)) {
			/* Waiting for critical section interrupted by signal */
			up(&pipe->cs_writers);
			return -ERESTARTSYS;
		}

		if (kfifo_avail(fifo) < count) {
			pipe->writer_waiting = 1;
			mutex_unlock(&pipe->lock);

			if (down_interruptible(&pipe->full)) {
				up(&pipe->cs_writers);
				return -ERESTARTSYS;
			}
		} else {
			break;
		}
	}

	pipe->writer_waiting = 0;

	/* Write data to pipe */
	retval = kfifo_from_user(fifo, ubuf, count, &copied);
	if (retval) {
		klog(KERN_WARNING, "kfifo_from_user failed");
		mutex_unlock(&pipe->lock);
		up(&pipe->cs_writers);
		return retval;
	}

	retval = copied;

	/* Wake up a reader if one is waiting */
	if (pipe->reader_waiting)
		up(&pipe->empty);

	mutex_unlock(&pipe->lock);
	up(&pipe->cs_writers);

	klog(KERN_NOTICE, "Written %zd bytes", retval);

	return retval;
}
