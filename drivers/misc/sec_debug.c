/* sec_debug.c
 *
 * Exception handling in kernel by SEC
 *
 * Copyright (c) 2011 Samsung Electronics
 *                http://www.samsung.com/
 */

#ifdef CONFIG_SEC_DEBUG
#include <linux/errno.h>
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/input.h>

#include <linux/file.h>
#include <mach/hardware.h>

#include <mach/msm_iomap.h>

#include <linux/sec_debug.h>
#include <asm/cacheflush.h>
#include <linux/smp.h>
#include <linux/cpu.h>

#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
#include <linux/circ_buf.h>
#endif

#include "../arch/arm/mach-msm/smd_private.h"

#ifdef CONFIG_SEC_SINGLE_CORE
#define CONFIG_NR_CPUS 1
#endif

/********************************
 *  Variable
 *********************************/
 
typedef struct  {
	int SCTLR;
	int TTBR0;
	int TTBR1;
	int TTBCR;
	int DACR;
	int DFSR;
	int DFAR;
	int IFSR;
	int IFAR;
	int DAFSR;
	int IAFSR;
	int PMRRR;
	int NMRRR;
	int FCSEPID;
	int CONTEXT;
	int URWTPID;
	int UROTPID;
	int POTPIDR;
}sec_debug_mmu_reg_t;

/* ARM CORE regs mapping structure */
typedef struct {
	/* COMMON */
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;

	/* SVC */
	unsigned int r13_svc;
	unsigned int r14_svc;
	unsigned int spsr_svc;

	/* PC & CPSR */
	unsigned int pc;
	unsigned int cpsr;

	/* USR/SYS */
	unsigned int r13_usr;
	unsigned int r14_usr;

	/* FIQ */
	unsigned int r8_fiq;
	unsigned int r9_fiq;
	unsigned int r10_fiq;
	unsigned int r11_fiq;
	unsigned int r12_fiq;
	unsigned int r13_fiq;
	unsigned int r14_fiq;
	unsigned int spsr_fiq;

	/* IRQ */
	unsigned int r13_irq;
	unsigned int r14_irq;
	unsigned int spsr_irq;

	/* MON */
	unsigned int r13_mon;
	unsigned int r14_mon;
	unsigned int spsr_mon;

	/* ABT */
	unsigned int r13_abt;
	unsigned int r14_abt;
	unsigned int spsr_abt;

	/* UNDEF */
	unsigned int r13_und;
	unsigned int r14_und;
	unsigned int spsr_und;
}sec_debug_core_t;

static int save_final_context;
static unsigned int secdbg_paddr;
static unsigned int secdbg_size;

DEFINE_PER_CPU(sec_debug_core_t, sec_debug_core_reg);
DEFINE_PER_CPU(sec_debug_mmu_reg_t, sec_debug_mmu_reg);

static int force_error(const char *val, struct kernel_param *kp);
module_param_call(force_error, force_error, NULL, NULL, 0644);

#ifdef CONFIG_SEC_DEBUG_SUBSYS
struct sec_debug_subsys *secdbg_subsys;
struct sec_debug_subsys_data_scorpion *secdbg_scorpion;
#endif

static unsigned int secdbg_paddr;
static unsigned int secdbg_size;
static unsigned int sec_subsys_paddr;
static unsigned int sec_subsys_size;

static char sec_build_info[100];
static char *sec_build_time[] = {
	__DATE__,
	__TIME__
};

