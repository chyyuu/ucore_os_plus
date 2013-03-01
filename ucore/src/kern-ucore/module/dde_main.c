/*
 * =====================================================================================
 *
 *       Filename:  module.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/17/2012 09:02:24 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/mm.h>

#define __NO_UCORE_TYPE__
#include <module.h>
#include <kio.h>
//#include <assert.h>

#ifdef __DEBUG
#define _TODO_() printk(KERN_ALERT "TODO %s\n", __func__)
#else
#define _TODO_()
#endif

/* Provided by the linker */
extern const struct kernel_symbol __start___ksymtab[];
extern const struct kernel_symbol __stop___ksymtab[];
extern const struct kernel_symbol __start___ksymtab_gpl[];
extern const struct kernel_symbol __stop___ksymtab_gpl[];
extern const struct kernel_symbol __start___ksymtab_gpl_future[];
extern const struct kernel_symbol __stop___ksymtab_gpl_future[];
extern const struct kernel_symbol __start___ksymtab_gpl_future[];
extern const struct kernel_symbol __stop___ksymtab_gpl_future[];
extern const unsigned long __start___kcrctab[];
extern const unsigned long __start___kcrctab_gpl[];
extern const unsigned long __start___kcrctab_gpl_future[];
#ifdef CONFIG_UNUSED_SYMBOLS
extern const struct kernel_symbol __start___ksymtab_unused[];
extern const struct kernel_symbol __stop___ksymtab_unused[];
extern const struct kernel_symbol __start___ksymtab_unused_gpl[];
extern const struct kernel_symbol __stop___ksymtab_unused_gpl[];
extern const unsigned long __start___kcrctab_unused[];
extern const unsigned long __start___kcrctab_unused_gpl[];
#endif

extern initcall_t __initcall_start[], __initcall_end[];

static void do_initcalls()
{
	int i;
	int cnt = __initcall_end - __initcall_start;
	kprintf("do_initcalls() %08x %08x %d calls, begin...\n",
		__initcall_start, __initcall_end, cnt);
	for (i = 0; i < cnt; i++) {
		kprintf("  calling 0x%08x\n", __initcall_start[i]);
		__initcall_start[i] ();
	}
	kprintf("do_initcalls() end!\n");
}

int request_module(const char *fmt, ...)
{
	return -EINVAL;
}

static void loadable_module_init()
{
	int i;
	int cnt = __stop___ksymtab - __start___ksymtab;
	for (i = 0; i < cnt; i++) {
		pr_debug("%d\t%08x\t%s\n", i, __start___ksymtab[i].value,
			 __start___ksymtab[i].name);
	}
}

void dde_init()
{
	extern init_dde_workqueue();
	/* default workqueue */
	if (init_dde_workqueue())
		panic("fail to create default workqueue");

	/* invoked in vfs_cache_init in Linux */
	chrdev_init();

	driver_init();
	dde_call_machine_init();
	do_initcalls();

	//loadable_module_init();
}

/* basic functions */
int printk(const char *s, ...)
{
	va_list ap;
	va_start(ap, s);
	int cnt = vkprintf(s, ap);
	va_end(ap);
	return cnt;
}

void panic(const char *fmt, ...)
{
	intr_disable();
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel panic in MODULE:\n    ");
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
}

void warn_slowpath(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel panic in %s:%d:\n    ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
}

void __attribute__ ((noreturn)) __bug(const char *file, int line)
{
	printk(KERN_CRIT "kernel BUG at %s:%d!\n", file, line);
	panic("0");
	/* Avoid "noreturn function does return" */
	for (;;) ;
}

int sprint_symbol(char *buffer, unsigned long address)
{
	char *modname;
	const char *name;
	unsigned long offset, size;
	int len;

	//name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
	name = NULL;
	if (!name)
		return sprintf(buffer, "0x%lx", address);

	if (name != buffer)
		strcpy(buffer, name);
	len = strlen(buffer);
	buffer += len;

	if (modname)
		len += sprintf(buffer, "+%#lx/%#lx [%s]",
			       offset, size, modname);
	else
		len += sprintf(buffer, "+%#lx/%#lx", offset, size);

	return len;
}

