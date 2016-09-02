#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/io.h>

/*
 * Example usage: sec_log=256K@0x45000000
 * In above case, log_buf size is 256KB and its base address is
 * 0x45000000 physically. Actually, *(int *)(base - 8) is log_magic and
 * *(int *)(base - 4) is log_ptr. So we reserve (size + 8) bytes from
 * (base - 8).
 */
#define LOG_MAGIC 0x4d474f4c	/* "LOGM" */

/* These variables are also protected by logbuf_lock */
static unsigned *sec_log_ptr;
static char *sec_log_buf;
static unsigned sec_log_size;

//static char *last_kmsg_buffer;
static char last_kmsg_buffer[1 << CONFIG_LOG_BUF_SHIFT];
static unsigned last_kmsg_size;
static void __init sec_log_save_old(void);

extern void register_log_char_hook(void (*f) (char c));

static inline void emit_sec_log_char(char c)
{
	if (sec_log_buf && sec_log_ptr) {
		sec_log_buf[*sec_log_ptr & (sec_log_size - 1)] = c;
		(*sec_log_ptr)++;
	}
}

static unsigned sec_log_save_size;
static unsigned long long sec_log_save_base;

static int __init sec_log_setup(char *str)
{
	unsigned size = memparse(str, &str);

	if (size && (size == roundup_pow_of_two(size)) && (*str == '@')) {
		unsigned long long base;
		base = simple_strtoul(++str, &str, 0);

		sec_log_save_size = size;
		sec_log_save_base = base;
		sec_log_size = size;

		return 1;

	}
	return 1;
}

static int __init sec_log_remap_nocache(void)
{
	unsigned *sec_log_mag;
	void __iomem *nocache_base = 0;

	pr_err("%s: sec_log_save_size %d at sec_log_save_base 0x%x\n",
	__func__, sec_log_save_size, (unsigned int)sec_log_save_base);

	nocache_base = ioremap_nocache(sec_log_save_base - 4096,
		sec_log_save_size + 8192);
		nocache_base = nocache_base + 4096;

	sec_log_mag = nocache_base - 8;
	sec_log_ptr = nocache_base - 4;
	sec_log_buf = nocache_base;
	sec_log_size = sec_log_save_size;

	if (*sec_log_mag != LOG_MAGIC) {
		pr_info("%s: no old log found\n", __func__);
		*sec_log_ptr = 0;
		*sec_log_mag = LOG_MAGIC;
	} else
		sec_log_save_old();

	register_log_char_hook(emit_sec_log_char);

//koo8397 temp close	sec_getlog_supply_kloginfo(phys_to_virt(base));

out:
	return 0;
}

__setup("sec_log=", sec_log_setup);

static void __init sec_log_save_old(void)
{
	/* provide previous log as last_kmsg */
	last_kmsg_size = min((unsigned)(1 << CONFIG_LOG_BUF_SHIFT), *sec_log_ptr);
	//last_kmsg_buffer = (char *)sec_log_buf;//(char *)alloc_bootmem(last_kmsg_size);

	if (last_kmsg_size && last_kmsg_buffer) {
		unsigned i;
		for (i = 0; i < last_kmsg_size; i++)
			last_kmsg_buffer[i] =
			    sec_log_buf[(*sec_log_ptr - last_kmsg_size +
					 i) & (sec_log_size - 1)];

		pr_info("%s: saved old log at %d@%p\n",
			__func__, last_kmsg_size, last_kmsg_buffer);
	} else
		pr_err("%s: failed saving old log %d@%p\n",
		       __func__, last_kmsg_size, last_kmsg_buffer);
}

static ssize_t sec_log_read_old(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= last_kmsg_size)
		return 0;

	count = min(len, (size_t) (last_kmsg_size - pos));
	if (copy_to_user(buf, last_kmsg_buffer + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}

static const struct file_operations last_kmsg_file_ops = {
	.owner = THIS_MODULE,
	.read = sec_log_read_old,
};

static int __init sec_log_late_init(void)
{
	struct proc_dir_entry *entry;

	if (last_kmsg_buffer == NULL)
		return 0;

	entry = create_proc_entry("last_kmsg", S_IFREG | S_IRUGO, NULL);
	if (!entry) {
		pr_err("%s: failed to create proc entry\n", __func__);
		return 0;
	}

	entry->proc_fops = &last_kmsg_file_ops;
	entry->size = last_kmsg_size;
	return 0;
}

module_init(sec_log_remap_nocache);
late_initcall(sec_log_late_init);