/*
 * config.h -- structures, constants, macros
 *
 * Copyright (C) 2021 Leonardo Jelenkovic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#pragma once

#define DRIVER_NAME 	"shofer"

#define AUTHOR		"Leonardo Jelenkovic"
#define LICENSE		"Dual BSD/GPL"

#define PIPE_SIZE	64
#define MAX_THREADS	5

/* Pipe structure for inter-thread communication */
struct pipe {
	size_t pipe_size;
	size_t max_threads;
	size_t thread_cnt;

	struct kfifo fifo;
	struct semaphore cs_readers;	/* critical section for readers - one at a time */
	struct semaphore empty;		/* if pipe is empty, reader waits */
	int reader_waiting;		/* is a reader waiting? */
	struct semaphore cs_writers;	/* critical section for writers - one at a time */
	struct semaphore full;		/* if pipe is full, writer waits */
	int writer_waiting;		/* is a writer waiting? */
	struct mutex lock;		/* for mutual exclusion */
};

/* Device driver */
struct shofer_dev {
	dev_t dev_no;		/* device number */
	struct pipe pipe;	/* pipe structure */
	struct cdev cdev;	/* char device structure */
};

#define klog(LEVEL, format, ...)	\
printk(LEVEL "[shofer] %d: " format "\n", __LINE__, ##__VA_ARGS__)

//#define SHOFER_DEBUG

#ifdef SHOFER_DEBUG
#define LOG(format, ...)	klog(KERN_DEBUG, format,  ##__VA_ARGS__)
#else /* !SHOFER_DEBUG */
#warning Debug not activated
#define LOG(format, ...)
#endif /* SHOFER_DEBUG */