/* Look up a kernel symbol and print it to the kernel messages. */
void __print_symbol(const char *fmt, unsigned long address)
{
	printk("__print_symbol: %s, 0x%08x\n", fmt, address);
}

/* 
 * Optimised C version of memzero for the ARM.
 */
void __memzero(void *s, __kernel_size_t n)
{
	union {
		void *vp;
		unsigned long *ulp;
		unsigned char *ucp;
	} u;
	int i;

	u.vp = s;

	for (i = n >> 5; i > 0; i--) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 4) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 3) {
		*u.ulp++ = 0;
		*u.ulp++ = 0;
	}

	if (n & 1 << 2)
		*u.ulp++ = 0;

	if (n & 1 << 1) {
		*u.ucp++ = 0;
		*u.ucp++ = 0;
	}

	if (n & 1)
		*u.ucp++ = 0;
}

char *kstrdup(const char *s, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (!s)
		return NULL;

	len = strlen(s) + 1;
	buf = __kmalloc(len, gfp);
	if (buf)
		memcpy(buf, s, len);
	return buf;
}

EXPORT_SYMBOL(kstrdup);

/* mutux */
void __mutex_init(struct mutex *lock, const char *name,
		  struct lock_class_key *key)
{
}

extern void __init_rwsem(struct rw_semaphore *sem, const char *name,
			 struct lock_class_key *key)
{
}

void mutex_lock(struct mutex *lock)
{
	//TODO
}

int mutex_lock_interruptible(struct mutex *lock)
{
	return 0;
}

void mutex_unlock(struct mutex *lock)
{
	//TODO
}

void synchronize_rcu(void)
{

}

/* notifier */
int blocking_notifier_call_chain(struct blocking_notifier_head *nh,
				 unsigned long val, void *v)
{
	return 0;
}

extern int blocking_notifier_chain_register(struct blocking_notifier_head *nh,
					    struct notifier_block *nb)
{
	return 0;
}

extern int blocking_notifier_chain_unregister(struct blocking_notifier_head *nh,
					      struct notifier_block *nb)
{
	return 0;
}

/* sysfs */
int sysfs_create_link(struct kobject *kobj, struct kobject *target,
		      const char *name)
{
	_TODO_();
	return 0;
}

int sysfs_create_dir(struct kobject *kobj)
{
	_TODO_();
	return 0;
}

void sysfs_remove_dir(struct kobject *kobj)
{
	return;
}

int sysfs_rename_dir(struct kobject *kobj, const char *new_name)
{
	return 0;
}

int sysfs_move_dir(struct kobject *kobj, struct kobject *new_parent_kobj)
{
	return 0;
}

/**
 *      sysfs_create_link_nowarn - create symlink between two objects.
 *      @kobj:  object whose directory we're creating the link in.
 *      @target:        object we're pointing to.
 *      @name:          name of the symlink.
 *
 *      This function does the same as sysf_create_link(), but it
 *      doesn't warn if the link already exists.
 */
int sysfs_create_link_nowarn(struct kobject *kobj, struct kobject *target,
			     const char *name)
{
	return 0;
}

/**
 *      sysfs_remove_link - remove symlink in object's directory.
 *      @kobj:  object we're acting for.
 *      @name:  name of the symlink to remove.
 */

void sysfs_remove_link(struct kobject *kobj, const char *name)
{
}

int sysfs_create_file(struct kobject *kobj, const struct attribute *attr)
{
	_TODO_();
	return 0;
}

int sysfs_chmod_file(struct kobject *kobj, struct attribute *attr, mode_t mode)
{
	return 0;
}

void sysfs_remove_file(struct kobject *kobj, const struct attribute *attr)
{
	return;
}

int sysfs_create_group(struct kobject *kobj, const struct attribute_group *grp)
{
	return 0;
}

int sysfs_update_group(struct kobject *kobj, const struct attribute_group *grp)
{
	return 0;
}

void sysfs_remove_group(struct kobject *kobj, const struct attribute_group *grp)
{
}

int sysfs_create_bin_file(struct kobject *kobj, struct bin_attribute *attr)
{
	return 0;
}

void sysfs_remove_bin_file(struct kobject *kobj, struct bin_attribute *attr)
{
	return;
}

