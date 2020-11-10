#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>

struct my_list_entry {
    struct list_head list;
    int data;
};

void insert_entries(struct list_head* list, int count) {
    int i;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    for (i = 0; i < count; ++i) {
        entry = kmalloc(sizeof(struct my_list_entry), GFP_KERNEL);
        entry->data = i;
        list_add_tail(&entry->list, list);
    }
    end_time = ktime_get_ns();
    printk("insert %d entries time: %llu ns\n", count, (end_time - start_time));

}

void iterate_entries(struct list_head* list) {
    int count = 0;
    struct list_head* elem;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    list_for_each(elem, list) {
        entry = list_entry(elem, struct my_list_entry, list);
        // printk("current value: %d\n", entry->data);
        ++count;
    }
    end_time = ktime_get_ns();
    printk("iterate %d entries time: %llu ns\n", count, (end_time - start_time));
}

void delete_entries(struct list_head* list) {
    int count = 0;
    struct list_head* elem;
    struct list_head* tmp;
    struct my_list_entry* entry;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    list_for_each_safe(elem, tmp, list) {
        entry = list_entry(elem, struct my_list_entry, list);
        list_del(elem);
        kfree(entry);
        ++count;
    }
    end_time = ktime_get_ns();
    printk("delete %d entries time: %llu ns\n", count, (end_time - start_time));
}

static int my_init(void)
{
    struct list_head my_list;
    printk("module loaded\n");
    INIT_LIST_HEAD(&my_list);

    insert_entries(&my_list, 1000);
    iterate_entries(&my_list);
    delete_entries(&my_list);

    insert_entries(&my_list, 10000);
    iterate_entries(&my_list);
    delete_entries(&my_list);

    insert_entries(&my_list, 100000);
    iterate_entries(&my_list);
    delete_entries(&my_list);

	return 0;
}

static void my_exit(void)
{
    printk("module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