/* klaatu - schedule log */
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
struct sec_debug_log {
	atomic_t idx_sched[CONFIG_NR_CPUS];
	struct sched_log sched[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_irq[CONFIG_NR_CPUS];
	struct irq_log irq[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_irq_exit[CONFIG_NR_CPUS];
	struct irq_exit_log irq_exit[CONFIG_NR_CPUS][SCHED_LOG_MAX];

	atomic_t idx_timer[CONFIG_NR_CPUS];
	struct timer_log timer_log[CONFIG_NR_CPUS][SCHED_LOG_MAX];

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
	atomic_t idx_secmsg[CONFIG_NR_CPUS];
	struct secmsg_log secmsg[CONFIG_NR_CPUS][MSG_LOG_MAX];
#endif
#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
	atomic_t dcvs_log_idx ;
	struct dcvs_debug dcvs_log[DCVS_LOG_MAX] ;
#endif
#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
	atomic_t fg_log_idx;
	struct fuelgauge_debug fg_log[FG_LOG_MAX] ;
#endif
};

struct sec_debug_subsys *secdbg_subsys;
struct sec_debug_subsys_data_krait *secdbg_krait;

#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

struct sec_debug_log *secdbg_log;


#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
extern void * high_memory;
#endif

/* klaatu - semaphore log */
#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
static struct sem_debug sem_debug_free_head;
static struct sem_debug sem_debug_done_head;
static int sem_debug_free_head_cnt;
static int sem_debug_done_head_cnt;
static int sem_debug_init;
static spinlock_t sem_debug_lock;

/* rwsemaphore logging */
static struct rwsem_debug rwsem_debug_free_head;
static struct rwsem_debug rwsem_debug_done_head;
static int rwsem_debug_free_head_cnt;
static int rwsem_debug_done_head_cnt;
static int rwsem_debug_init;
static spinlock_t rwsem_debug_lock;
#endif	/* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

/********************************
 *  Define
 *********************************/
#define LOCKUP_FIRST_KEY KEY_VOLUMEUP
#define LOCKUP_SECOND_KEY KEY_VOLUMEDOWN
#define LOCKUP_THIRD_KEY KEY_HOME

/********************************
 *  Function
 *********************************/
/* core reg dump function*/
__used sec_debug_core_t sec_debug_core_reg;
__used sec_debug_mmu_reg_t sec_debug_mmu_reg;

#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
#define KFREE_HOOK_BYPASS_MASK 0x1
#define KFREE_CIRC_BUF_SIZE (1<<15)
#define KFREE_FREE_MAGIC 0xf4eef4ee
static int dfd_disabled;
static DEFINE_SPINLOCK(circ_buf_lock);

struct kfree_info_entry {
	void *addr;
	void *caller;
};

struct kfree_circ_buf {
	int head;
	int tail;
	struct kfree_info_entry entry[KFREE_CIRC_BUF_SIZE];
};

struct kfree_circ_buf kfree_circ_buf;

static void *circ_buf_lookup(struct kfree_circ_buf *circ_buf, void *addr)
{
	int i;
	for (i = circ_buf->tail; i != circ_buf->head ;
		i = (i + 1) & (KFREE_CIRC_BUF_SIZE - 1)) {
		if (circ_buf->entry[i].addr == addr)
			return &circ_buf->entry[i];
	}

	return NULL;
}

static void *circ_buf_get(struct kfree_circ_buf *circ_buf)
{
	void *entry;
	entry = &circ_buf->entry[circ_buf->tail];
	smp_rmb();
	circ_buf->tail = (circ_buf->tail + 1) &
		(KFREE_CIRC_BUF_SIZE - 1);
	return entry;
}
static void *circ_buf_put(struct kfree_circ_buf *circ_buf,
				struct kfree_info_entry *entry)
{
	memcpy(&circ_buf->entry[circ_buf->head], entry, sizeof(*entry));
	smp_wmb();
	circ_buf->head = (circ_buf->head + 1) &
		(KFREE_CIRC_BUF_SIZE - 1);
	return entry;
}

static int dfd_flush(void)
{
	struct kfree_info_entry *pentry;
	unsigned long cnt;
	unsigned long flags;

	spin_lock_irqsave(&circ_buf_lock, flags);
	cnt = CIRC_CNT(kfree_circ_buf.head, kfree_circ_buf.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&circ_buf_lock, flags);
	pr_debug("%s: cnt=%lu\n", __func__, cnt);

do_flush:
	while (cnt) {
		pentry = NULL;
		/* we want to keep the lock region as short as possible
		 * so we will re-read the buf count every loop */
		spin_lock_irqsave(&circ_buf_lock, flags);
		cnt = CIRC_CNT(kfree_circ_buf.head, kfree_circ_buf.tail,
			KFREE_CIRC_BUF_SIZE);
		if (cnt == 0) {
			spin_unlock_irqrestore(&circ_buf_lock, flags);
			break;
		}
		pentry = circ_buf_get(&kfree_circ_buf);
		spin_unlock_irqrestore(&circ_buf_lock, flags);
		if (pentry)
			kfree((void *)((unsigned long)pentry->addr |
				KFREE_HOOK_BYPASS_MASK));
		cnt--;
	}

	spin_lock_irqsave(&circ_buf_lock, flags);
	cnt = CIRC_CNT(kfree_circ_buf.head, kfree_circ_buf.tail,
		KFREE_CIRC_BUF_SIZE);
	spin_unlock_irqrestore(&circ_buf_lock, flags);

	if (!dfd_disabled)
		goto out;

	if (cnt)
		goto do_flush;

out:
	return cnt;
}

static void dfd_disable(void)
{
	dfd_disabled = 1;
	dfd_flush();
	pr_info("%s: double free detection is disabled\n", __func__);
}

static void dfd_enable(void)
{
	dfd_disabled = 0;
	pr_info("%s: double free detection is enabled\n", __func__);
}

void *kfree_hook(void *p, void *caller)
{
	unsigned int flags;
	struct kfree_info_entry *match = NULL;
	void *tofree = NULL;
	unsigned int addr = (unsigned int)p;

	if (!virt_addr_valid(addr)) {
		/* there are too many NULL pointers so don't print for NULL */
		if (addr)
			pr_debug("%s: trying to free an invalid addr %x from %pS\n",
				__func__, addr, caller);
		return NULL;
	}

	if (addr&0x1) {
		/* return original address to free */
		return (void *)(addr&~(KFREE_HOOK_BYPASS_MASK));
	}

	spin_lock_irqsave(&circ_buf_lock, flags);

	if (kfree_circ_buf.head == 0)
		pr_debug("%s: circular buffer head rounded to zero.", __func__);

	if (*(unsigned int *)p == KFREE_FREE_MAGIC) {
		/* memory that is to be freed may originally have had magic
		 * value, so search the whole circ buf for an actual match */
		match = circ_buf_lookup(&kfree_circ_buf, p);
	}

	if (match) {
		pr_err("%s: 0x%08x was already freed by %pS()\n",
			__func__, (unsigned int)p, match->caller);
		spin_unlock_irqrestore(&circ_buf_lock, flags);
		panic("double free detected!");
		return NULL;
	} else {
		struct kfree_info_entry entry;

		/* mark free magic on the freeing node */
		*(unsigned int *)p = KFREE_FREE_MAGIC;

		/* do an actual kfree for the oldest entry
		 * if the circular buffer is full */
		if (CIRC_SPACE(kfree_circ_buf.head, kfree_circ_buf.tail,
			KFREE_CIRC_BUF_SIZE) == 0) {
			struct kfree_info_entry *pentry;
			pentry = circ_buf_get(&kfree_circ_buf);
			if (pentry)
				tofree = pentry->addr;
		}

		/* add the new entry to the circular buffer */
		entry.addr = p;
		entry.caller = caller;
		circ_buf_put(&kfree_circ_buf, &entry);
		if (tofree) {
			spin_unlock_irqrestore(&circ_buf_lock, flags);
			kfree((void *)((unsigned int)tofree |
				KFREE_HOOK_BYPASS_MASK));
			return NULL;
		}
	}

	spin_unlock_irqrestore(&circ_buf_lock, flags);
	return NULL;
}
#endif


static void sec_debug_save_core_reg(sec_debug_core_t *core_reg)
{
	asm(
		/* we will be in SVC mode when we enter this function.
		* Collect SVC registers along with cmn registers.
		*/
		"str r0, [%0,#0]\n\t"
		"str r1, [%0,#4]\n\t"
		"str r2, [%0,#8]\n\t"
		"str r3, [%0,#12]\n\t"
		"str r4, [%0,#16]\n\t"
		"str r5, [%0,#20]\n\t"
		"str r6, [%0,#24]\n\t"
		"str r7, [%0,#28]\n\t"
		"str r8, [%0,#32]\n\t"
		"str r9, [%0,#36]\n\t"
		"str r10, [%0,#40]\n\t"
		"str r11, [%0,#44]\n\t"
		"str r12, [%0,#48]\n\t"

		/* SVC */
		"str r13, [%0,#52]\n\t"
		"str r14, [%0,#56]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#60]\n\t"

		/* PC and CPSR */
		"sub r1, r15, #0x4\n\t"
		"str r1, [%0,#64]\n\t"
		"mrs r1, cpsr\n\t"
		"str r1, [%0,#68]\n\t"

		/* SYS/USR */
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1f\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#72]\n\t"
		"str r14, [%0,#76]\n\t"

		/*FIQ*/
		"mrs r1, cpsr\n\t"
		"and r1,r1,#0xFFFFFFE0\n\t"
		"orr r1,r1,#0x11\n\t"
		"msr cpsr,r1\n\t"

		"str r8, [%0,#80]\n\t"
		"str r9, [%0,#84]\n\t"
		"str r10, [%0,#88]\n\t"
		"str r11, [%0,#92]\n\t"
		"str r12, [%0,#96]\n\t"
		"str r13, [%0,#100]\n\t"
		"str r14, [%0,#104]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#108]\n\t"

		/*IRQ*/
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x12\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#112]\n\t"
		"str r14, [%0,#116]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#120]\n\t"

		/*MON*/
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x16\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#124]\n\t"
		"str r14, [%0,#128]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#132]\n\t"

		/*ABT*/
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x17\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#136]\n\t"
		"str r14, [%0,#140]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#144]\n\t"

		/*UND*/
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x1B\n\t"
		"msr cpsr,r1\n\t"

		"str r13, [%0,#148]\n\t"
		"str r14, [%0,#152]\n\t"
		"mrs r1, spsr\n\t"
		"str r1, [%0,#156]\n\t"

		/* restore to SVC mode */
		"mrs r1, cpsr\n\t"
		"and r1, r1, #0xFFFFFFE0\n\t"
		"orr r1, r1, #0x13\n\t"
		"msr cpsr,r1\n\t"

	:			/* output */
		: "r"(core_reg) 		/* input */
	: "%r1" 	/* clobbered register */
	);

	return;
}




static void sec_debug_save_mmu_reg(sec_debug_mmu_reg_t *mmu_reg)
{
	asm("mrc    p15, 0, r1, c1, c0, 0\n\t"	/* SCTLR */
		"str r1, [%0]\n\t"
		"mrc    p15, 0, r1, c2, c0, 0\n\t"	/* TTBR0 */
		"str r1, [%0,#4]\n\t"
		"mrc    p15, 0, r1, c2, c0,1\n\t"	/* TTBR1 */
		"str r1, [%0,#8]\n\t"
		"mrc    p15, 0, r1, c2, c0,2\n\t"	/* TTBCR */
		"str r1, [%0,#12]\n\t"
		"mrc    p15, 0, r1, c3, c0,0\n\t"	/* DACR */
		"str r1, [%0,#16]\n\t"
		"mrc    p15, 0, r1, c5, c0,0\n\t"	/* DFSR */
		"str r1, [%0,#20]\n\t"
		"mrc    p15, 0, r1, c6, c0,0\n\t"	/* DFAR */
		"str r1, [%0,#24]\n\t"
		"mrc    p15, 0, r1, c5, c0,1\n\t"	/* IFSR */
		"str r1, [%0,#28]\n\t"
		"mrc    p15, 0, r1, c6, c0,2\n\t"	/* IFAR */
		"str r1, [%0,#32]\n\t"
		/*Dont populate DAFSR and RAFSR */
		"mrc    p15, 0, r1, c10, c2,0\n\t"	/* PMRRR */
		"str r1, [%0,#44]\n\t"
		"mrc    p15, 0, r1, c10, c2,1\n\t"	/* NMRRR */
		"str r1, [%0,#48]\n\t"
		"mrc    p15, 0, r1, c13, c0,0\n\t"	/* FCSEPID */
		"str r1, [%0,#52]\n\t"
		"mrc    p15, 0, r1, c13, c0,1\n\t"	/* CONTEXT */
		"str r1, [%0,#56]\n\t"
		"mrc    p15, 0, r1, c13, c0,2\n\t"	/* URWTPID */
		"str r1, [%0,#60]\n\t"
		"mrc    p15, 0, r1, c13, c0,3\n\t"	/* UROTPID */
		"str r1, [%0,#64]\n\t"
		"mrc    p15, 0, r1, c13, c0,4\n\t"	/* POTPIDR */
		"str r1, [%0,#68]\n\t"
		:			/* output */
	    : "r"(mmu_reg)			/* input */
	: "%r1", "memory"	/* clobbered register */
	);
	return;
}


void sec_save_final_context(void)
{
	samsung_vendor1_id *smem_vendor1 = NULL;
	unsigned size;
	unsigned long flags;

	if (save_final_context) {
		printk(KERN_EMERG"(sec_save_final_context) already saved.\n");
		return;
	}

	local_irq_save(flags);
        sec_debug_save_mmu_reg(&(per_cpu(sec_debug_mmu_reg, smp_processor_id() )));
	printk(KERN_EMERG"(sec_save_final_context) sec_get_mmu_reg_dump \n");

	sec_debug_save_core_reg(&(per_cpu(sec_debug_core_reg, smp_processor_id() ) ));
	printk(KERN_EMERG "(%s) sec_save_final_context (CPU:%d)\n", __func__,
		smp_processor_id());

	smem_vendor1 = (samsung_vendor1_id *)smem_get_entry(SMEM_ID_VENDOR1,\
				&size);
	memcpy(&(smem_vendor1->apps_dump.apps_regs),
		&(per_cpu(sec_debug_mmu_reg, smp_processor_id() )), sizeof(sec_debug_mmu_reg));
	local_irq_restore(flags);
	flush_cache_all();	

	save_final_context = 1;
}
EXPORT_SYMBOL(sec_save_final_context);

void  sec_debug_print_build_info(void)
{
 	pr_emerg("%s\n",sec_build_info);
}

static void sec_debug_set_build_info(void)
{
	char *p = sec_build_info;
	strncat(p, "Kernel Build Info : ", 20);
	strncat(p, "Date:", 5);
	strncat(p, sec_build_time[0], 12);
	strncat(p, "Time:", 5);
	strncat(p, sec_build_time[1], 9);
}

#ifdef CONFIG_SEC_DEBUG_SUBSYS
int sec_debug_save_die_info(const char *str, struct pt_regs *regs)
{
	if (!secdbg_scorpion)
		return -ENOMEM;
	snprintf(secdbg_scorpion->excp.pc_sym,
			sizeof(secdbg_scorpion->excp.pc_sym),
			"%pS", (void *)regs->ARM_pc);
	snprintf(secdbg_scorpion->excp.lr_sym,
			sizeof(secdbg_scorpion->excp.lr_sym),
			"%pS", (void *)regs->ARM_lr);

	return 0;
}

int sec_debug_save_panic_info(const char *str, unsigned int caller)
{
	if (!secdbg_scorpion)
		return -ENOMEM;
	snprintf(secdbg_scorpion->excp.panic_caller,
		sizeof(secdbg_scorpion->excp.panic_caller), "%pS",
							(void *)caller);
	snprintf(secdbg_scorpion->excp.panic_msg,
		sizeof(secdbg_scorpion->excp.panic_msg), "%s", str);
	snprintf(secdbg_scorpion->excp.thread,
		sizeof(secdbg_scorpion->excp.thread), "%s:%d", current->comm,
		task_pid_nr(current));

	return 0;
}

int sec_debug_subsys_add_var_mon(char *name, unsigned int size, unsigned int pa)
{
	if (!secdbg_scorpion)
		return -ENOMEM;

	if (secdbg_scorpion->var_mon.idx >
			ARRAY_SIZE(secdbg_scorpion->var_mon.var))
		return -ENOMEM;

	strlcpy(secdbg_scorpion->var_mon.var[secdbg_scorpion->var_mon.idx].name,
		name, sizeof(secdbg_scorpion->var_mon.var[0].name));
	secdbg_scorpion->var_mon.var[secdbg_scorpion->var_mon.idx].sizeof_type
									= size;
	secdbg_scorpion->var_mon.var[secdbg_scorpion->var_mon.idx].var_paddr
									= pa;

	secdbg_scorpion->var_mon.idx++;

	return 0;
}

int sec_debug_subsys_init(void)
{
	struct sec_debug_subsys *vaddr;
	struct sec_debug_subsys *paddr;

	secdbg_subsys = NULL;
	secdbg_scorpion = NULL;

	if (sec_subsys_paddr == 0 || sec_subsys_size == 0) {
		pr_info("%s: sec subsys buffer not provided!\n", __func__);
		return -ENOMEM;
	} else {
		vaddr = ioremap_nocache(sec_subsys_paddr, sec_subsys_size);
		paddr = (struct sec_debug_subsys *) sec_subsys_paddr;
	}

	pr_info("%s: vaddr=0x%x paddr=0x%x size=0x%x "
		"sizeof(struct sec_debug_subsys)=0x%x\n", __func__,
		(unsigned int)vaddr, sec_subsys_paddr, sec_subsys_size,
		sizeof(struct sec_debug_subsys));

	if ((vaddr == NULL) ||
		(sizeof(struct sec_debug_subsys) > sec_subsys_size)) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -EFAULT;
	}

	secdbg_subsys = vaddr;

	/*clear the priv data*/
	memset(&secdbg_subsys->priv, 0, sizeof(secdbg_subsys->priv));

	secdbg_scorpion = &secdbg_subsys->priv.scorpion;

	/*just storing the calculated physical address*/
	/*no memory access using physical address*/
	secdbg_subsys->scorpion = &paddr->priv.scorpion;
	secdbg_subsys->modem = &paddr->priv.modem;

	pr_info("%s: scorpion(%x) modem(%x)\n", __func__,
		(unsigned int)secdbg_subsys->scorpion,
		(unsigned int)secdbg_subsys->modem);

	strncpy(secdbg_scorpion->name, "Scorpion",
		sizeof(secdbg_scorpion->name));
	strncpy(secdbg_scorpion->state, "Init",
		sizeof(secdbg_scorpion->state));
	secdbg_scorpion->nr_cpus = CONFIG_NR_CPUS;

	sec_debug_subsys_set_kloginfo(&secdbg_scorpion->log.idx_paddr,
		&secdbg_scorpion->log.log_paddr, &secdbg_scorpion->log.size);

	secdbg_scorpion->tz_core_dump =
		(struct tzbsp_dump_buf_s **)NULL;
	get_fbinfo(0, &secdbg_scorpion->fb_info.fb_paddr,
		&secdbg_scorpion->fb_info.xres,
		&secdbg_scorpion->fb_info.yres,
		&secdbg_scorpion->fb_info.bpp,
		&secdbg_scorpion->fb_info.rgb_bitinfo.r_off,
		&secdbg_scorpion->fb_info.rgb_bitinfo.r_len,
		&secdbg_scorpion->fb_info.rgb_bitinfo.g_off,
		&secdbg_scorpion->fb_info.rgb_bitinfo.g_len,
		&secdbg_scorpion->fb_info.rgb_bitinfo.b_off,
		&secdbg_scorpion->fb_info.rgb_bitinfo.b_len,
		&secdbg_scorpion->fb_info.rgb_bitinfo.a_off,
		&secdbg_scorpion->fb_info.rgb_bitinfo.a_len);

	SEC_DEBUG_SUBSYS_ADD_STR_TO_MONITOR(linux_banner);

	if (secdbg_paddr) {
		secdbg_scorpion->sched_log.sched_idx_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, idx_sched);
		secdbg_scorpion->sched_log.sched_buf_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, sched);
		secdbg_scorpion->sched_log.sched_struct_sz =
			sizeof(struct sched_log);
		secdbg_scorpion->sched_log.sched_array_cnt = SCHED_LOG_MAX;

