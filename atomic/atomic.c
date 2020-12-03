#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define KTHREAD_CNT 4

struct task_struct* kthreads[KTHREAD_CNT];

int n = 0;
int lock = 0;

static int fad(void* data)
{
    while (!kthread_should_stop()) {
        __sync_fetch_and_add(&n, 1);
        printk(KERN_INFO "thread(%s)[%d]: %d\n", current->comm, current->pid, n);
        msleep(1000);
    }
    return 0;
}

static int cas(void* data)
{
        
    while (!kthread_should_stop()) {
        while (__sync_val_compare_and_swap(&lock, 0, 1))
            ;
        n += 1;
        lock = 0;
        printk(KERN_INFO "thread(%s)[%d]: %d\n", current->comm, current->pid, n);
        msleep(1000);
    }
    return 0;
}

static int tas(void* data)
{
    while (!kthread_should_stop()) {
        while (__sync_lock_test_and_set(&lock, 1))
            ;
        n += 1;
        __sync_lock_release(&lock);
        printk(KERN_INFO "thread(%s)[%d]: %d\n", current->comm, current->pid, n);
        msleep(1000);
    }
    return 0;
}

static int atomic_init(void)
{
    printk("atomic init\n");
    kthreads[0] = kthread_run(fad, NULL, "fad");
    kthreads[1] = kthread_run(fad, NULL, "fad");
    kthreads[2] = kthread_run(cas, NULL, "cas");
    kthreads[3] = kthread_run(tas, NULL, "tas");

    return 0;
}

static void atomic_exit(void)
{
    int i;
    printk("atomic exit");
    for (i = 0; i < KTHREAD_CNT; ++i)
        kthread_stop(kthreads[i]);
}

module_init(atomic_init);
module_exit(atomic_exit);
MODULE_LICENSE("GPL");
