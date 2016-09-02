#ifndef _SEC_DEBUG_H_
#define _SEC_DEBUG_H_

#include <linux/io.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/semaphore.h>


#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
extern void sec_debug_task_sched_log_short_msg(char *msg);
extern void sec_debug_task_sched_log(int cpu, struct task_struct *task);
extern void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en);
extern void sec_debug_irq_sched_log_end(void);
extern void sec_debug_timer_log(unsigned int type, int int_lock, void *fn);
extern void sec_debug_sched_log_init(void);
#define secdbg_sched_msg(fmt, ...) \
	do { \
		char ___buf[16]; \
		snprintf(___buf, sizeof(___buf), fmt, ##__VA_ARGS__); \
		sec_debug_task_sched_log_short_msg(___buf); \
	} while (0)
#else
static inline void sec_debug_task_sched_log(int cpu, struct task_struct *task)
{
}
static inline void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en)
{
}
static inline void sec_debug_irq_sched_log_end(void)
{
}
static inline void sec_debug_timer_log(unsigned int type,
						int int_lock, void *fn)
{
}
static inline void sec_debug_sched_log_init(void)
{
}
#define secdbg_sched_msg(fmt, ...)
#endif

#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
extern void *kfree_hook(void *p, void *caller);
#endif

#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
extern void sec_debug_irq_enterexit_log(unsigned int irq,
						unsigned long long start_time);
#else
static inline void sec_debug_irq_enterexit_log(unsigned int irq,
						unsigned long long start_time)
{
}
#endif

#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
extern void debug_semaphore_init(void);
extern void debug_semaphore_down_log(struct semaphore *sem);
extern void debug_semaphore_up_log(struct semaphore *sem);
extern void debug_rwsemaphore_init(void);
extern void debug_rwsemaphore_down_log(struct rw_semaphore *sem, int dir);
extern void debug_rwsemaphore_up_log(struct rw_semaphore *sem);
#define debug_rwsemaphore_down_read_log(x) \
	debug_rwsemaphore_down_log(x, READ_SEM)
#define debug_rwsemaphore_down_write_log(x) \
	debug_rwsemaphore_down_log(x, WRITE_SEM)
#else
static inline void debug_semaphore_init(void)
{
}
static inline void debug_semaphore_down_log(struct semaphore *sem)
{
}
static inline void debug_semaphore_up_log(struct semaphore *sem)
{
}
static inline void debug_rwsemaphore_init(void)
{
}
static inline void debug_rwsemaphore_down_read_log(struct rw_semaphore *sem)
{
}
static inline void debug_rwsemaphore_down_write_log(struct rw_semaphore *sem)
{
}
static inline void debug_rwsemaphore_up_log(struct rw_semaphore *sem)
{
}
#endif



#ifdef CONFIG_SEC_DEBUG_SCHED_LOG
#define SCHED_LOG_MAX 1024

struct irq_log {
	unsigned long long time;
	int irq;
	void *fn;
	int en;
	int preempt_count;
	void *context;
};

struct irq_exit_log {
	unsigned int irq;
	unsigned long long time;
	unsigned long long end_time;
	unsigned long long elapsed_time;
};

struct sched_log {
	unsigned long long time;
	char comm[TASK_COMM_LEN];
	pid_t pid;
};


struct timer_log {
	unsigned long long time;
	unsigned int type;
	int int_lock;
	void *fn;
};