		secdbg_scorpion->sched_log.irq_idx_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, idx_irq);
		secdbg_scorpion->sched_log.irq_buf_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, irq);
		secdbg_scorpion->sched_log.irq_struct_sz =
			sizeof(struct irq_log);
		secdbg_scorpion->sched_log.irq_array_cnt = SCHED_LOG_MAX;

		secdbg_scorpion->sched_log.irq_exit_idx_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, idx_irq_exit);
		secdbg_scorpion->sched_log.irq_exit_buf_paddr = secdbg_paddr +
			offsetof(struct sec_debug_log, irq_exit);
		secdbg_scorpion->sched_log.irq_exit_struct_sz =
			sizeof(struct irq_exit_log);
		secdbg_scorpion->sched_log.irq_exit_array_cnt = SCHED_LOG_MAX;
	}

	/* fill magic nubmer last to ensure data integrity when the magic
	 * numbers are written
	 */
	secdbg_subsys->magic[0] = SEC_DEBUG_SUBSYS_MAGIC0;
	secdbg_subsys->magic[1] = SEC_DEBUG_SUBSYS_MAGIC1;
	secdbg_subsys->magic[2] = SEC_DEBUG_SUBSYS_MAGIC2;
	secdbg_subsys->magic[3] = SEC_DEBUG_SUBSYS_MAGIC3;
	return 0;
}
late_initcall(sec_debug_subsys_init);

