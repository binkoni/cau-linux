#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/kthread.h>

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
    for (i = 0; i < nr_entries; ++i) {
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
    list_for_each(elem, list) {
        entry = list_entry(elem, struct lentry, elem);
        if (cond(entry)) {
            *found = entry;
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
        (xlist)->args[i].insert.entries = _entries + _nr_entries / (xlist)->nr_lists * i; \
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
    *_found = NULL; \
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
    set_current_state(TASK_RUNNING); \
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

void insert_entries_xlist(struct lentry* entries, size_t nr_entries, struct xlist* xlist) {
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    XLIST_ADD_TAIL(entries, nr_entries, xlist);
    end_time = ktime_get_ns();
    printk("insert %ld entries into xlist time: %llu ns\n", nr_entries, (end_time - start_time));
}

void insert_entries_list(struct lentry* entries, size_t nr_entries, struct list_head* list) {
    int i;
    u64 start_time, end_time;
    start_time = ktime_get_ns();
    for (i = 0; i < nr_entries; ++i) {
        list_add_tail(&entries[i].elem, list);
    }
    end_time = ktime_get_ns();
    printk("insert %ld entries into list time: %llu ns\n", nr_entries, (end_time - start_time));
}

void iterate_entries_xlist(struct xlist* xlist) {
    u64 start_time, end_time;
    struct list_head* elem;
    struct lentry* entry;
    size_t count;
    count = 0;
    start_time = ktime_get_ns();
    XLIST_FOR_EACH_START(elem, xlist)
        entry = list_entry(elem, struct lentry, elem);
        ++count;
    XLIST_FOR_EACH_END
    end_time = ktime_get_ns();
    printk("iterate %ld entries in xlist time: %llu ns\n", count, (end_time - start_time));
}

void find_entry_xlist(int (*cond)(struct lentry*), struct xlist* xlist) {
    u64 start_time, end_time;
    struct lentry* entry;
    start_time = ktime_get_ns();
    XLIST_FIND(&entry, cond, xlist);
    if (entry) {
        // printk("found %d\n", entry->data);
    } else {
        // printk("not found\n");
    }
    end_time = ktime_get_ns();
    printk("find entry in xlist time: %llu ns\n", (end_time - start_time));

}

void iterate_entries_list(struct list_head* list) {
    u64 start_time, end_time;
    struct list_head* elem;
    struct lentry* entry;
    size_t count;
    count = 0;
    start_time = ktime_get_ns();
    list_for_each(elem, list) {
        entry = list_entry(elem, struct lentry, elem);
        ++count;
    }
    end_time = ktime_get_ns();
    printk("iterate %ld entries in list time: %llu ns\n", count, (end_time - start_time));
}

void find_entry_list(int (*cond)(struct lentry*), struct list_head* list) {
    u64 start_time, end_time;
    struct lentry* entry;
    struct list_head* elem;
    int found = 0;
    start_time = ktime_get_ns();
    list_for_each(elem, list) {
        entry = list_entry(elem, struct lentry, elem);
        if (cond(entry)) {
            found = 1;
            // printk("found %d\n", entry->data);
            break;
        }
    }
    if (!found) {
        // printk("not found\n");
    }
    end_time = ktime_get_ns();
    printk("find entry in list time: %llu ns\n", (end_time - start_time));
}

void delete_entries_xlist(struct xlist* xlist) {
    u64 start_time, end_time;
    struct list_head* elem;
    struct list_head* tmp;
    struct lentry* entry;
    size_t count;
    count = 0;
    start_time = ktime_get_ns();
    XLIST_FOR_EACH_SAFE_START(elem, tmp, xlist)
        entry = list_entry(elem, struct lentry, elem);
        list_del(elem);
        ++count;
    XLIST_FOR_EACH_SAFE_END
    end_time = ktime_get_ns();
    printk("delete %ld entries from xlist time: %llu ns\n", count, (end_time - start_time));
}

void delete_entries_list(struct list_head* list) {
    u64 start_time, end_time;
    struct list_head* elem;
    struct list_head* tmp;
    struct lentry* entry;
    size_t count;
    count = 0;
    start_time = ktime_get_ns();
    list_for_each_safe(elem, tmp, list) {
        entry = list_entry(elem, struct lentry, elem);
        list_del(elem);
        ++count;
    }
    end_time = ktime_get_ns();
    printk("delete %ld entries from list time: %llu ns\n", count, (end_time - start_time));
}

int find_90000(struct lentry* entry) {
    return entry->data == 90000;
}

void compare_ds(size_t nr_entries) {
    int i;
    struct xlist xlist;
    struct list_head list;
    struct lentry* entries;

    XLIST_INIT(&xlist, 4);
    INIT_LIST_HEAD(&list);

    entries = kmalloc(nr_entries * sizeof(struct lentry), GFP_KERNEL);
    for (i = 0; i < nr_entries; ++i) {
        entries[i].data = i * 100;
    }

    insert_entries_xlist(entries, nr_entries, &xlist);
    iterate_entries_xlist(&xlist);
    find_entry_xlist(find_90000, &xlist);
    delete_entries_xlist(&xlist);

    insert_entries_list(entries, nr_entries, &list);
    iterate_entries_list(&list);
    find_entry_list(find_90000, &list);

    kfree(entries);
}

static int my_init(void)
{
    printk("module loaded\n");

    compare_ds(4);
    compare_ds(40);
    compare_ds(100);
    compare_ds(1000);
    compare_ds(10000);
    compare_ds(100000);

	return 0;
}

static void my_exit(void)
{
    printk("module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