int sysfs_schedule_callback(struct kobject *kobj, void (*func) (void *),
			    void *data, struct module *owner)
{
	return 0;
}

int add_uevent_var(struct kobj_uevent_env *env, const char *format, ...)
{
	printk("INFO: add_uevent_var: %s\n", format);
	return 0;
}

int kobject_uevent(struct kobject *kobj, enum kobject_action action)
{
	return 0;
}

int kobject_uevent_env(struct kobject *kobj, enum kobject_action action,
		       char *envp[])
{
	return 0;
}

int kobject_action_type(const char *buf, size_t count,
			enum kobject_action *type)
{
	return 0;
}

/* firmware */
int firmware_init(void)
{
	return 0;
}

/* cpu */
int cpu_dev_init()
{
	return 0;
}

/* dma */
int
dma_declare_coherent_memory(struct device *dev, dma_addr_t bus_addr,
			    dma_addr_t device_addr, size_t size, int flags)
{
	return 0;
}

void dma_release_declared_memory(struct device *dev)
{
}

void *dma_mark_declared_memory_occupied(struct device *dev,
					dma_addr_t device_addr, size_t size)
{
	return ERR_PTR(-EBUSY);
}

void *dma_alloc_coherent(struct device *dev, size_t size,
			 dma_addr_t * dma_handle, gfp_t flag)
{
	//        return pci_alloc_consistent(to_pci_dev(dev), size, dma_handle);
	panic("TODO");
	return NULL;
}

void
dma_free_coherent(struct device *dev, size_t size, void *cpu_addr,
		  dma_addr_t dma_handle)
{
	printk(KERN_ALERT "TODO %s\n", __func__);
}

/* console */
void acquire_console_sem(void)
{
}

int try_acquire_console_sem(void)
{
	return 0;
}

void release_console_sem(void)
{
}

/* uaccess */

/* procfs */
struct proc_dir_entry *proc_create_data(const char *name, mode_t mode,
					struct proc_dir_entry *parent,
					const struct file_operations *proc_fops,
					void *data)
{
	//TODO
	printk(KERN_DEBUG "proc_create_data: %s\n", name);
	return NULL;
}

void remove_proc_entry(const char *name, struct proc_dir_entry *parent)
{

}

struct proc_dir_entry *proc_mkdir(const char *name, struct proc_dir_entry *p)
{
	return NULL;
}

int seq_open(struct file *file, const struct seq_operations *op)
{
	return -EINVAL;
}

ssize_t seq_read(struct file * file, char __user * buf, size_t size,
		 loff_t * ppos)
{
	return -EINVAL;
}

loff_t seq_lseek(struct file * file, loff_t offset, int origin)
{
	return -EINVAL;
}

int seq_release(struct inode *inode, struct file *file)
{
	return -EINVAL;
}

int seq_escape(struct seq_file *m, const char *s, const char *esc)
{
	return -EINVAL;
}

int seq_putc(struct seq_file *m, char c)
{
	return -EINVAL;
}

int seq_puts(struct seq_file *m, const char *s)
{
	return -EINVAL;
}

int seq_write(struct seq_file *seq, const void *data, size_t len)
{
	return -EINVAL;
}

int seq_printf(struct seq_file *m, const char *f, ...)
{
	return -EINVAL;
}

struct list_head *seq_list_start(struct list_head *head, loff_t pos)
{
	return NULL;
}

struct list_head *seq_list_next(void *v, struct list_head *head, loff_t * ppos)
{
	return NULL;
}

/* vmm */
int remap_pfn_range(struct vm_area_struct *vma, unsigned long addr,
		    unsigned long pfn, unsigned long size, pgprot_t prot)
{
	if (vma->vm_start != addr || vma->vm_end != addr + size)
		return -EINVAL;
	vma->vm_pgoff = pfn;
	//TODO
	void *r = ucore_map_pfn_range(addr, pfn, size, VM_IO);
	if (!r) {
		return -ENOMEM;
	}
	vma->vm_start = (unsigned long)r;
	return 0;
}

/* bdi api, what is it? */
int bdi_init(struct backing_dev_info *bdi)
{
	return 0;
}