static int __init sec_subsys_setup(char *str)
{
	unsigned size = memparse(str, &str);

	pr_emerg("%s: str=%s\n", __func__, str);

	if (size && (*str == '@')) {
		sec_subsys_paddr = (unsigned int)memparse(++str, NULL);
		sec_subsys_size = size;
	}

	pr_emerg("%s: sec_subsys_paddr = 0x%x\n", __func__, sec_subsys_paddr);
	pr_emerg("%s: sec_subsys_size = 0x%x\n", __func__, sec_subsys_size);

	return 1;
}

__setup("sec_subsys=", sec_subsys_setup);

#endif


__init int sec_debug_init(void)
{
#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
	int i;
	struct sec_debug_log *vaddr;
	int size;

	pr_emerg("(%s)\n", __func__);

	if (secdbg_paddr == 0 || secdbg_size == 0) {
		pr_info("%s: sec debug buffer not provided. Using kmalloc..\n",
			__func__);
		size = sizeof(struct sec_debug_log);
		vaddr = kmalloc(size, GFP_KERNEL);
	} else {
		size = secdbg_size;
		vaddr = ioremap_nocache(secdbg_paddr, secdbg_size);
	}

	pr_info("%s: vaddr=0x%x paddr=0x%x size=0x%x "
		"sizeof(struct sec_debug_log)=0x%x\n", __func__,
		(unsigned int)vaddr, secdbg_paddr, secdbg_size,
		sizeof(struct sec_debug_log));

	if ((vaddr == NULL) || (sizeof(struct sec_debug_log) > size)) {
		pr_info("%s: ERROR! init failed!\n", __func__);
		return -EFAULT;
	}

	for (i = 0; i < CONFIG_NR_CPUS; i++) {
		atomic_set(&(vaddr->idx_sched[i]), -1);
		atomic_set(&(vaddr->idx_irq[i]), -1);
		atomic_set(&(vaddr->idx_irq_exit[i]), -1);
		atomic_set(&(vaddr->idx_timer[i]), -1);
#ifdef CONFIG_SEC_DEBUG_MSG_LOG
		atomic_set(&(vaddr->idx_secmsg[i]), -1);
#endif
	}
#ifdef CONFIG_SEC_DEBUG_DCVS_LOG
		atomic_set(&(vaddr->dcvs_log_idx), -1);
#endif
#ifdef CONFIG_SEC_DEBUG_FUELGAUGE_LOG
		atomic_set(&(vaddr->fg_log_idx), -1);
#endif

#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
	debug_semaphore_init();
#endif
	secdbg_log = vaddr;

	pr_info("%s: init done\n", __func__);

#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */
        sec_debug_set_build_info();

	return 0;
}

