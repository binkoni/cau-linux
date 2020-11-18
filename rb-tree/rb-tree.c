#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/rbtree.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>

struct my_node {
	struct rb_node node;
	int key;
	int value;
};

void insert_entries(struct rb_root* tree, int count) {
	int i;
	struct my_node* entry;
	struct rb_node **where;
	struct rb_node *parent;

	u64 start_time, end_time;
	start_time = ktime_get_ns();
	for (i = 0; i < count; ++i) {
		entry = kmalloc(sizeof(struct my_node), GFP_KERNEL);
		entry->key = i;
		entry->value = i * 100;
		
        where = &tree->rb_node;
        parent = NULL;
		while (*where) {
		    parent = *where;
    		if (entry->key < rb_entry(parent, struct my_node, node)->key)
    			where = &parent->rb_left;
    		else
    			where = &parent->rb_right;
    	}
        rb_link_node(&entry->node, parent, where);
		rb_insert_color(&entry->node, tree);
	}
	end_time = ktime_get_ns();
	printk("insert %d entries time: %llu ns\n", count, (end_time - start_time));

}

void iterate_entries(struct rb_root* tree) {
	int count = 0;
	struct rb_node* node;
	struct my_node* entry;
	u64 start_time, end_time;
	start_time = ktime_get_ns();
	for (node = rb_first(tree); node; node = rb_next(node)) {
		entry = rb_entry(node, struct my_node, node);
		++count;
	}
	end_time = ktime_get_ns();
	printk("iterate %d entries time: %llu ns\n", count, (end_time - start_time));
}

void delete_entries(struct rb_root* tree) {
	int count = 0;
	struct rb_node* node;
	struct my_node* entry;
	u64 start_time, end_time;
	start_time = ktime_get_ns();
	for (node = rb_first(tree); node; node = rb_next(node)) {
		entry = rb_entry(node, struct my_node, node);
		rb_erase(&entry->node, tree);
		++count;
	}
	end_time = ktime_get_ns();
	printk("delete %d entries time: %llu ns\n", count, (end_time - start_time));
}

static int my_init(void)
{
	struct rb_root my_tree = RB_ROOT;
	printk("module loaded\n");

	insert_entries(&my_tree, 1000);
	iterate_entries(&my_tree);
	delete_entries(&my_tree);

	insert_entries(&my_tree, 10000);
	iterate_entries(&my_tree);
	delete_entries(&my_tree);

	insert_entries(&my_tree, 100000);
	iterate_entries(&my_tree);
	delete_entries(&my_tree);

	return 0;
}

static void my_exit(void)
{
	printk("module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
