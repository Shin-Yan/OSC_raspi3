#ifndef _KTHREAD_H
#define _KTHREAD_H

#include <sched.h>

void kthread_init(void);

void kthread_create(void (*start)(void));

void kthread_fini(void);

void kthread_early_init(void);

void kthread_add_wait_queue(task_struct *task);

void kthread_kill_zombies(void);

#endif