static int __init sec_dbg_setup(char *str)
{
	unsigned size = memparse(str, &str);

	pr_emerg("%s: str=%s\n", __func__, str);

	if (size && (size == roundup_pow_of_two(size)) && (*str == '@')) {
		secdbg_paddr = (unsigned int)memparse(++str, NULL);
		secdbg_size = size;
		}

	pr_emerg("%s: secdbg_paddr = 0x%x\n", __func__, secdbg_paddr);
	pr_emerg("%s: secdbg_size = 0x%x\n", __func__, secdbg_size);

	return 1;
}
__setup("sec_dbg=", sec_dbg_setup);

#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
void __sec_debug_task_sched_log(int cpu, struct task_struct *task,
						char *msg)
{
	unsigned i;

	if (!secdbg_log)
		return;

	if (!task && !msg)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_sched[cpu]))
		& (SCHED_LOG_MAX - 1);
	secdbg_log->sched[cpu][i].time = cpu_clock(cpu);
	if (task) {
		strncpy(secdbg_log->sched[cpu][i].comm, task->comm,
			sizeof(secdbg_log->sched[cpu][i].comm));
		secdbg_log->sched[cpu][i].pid = task->pid;
	} else {
		strncpy(secdbg_log->sched[cpu][i].comm, msg,
			sizeof(secdbg_log->sched[cpu][i].comm));
		secdbg_log->sched[cpu][i].pid = -1;
	}
}

