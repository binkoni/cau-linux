#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>

struct lentry {
    struct list_head elem;
    int data;
};

struct xlist;

struct xlist_thread_arg {
    struct task_struct* task;
    struct xlist* xlist;
    struct list_head* list;
    union {
        struct {
            size_t nr_entries;
            struct lentry* entries;
        } insert;
        struct {
            int (*cond)(struct lentry*);
            struct lentry** found;
        } search;
    };
};

struct xlist {
    size_t nr_lists;
    size_t nr_waiting;
    struct list_head* lists;
    struct xlist_thread_arg* args;
};

int xlist_insert_thread(void* arg) {
    int i;
    struct xlist_thread_arg* xlist_thread_arg = arg;
    struct task_struct* task = xlist_thread_arg->task;
    struct xlist* xlist = xlist_thread_arg->xlist;
    struct list_head* list = xlist_thread_arg->list;
    size_t nr_entries = xlist_thread_arg->insert.nr_entries;
    struct lentry* entries = xlist_thread_arg->insert.entries;
    printk("going to insert %ld entries\n", nr_entries);
    for (i = 0; i < nr_entries; ++i) {
        printk("insert %d\n", entries[i].data);
        list_add_tail(&entries[i].elem, list);
    }
    __sync_fetch_and_sub(&(xlist)->nr_waiting, 1);
    wake_up_process(task);
    return 0;
}

int xlist_search_thread(void* arg) {
    struct xlist_thread_arg* xlist_thread_arg = arg;
    struct task_struct* task = xlist_thread_arg->task;
    struct xlist* xlist = xlist_thread_arg->xlist;
    struct list_head* list = xlist_thread_arg->list;
    struct list_head* elem;
    struct lentry* entry;
    int (*cond)(struct lentry*) = xlist_thread_arg->search.cond;
    struct lentry** found = xlist_thread_arg->search.found;
    *found = NULL;
    list_for_each(elem, list) {
        entry = list_entry(elem, struct lentry, elem);
        if (cond(entry)) {
            __sync_bool_compare_and_swap(found, NULL, entry);
            break;
        }
    }
    __sync_fetch_and_sub(&(xlist)->nr_waiting, 1);
    wake_up_process(task);
    return 0;
}

#define XLIST_INIT(_xlist, _nr_lists) \
do { \
    int i; \
    (_xlist)->nr_lists = _nr_lists; \
    (_xlist)->lists = kmalloc(_nr_lists * sizeof(struct list_head), GFP_KERNEL); \
    (_xlist)->nr_waiting = 0; \
    (_xlist)->args = kmalloc(_nr_lists * sizeof(struct xlist_thread_arg), GFP_KERNEL); \
    for (i = 0; i < _nr_lists; ++i) { \
        INIT_LIST_HEAD(&(_xlist)->lists[i]); \
        (_xlist)->args[i].task = current; \
        (_xlist)->args[i].xlist = _xlist; \
        (_xlist)->args[i].list = &(_xlist)->lists[i]; \
    } \
} while (0)

#define XLIST_ADD_TAIL(_entries, _nr_entries, xlist) \
do { \
    int i; \
    for (i = 0; i < (xlist)->nr_lists; ++i) { \
        (xlist)->args[i].insert.nr_entries = _nr_entries / (xlist)->nr_lists; \
        (xlist)->args[i].insert.entries = (*_entries) + _nr_entries / (xlist)->nr_lists * i; \
        __sync_fetch_and_add(&(xlist)->nr_waiting, 1); \
        kthread_run(xlist_insert_thread, &(xlist)->args[i], "xlist_insert_thread"); \
    } \
    set_current_state(TASK_INTERRUPTIBLE); \
    while ((xlist)->nr_waiting) { \
        schedule(); \
        set_current_state(TASK_INTERRUPTIBLE); \
    } \
    set_current_state(TASK_RUNNING); \
} while (0)

#define XLIST_FIND(_found, _cond, xlist) \
do { \
    int i; \
    for (i = 0; i < (xlist)->nr_lists; ++i) { \
        (xlist)->args[i].search.cond = _cond; \
        (xlist)->args[i].search.found = _found; \
        __sync_fetch_and_add(&(xlist)->nr_waiting, 1); \
        kthread_run(xlist_search_thread, &(xlist)->args[i], "xlist_search_thread"); \
    } \
    set_current_state(TASK_INTERRUPTIBLE); \
    while ((xlist)->nr_waiting) { \
        schedule(); \
        set_current_state(TASK_INTERRUPTIBLE); \
    } \
} while (0)

#define XLIST_FOR_EACH_START(elem, xlist) \
do { \
    int i; \
    for (i = 0; i < (xlist)->nr_lists; ++i) { \
        list_for_each(elem, &(xlist)->lists[i]) {

#define XLIST_FOR_EACH_END \
        } \
    } \
} while(0);

#define XLIST_FOR_EACH_SAFE_START(elem, tmp, xlist) \
do { \
    int i; \
    for (i = 0; i < (xlist)->nr_lists; ++i) { \
        list_for_each_safe(elem, tmp, &(xlist)->lists[i]) {

#define XLIST_FOR_EACH_SAFE_END \
        } \
    } \
} while(0);

/*
void insert_entries(struct xlist* xlist, int count) {
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
*/

int find_1000(struct lentry* entry) {
    return entry->data == 1000;
}

static int my_init(void)
{
    int i;
    struct xlist xlist;
    struct lentry* entries;
    struct list_head* elem;
    struct lentry* entry;

    printk("module loaded\n");
    XLIST_INIT(&xlist, 4);
    entries = kmalloc(100 * sizeof(struct lentry), GFP_KERNEL);
    for (i = 0; i < 100; ++i) {
        entries[i].data = i * 100;
    }
    XLIST_ADD_TAIL(&entries, 100, &xlist);
    XLIST_FOR_EACH_START(elem, &xlist)
        entry = list_entry(elem, struct lentry, elem);
        printk("current value: %d\n", entry->data);
    XLIST_FOR_EACH_END

    // XLIST_FIND(&entry, &find_1000, &xlist);
    printk("found %d\n", entry->data);

	return 0;
}

static void my_exit(void)
{
    printk("module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
