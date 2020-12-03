#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/kthread.h>

struct my_list_entry {
    struct list_head list;
    int data;
};

struct list_head my_list;
struct list_head* elem;
struct rw_semaphore lock;

void insert_entries(struct list_head* list, int count) {
    int i;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    for (i = 0; i < count; ++i) {
        down_write(&lock);
        entry = kmalloc(sizeof(struct my_list_entry), GFP_KERNEL);
        entry->data = i;
        list_add_tail(&entry->list, list);
        up_write(&lock);
    }
    end_time = ktime_get_ns();
    printk("insert %d entries time: %llu ns\n", i, (end_time - start_time));

}

static void iterate_entries(struct list_head* list, int count) {
    int i;
    struct list_head* elem;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    for (i = 0; i < count; ++i) {
        down_write(&lock);
        if (elem && elem->next) {
            elem = elem->next;
            entry = list_entry(elem, struct my_list_entry, list);
        }
        up_write(&lock);
    }
    end_time = ktime_get_ns();
    printk("iterate %d entries time: %llu ns\n", i, (end_time - start_time));
}

static void delete_entries(struct list_head* list, int count) {
    int i;
    struct list_head* elem;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    for (i = 0; i < count; ++i) {
        down_write(&lock);
        elem = list->next;
        entry = list_entry(elem, struct my_list_entry, list);
        list_del(elem);
        kfree(entry);
        up_write(&lock);
    }
    end_time = ktime_get_ns();
    printk("delete %d entries time: %llu ns\n", i, (end_time - start_time));
}

static int list_thread(void* arg) {
    insert_entries(&my_list, 25000);
    iterate_entries(&my_list, 25000);
    delete_entries(&my_list, 25000);
    return 0;
}

static int my_init(void)
{
    int i;
    printk("module loaded\n");
    INIT_LIST_HEAD(&my_list);
    init_rwsem(&lock);

    for (i = 0; i < 4; ++i)
        kthread_run(list_thread, NULL, "list_thread");

	return 0;
}

static void my_exit(void)
{
    printk("module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