void sec_debug_task_sched_log_short_msg(char *msg)
{
	int cpu;
	preempt_disable();
	cpu=smp_processor_id();
	preempt_enable();	
	__sec_debug_task_sched_log(cpu, NULL, msg);
}

void sec_debug_task_sched_log(int cpu, struct task_struct *task)
{
	__sec_debug_task_sched_log(cpu, task, NULL);
}

void sec_debug_timer_log(unsigned int type, int int_lock, void *fn)
{
	int cpu = smp_processor_id();
	unsigned i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_timer[cpu]))
			& (SCHED_LOG_MAX - 1);
	secdbg_log->timer_log[cpu][i].time = cpu_clock(cpu);
	secdbg_log->timer_log[cpu][i].type = type;
	secdbg_log->timer_log[cpu][i].int_lock = int_lock;
	secdbg_log->timer_log[cpu][i].fn = (void *)fn;
}

void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en)
{
	int cpu = smp_processor_id();
	unsigned i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_irq[cpu]))
			& (SCHED_LOG_MAX - 1);
	secdbg_log->irq[cpu][i].time = cpu_clock(cpu);
	secdbg_log->irq[cpu][i].irq = irq;
	secdbg_log->irq[cpu][i].fn = (void *)fn;
	secdbg_log->irq[cpu][i].en = en;
	secdbg_log->irq[cpu][i].preempt_count = preempt_count();
	secdbg_log->irq[cpu][i].context = &cpu;
}

#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
void sec_debug_irq_enterexit_log(unsigned int irq,
						unsigned long long start_time)
{
	int cpu = smp_processor_id();
	unsigned i;

	if (!secdbg_log)
		return;

	i = atomic_inc_return(&(secdbg_log->idx_irq_exit[cpu]))
			& (SCHED_LOG_MAX - 1);
	secdbg_log->irq_exit[cpu][i].time = start_time;
	secdbg_log->irq_exit[cpu][i].end_time = cpu_clock(cpu);
	secdbg_log->irq_exit[cpu][i].irq = irq;
	secdbg_log->irq_exit[cpu][i].elapsed_time =
		secdbg_log->irq_exit[cpu][i].end_time - start_time;
	}
#endif