extern void sec_debug_task_sched_log_short_msg(char *msg);
extern void sec_debug_task_sched_log(int cpu, struct task_struct *task);
extern void sec_debug_irq_sched_log(unsigned int irq, void *fn, int en);
extern void sec_debug_irq_sched_log_end(void);
extern void sec_debug_timer_log(unsigned int type, int int_lock, void *fn);
extern void sec_debug_print_build_info(void);
#define secdbg_sched_msg(fmt, ...) \
	do { \
		char ___buf[16]; \
		snprintf(___buf, sizeof(___buf), fmt, ##__VA_ARGS__); \
		sec_debug_task_sched_log_short_msg(___buf); \
	} while (0)
#endif /* CONFIG_SEC_DEBUG_SCHED_LOG */
	
#ifdef CONFIG_SEC_DEBUG_IRQ_EXIT_LOG
extern void sec_debug_irq_enterexit_log(unsigned int irq,\
					unsigned long long start_time);
#endif /* CONFIG_SEC_DEBUG_IRQ_EXIT_LOG */


#ifdef CONFIG_SEC_DEBUG_SEMAPHORE_LOG
#define SEMAPHORE_LOG_MAX 100
struct sem_debug {
	struct list_head list;
	struct semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	/* char comm[TASK_COMM_LEN]; */
};

enum {
	READ_SEM,
	WRITE_SEM
};

#define RWSEMAPHORE_LOG_MAX 100
struct rwsem_debug {
	struct list_head list;
	struct rw_semaphore *sem;
	struct task_struct *task;
	pid_t pid;
	int cpu;
	int direction;
	/* char comm[TASK_COMM_LEN]; */
};

#endif	/* CONFIG_SEC_DEBUG_SEMAPHORE_LOG */

extern void sec_save_final_context(void);
extern int sec_debug_init(void);
extern void sec_check_crash_key(int keycode, u8 keypress);
#if defined(CONFIG_SEC_DEBUG) && defined(CONFIG_SEC_DUMP)
	extern int dump_enable_flag;
#endif

#ifdef CONFIG_SEC_DEBUG_DOUBLE_FREE
extern void *kfree_hook(void *p, void *caller);
#endif

#ifdef CONFIG_SEC_DEBUG_SUBSYS

extern void sec_debug_subsys_fill_fbinfo(int idx, void *fb, u32 xres,
				u32 yres, u32 bpp, u32 color_mode);

#define SEC_DEBUG_SUBSYS_MAGIC0 0xFFFFFFFF
#define SEC_DEBUG_SUBSYS_MAGIC1 0x5ECDEB6
#define SEC_DEBUG_SUBSYS_MAGIC2 0x14F014F0
 /* high word : major version
  * low word : minor version
  * minor version changes should not affect LK behavior
  */
#define SEC_DEBUG_SUBSYS_MAGIC3 0x00010002


#define TZBSP_CPU_COUNT           2
/* CPU context for the monitor. */
struct tzbsp_dump_cpu_ctx_s {
	unsigned int mon_lr;
	unsigned int mon_spsr;
	unsigned int usr_r0;
	unsigned int usr_r1;
	unsigned int usr_r2;
	unsigned int usr_r3;
	unsigned int usr_r4;
	unsigned int usr_r5;
	unsigned int usr_r6;
	unsigned int usr_r7;
	unsigned int usr_r8;
	unsigned int usr_r9;
	unsigned int usr_r10;
	unsigned int usr_r11;
	unsigned int usr_r12;
	unsigned int usr_r13;
	unsigned int usr_r14;
	unsigned int irq_spsr;
	unsigned int irq_r13;
	unsigned int irq_r14;
	unsigned int svc_spsr;
	unsigned int svc_r13;
	unsigned int svc_r14;
	unsigned int abt_spsr;
	unsigned int abt_r13;
	unsigned int abt_r14;
	unsigned int und_spsr;
	unsigned int und_r13;
	unsigned int und_r14;
	unsigned int fiq_spsr;
	unsigned int fiq_r8;
	unsigned int fiq_r9;
	unsigned int fiq_r10;
	unsigned int fiq_r11;
	unsigned int fiq_r12;
	unsigned int fiq_r13;
	unsigned int fiq_r14;
};

struct tzbsp_dump_buf_s {
	unsigned int sc_status[TZBSP_CPU_COUNT];
	struct tzbsp_dump_cpu_ctx_s sc_ns[TZBSP_CPU_COUNT];
	struct tzbsp_dump_cpu_ctx_s sec;
};

struct core_reg_info {
	char name[12];
	unsigned int value;
};

struct sec_debug_subsys_excp {
	char type[16];
	char task[16];
	char file[32];
	int line;
	char msg[256];
	struct core_reg_info core_reg[64];
};

struct sec_debug_subsys_excp_scorpion {
	char pc_sym[64];
	char lr_sym[64];
	char panic_caller[64];
	char panic_msg[128];
	char thread[32];
};

struct sec_debug_subsys_log {
	unsigned int idx_paddr;
	unsigned int log_paddr;
	unsigned int size;
};

struct rgb_bit_info {
	unsigned char r_off;
	unsigned char r_len;
	unsigned char g_off;
	unsigned char g_len;
	unsigned char b_off;
	unsigned char b_len;
	unsigned char a_off;
	unsigned char a_len;
};

struct var_info {
	char name[16];
	int sizeof_type;
	unsigned int var_paddr;
};
struct sec_debug_subsys_simple_var_mon {
	int idx;
	struct var_info var[32];
};

struct sec_debug_subsys_fb {
	unsigned int fb_paddr;
	int xres;
	int yres;
	int bpp;
	struct rgb_bit_info rgb_bitinfo;
};

struct sec_debug_subsys_sched_log {
	unsigned int sched_idx_paddr;
	unsigned int sched_buf_paddr;
	unsigned int sched_struct_sz;
	unsigned int sched_array_cnt;
	unsigned int irq_idx_paddr;
	unsigned int irq_buf_paddr;
	unsigned int irq_struct_sz;
	unsigned int irq_array_cnt;
	unsigned int irq_exit_idx_paddr;
	unsigned int irq_exit_buf_paddr;
	unsigned int irq_exit_struct_sz;
	unsigned int irq_exit_array_cnt;
};

struct sec_debug_subsys_data {
	char name[16];
	char state[16];
	struct sec_debug_subsys_log log;
	struct sec_debug_subsys_excp excp;
	struct sec_debug_subsys_simple_var_mon var_mon;
};

struct sec_debug_subsys_data_modem {
	char name[16];
	char state[16];
	struct sec_debug_subsys_log log;
	struct sec_debug_subsys_excp excp;
	struct sec_debug_subsys_simple_var_mon var_mon;
};

struct sec_debug_subsys_data_scorpion {
	char name[16];
	char state[16];
	int nr_cpus;
	struct sec_debug_subsys_log log;
	struct sec_debug_subsys_excp_scorpion excp;
	struct sec_debug_subsys_simple_var_mon var_mon;
	struct tzbsp_dump_buf_s **tz_core_dump;
	struct sec_debug_subsys_fb fb_info;
	struct sec_debug_subsys_sched_log sched_log;
};

struct sec_debug_subsys_private {
	struct sec_debug_subsys_data_scorpion scorpion;
	struct sec_debug_subsys_data_modem modem;
};

struct sec_debug_subsys {
	unsigned int magic[4];
	struct sec_debug_subsys_data_scorpion *scorpion;
	struct sec_debug_subsys_data_modem *modem;

	struct sec_debug_subsys_private priv;
};

extern int sec_debug_subsys_add_var_mon(char *name, unsigned int size,
	unsigned int addr);
#define SEC_DEBUG_SUBSYS_ADD_VAR_TO_MONITOR(var) \
	sec_debug_subsys_add_var_mon(#var, sizeof(var), \
		(unsigned int)__pa(&var))
#define SEC_DEBUG_SUBSYS_ADD_STR_TO_MONITOR(pstr) \
	sec_debug_subsys_add_var_mon(#pstr, -1, (unsigned int)__pa(pstr))

extern int get_fbinfo(int fb_num, unsigned int *fb_paddr, unsigned int *xres,
		unsigned int *yres, unsigned int *bpp,
		unsigned char *roff, unsigned char *rlen,
		unsigned char *goff, unsigned char *glen,
		unsigned char *boff, unsigned char *blen,
		unsigned char *aoff, unsigned char *alen);
extern unsigned int msm_shared_ram_phys;
extern char *get_kernel_log_buf_paddr(void);
extern char *get_fb_paddr(void);
extern unsigned int get_wdog_regsave_paddr(void);

extern unsigned int get_last_pet_paddr(void);
extern void sec_debug_subsys_set_kloginfo(unsigned int *idx_paddr,
	unsigned int *log_paddr, unsigned int *size);
int sec_debug_save_die_info(const char *str, struct pt_regs *regs);
int sec_debug_save_panic_info(const char *str, unsigned int caller);

extern uint32_t global_pvs;
#endif

#endif /* _SEC_DEBUG_H_ */