#ifdef CONFIG_SEC_DEBUG_MSG_LOG
asmlinkage int sec_debug_msg_log(void *caller, const char *fmt, ...)
{
	int cpu = smp_processor_id();
	int r = 0;
	int i;
	va_list args;

	if (!secdbg_log)
		return 0;

	i = atomic_inc_return(&(secdbg_log->idx_secmsg[cpu]))
		& (MSG_LOG_MAX - 1);
	secdbg_log->secmsg[cpu][i].time = cpu_clock(cpu);
	va_start(args, fmt);
	r = vsnprintf(secdbg_log->secmsg[cpu][i].msg,
		sizeof(secdbg_log->secmsg[cpu][i].msg), fmt, args);
	va_end(args);

	secdbg_log->secmsg[cpu][i].caller0 = __builtin_return_address(0);
	secdbg_log->secmsg[cpu][i].caller1 = caller;
	secdbg_log->secmsg[cpu][i].task = current->comm;

	return r;
}
#endif
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */

/* klaatu - semaphore log */
#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
void debug_semaphore_init(void)
{
	int i = 0;
	struct sem_debug *sem_debug = NULL;

	spin_lock_init(&sem_debug_lock);
	sem_debug_free_head_cnt = 0;
	sem_debug_done_head_cnt = 0;

	/* initialize list head of sem_debug */
	INIT_LIST_HEAD(&sem_debug_free_head.list);
	INIT_LIST_HEAD(&sem_debug_done_head.list);

	for (i = 0; i < SEMAPHORE_LOG_MAX; i++) {
		/* malloc semaphore */
		sem_debug = kmalloc(sizeof(struct sem_debug), GFP_KERNEL);

		/* add list */
		list_add(&sem_debug->list, &sem_debug_free_head.list);
		sem_debug_free_head_cnt++;
	}

	sem_debug_init = 1;
}

void debug_semaphore_down_log(struct semaphore *sem)
{
	struct list_head *tmp;
	struct sem_debug *sem_dbg;
	unsigned long flags;

	if (!sem_debug_init)
		return;

	spin_lock_irqsave(&sem_debug_lock, flags);
	list_for_each(tmp, &sem_debug_free_head.list) {
		sem_dbg = list_entry(tmp, struct sem_debug, list);
		sem_dbg->task = current;
		sem_dbg->sem = sem;
		sem_dbg->pid = current->pid;
		sem_dbg->cpu = smp_processor_id();
		list_del(&sem_dbg->list);
		list_add(&sem_dbg->list, &sem_debug_done_head.list);
		sem_debug_free_head_cnt--;
		sem_debug_done_head_cnt++;
		break;
	}
	spin_unlock_irqrestore(&sem_debug_lock, flags);
}

void debug_semaphore_up_log(struct semaphore *sem)
{
	struct list_head *tmp;
	struct sem_debug *sem_dbg;
	unsigned long flags;

	if (!sem_debug_init)
		return;

	spin_lock_irqsave(&sem_debug_lock, flags);
	list_for_each(tmp, &sem_debug_done_head.list) {
		sem_dbg = list_entry(tmp, struct sem_debug, list);
		if (sem_dbg->sem == sem && sem_dbg->pid == current->pid) {
			list_del(&sem_dbg->list);
			list_add(&sem_dbg->list, &sem_debug_free_head.list);
			sem_debug_free_head_cnt++;
			sem_debug_done_head_cnt--;
			break;
		}
	}
	spin_unlock_irqrestore(&sem_debug_lock, flags);
}

/* rwsemaphore logging */
void debug_rwsemaphore_init(void)
{
	int i = 0;
	struct rwsem_debug *rwsem_debug = NULL;
	spin_lock_init(&rwsem_debug_lock);
	rwsem_debug_free_head_cnt = 0;
	rwsem_debug_done_head_cnt = 0;

	/* initialize list head of sem_debug */
	INIT_LIST_HEAD(&rwsem_debug_free_head.list);
	INIT_LIST_HEAD(&rwsem_debug_done_head.list);

	for (i = 0; i < RWSEMAPHORE_LOG_MAX; i++) {
		/* malloc semaphore */
		rwsem_debug =
			kmalloc(sizeof(struct rwsem_debug), GFP_KERNEL);
		/* add list */
		list_add(&rwsem_debug->list, &rwsem_debug_free_head.list);
		rwsem_debug_free_head_cnt++;
	}

	rwsem_debug_init = 1;
}

void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_free_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		sem_dbg->task = current;
		sem_dbg->sem = sem;
		sem_dbg->pid = current->pid;
		sem_dbg->cpu = smp_processor_id();
		sem_dbg->direction = dir;
		list_del(&sem_dbg->list);
		list_add(&sem_dbg->list, &rwsem_debug_done_head.list);
		rwsem_debug_free_head_cnt--;
		rwsem_debug_done_head_cnt++;
		break;
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}

void debug_rwsemaphore_up_log(struct rw_semaphore *sem)
{
	struct list_head *tmp;
	struct rwsem_debug *sem_dbg;
	unsigned long flags;

	if (!rwsem_debug_init)
		return;

	spin_lock_irqsave(&rwsem_debug_lock, flags);
	list_for_each(tmp, &rwsem_debug_done_head.list) {
		sem_dbg = list_entry(tmp, struct rwsem_debug, list);
		if (sem_dbg->sem == sem && sem_dbg->pid == current->pid) {
			list_del(&sem_dbg->list);
			list_add(&sem_dbg->list, &rwsem_debug_free_head.list);
			rwsem_debug_free_head_cnt++;
			rwsem_debug_done_head_cnt--;
			break;
		}
	}
	spin_unlock_irqrestore(&rwsem_debug_lock, flags);
}
#endif	/* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

#ifdef CONFIG_SEC_DEBUG_USER
extern int dump_enable_flag;
void sec_user_fault_dump(void)
{
#if 0
	if (sec_debug_level.en.kernel_fault == 1
	    && sec_debug_level.en.user_fault == 1)
#else
	if ( dump_enable_flag == 2 ) // debug level HIGH
#endif
		panic("User Fault");
}

static int sec_user_fault_write(struct file *file, const char __user * buffer,
				size_t count, loff_t * offs)
{
	char buf[100];

	if (count > sizeof(buf) - 1)
		return -EINVAL;
	if (copy_from_user(buf, buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if (strncmp(buf, "dump_user_fault", 15) == 0)
		sec_user_fault_dump();

	return count;
}

static const struct file_operations sec_user_fault_proc_fops = {
	.write = sec_user_fault_write,
};

static int __init sec_debug_user_fault_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("user_fault", S_IWUSR | S_IWGRP, NULL,
			    &sec_user_fault_proc_fops);
	if (!entry)
		return -ENOMEM;
	return 0;
}

device_initcall(sec_debug_user_fault_init);
#endif

#ifdef CONFIG_APPLY_GA_SOLUTION
extern void dump_all_task_info(void);
extern void dump_cpu_stat(void);
#endif

void sec_check_crash_key(int keycode, u8 keypress)
{
	static enum {NONE, STEP1, STEP2, STEP3, STEP4, STEP5, STEP6,\
			STEP7, STEP8} state = NONE;

	switch (state) {
	case NONE:
		if ((keycode == LOCKUP_FIRST_KEY) && keypress)
			state = STEP1;
		else
			state = NONE;
		break;
	case STEP1:
		if ((keycode == LOCKUP_FIRST_KEY) && !keypress)
			state = STEP2;
		else
			state = NONE;
		break;
	case STEP2:
		if ((keycode == LOCKUP_SECOND_KEY) && keypress)
			state = STEP3;
		else
			state = NONE;
		break;
	case STEP3:
		if ((keycode == LOCKUP_SECOND_KEY) && !keypress)
			state = STEP4;
		else
			state = NONE;
		break;
	case STEP4:
		if ((keycode == LOCKUP_THIRD_KEY) && keypress)
			state = STEP5;
		else
			state = NONE;
		break;
	case STEP5:
		if ((keycode == LOCKUP_THIRD_KEY) && !keypress)
			state = STEP6;
		else
			state = NONE;
		break;
	case STEP6:
		if ((keycode == LOCKUP_FIRST_KEY) && keypress)
			state = STEP7;
		else
			state = NONE;
		break;
	case STEP7:
		if ((keycode == LOCKUP_FIRST_KEY) && !keypress)
			state = STEP8;
		else
			state = NONE;
		break;
	case STEP8:
		if ((keycode == LOCKUP_THIRD_KEY) && keypress)
		{
#ifdef CONFIG_APPLY_GA_SOLUTION
			printk("Force Key DUMP\n");
		//	dump_all_task_info();
		//	dump_cpu_stat();
#endif
			panic("[Crash Key] LOCKUP CAPTURED!!!");
		}
		else
			state = NONE;
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(sec_check_crash_key);

static int force_error(const char *val, struct kernel_param *kp)
{
	pr_emerg("!!!WARN forced error : %s\n", val);

	if (!strncmp(val, "wdog", 4)) {
		pr_emerg("Generating a wdog bark!\n");
		raw_local_irq_disable();
	while (1)
		;
	} else if (!strncmp(val, "dabort", 6)) {
		pr_emerg("Generating a data abort exception!\n");
		*(unsigned int *)0x0 = 0x0;
	} else if (!strncmp(val, "pabort", 6)) {
		pr_emerg("Generating a prefetch abort exception!\n");
		((void (*)(void))0x0)();
	} else if (!strncmp(val, "undef", 5)) {
		pr_emerg("Generating a undefined instruction exception!\n");
		BUG();
	} else if (!strncmp(val, "bushang", 7)) {
		void __iomem *p;
		pr_emerg("Generating Bus Hang!\n");
		p = ioremap_nocache(0x04300000, 32);
		*(unsigned int *)p = *(unsigned int *)p;
		mb();
		pr_info("*p = %x\n", *(unsigned int *)p);
		pr_emerg("Clk may be enabled.Try again if it reaches here!\n");
	} else if (!strncmp(val, "dblfree", 7)) {
		void *p = kmalloc(sizeof(int), GFP_KERNEL);
		kfree(p);
		msleep(1000);
		kfree(p);
	} else if (!strncmp(val, "danglingref", 11)) {
		unsigned int *p = kmalloc(sizeof(int), GFP_KERNEL);
		kfree(p);
		*p = 0x1234;
	} else if (!strncmp(val, "lowmem", 6)) {
		int i = 0;
		pr_emerg("Allocating memory until failure!\n");
		while (kmalloc(128*1024, GFP_KERNEL))
			i++;
		pr_emerg("Allocated %d KB!\n", i*128);
	} else if (!strncmp(val, "memcorrupt", 10)) {
		int *ptr = kmalloc(sizeof(int), GFP_KERNEL);
		*ptr++ = 4;
		*ptr = 2;
		panic("MEMORY CORRUPTION");
#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
	} else if (!strncmp(val, "dfdenable", 9)) {
		dfd_enable();
	} else if (!strncmp(val, "dfddisable", 10)) {
		dfd_disable();
#endif
	} else {
		pr_emerg("No such error defined for now!\n");
		}

	return 0;
	}


#endif /* CONFIG_SEC_DEBUG */
