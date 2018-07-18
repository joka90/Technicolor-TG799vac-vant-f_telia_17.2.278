/*
 *	linux/kernel/softirq.c
 *
 *	Copyright (C) 1992 Linus Torvalds
 *
 *	Distribute under GPLv2.
 *
 *	Rewritten. Old one was good in 2.2, but in 2.3 it was immoral. --ANK (990903)
 *
 *	Remote softirq infrastructure is by Jens Axboe.
 */

#include <linux/export.h>
#include <linux/kernel_stat.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/notifier.h>
#include <linux/percpu.h>
#include <linux/cpu.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/rcupdate.h>
#include <linux/delay.h>
#include <linux/ftrace.h>
#include <linux/smp.h>
#include <linux/tick.h>
#include <linux/locallock.h>

#define CREATE_TRACE_POINTS
#include <trace/events/irq.h>

#include <asm/irq.h>
/*
   - No shared variables, all the data are CPU local.
   - If a softirq needs serialization, let it serialize itself
     by its own spinlocks.
   - Even if softirq is serialized, only local cpu is marked for
     execution. Hence, we get something sort of weak cpu binding.
     Though it is still not clear, will it result in better locality
     or will not.

   Examples:
   - NET RX softirq. It is multithreaded and does not require
     any global serialization.
   - NET TX softirq. It kicks software netdevice queues, hence
     it is logically serialized per device, but this serialization
     is invisible to common code.
   - Tasklets: serialized wrt itself.
 */

#ifndef __ARCH_IRQ_STAT
irq_cpustat_t irq_stat[NR_CPUS] ____cacheline_aligned;
EXPORT_SYMBOL(irq_stat);
#endif

static struct softirq_action softirq_vec[NR_SOFTIRQS] __cacheline_aligned_in_smp;

DEFINE_PER_CPU(struct task_struct *, ksoftirqd);

char *softirq_to_name[NR_SOFTIRQS] = {
	"HI", "TIMER", "NET_TX", "NET_RX", "BLOCK", "BLOCK_IOPOLL",
	"TASKLET", "SCHED", "HRTIMER", "RCU"
};

#ifdef CONFIG_NO_HZ
# ifdef CONFIG_PREEMPT_RT_FULL
/*
 * On preempt-rt a softirq might be blocked on a lock. There might be
 * no other runnable task on this CPU because the lock owner runs on
 * some other CPU. So we have to go into idle with the pending bit
 * set. Therefor we need to check this otherwise we warn about false
 * positives which confuses users and defeats the whole purpose of
 * this test.
 *
 * This code is called with interrupts disabled.
 */
void softirq_check_pending_idle(void)
{
	static int rate_limit;
	u32 warnpending = 0, pending = local_softirq_pending();

	if (rate_limit >= 10)
		return;

	if (pending) {
		struct task_struct *tsk;

		tsk = __get_cpu_var(ksoftirqd);
		/*
		 * The wakeup code in rtmutex.c wakes up the task
		 * _before_ it sets pi_blocked_on to NULL under
		 * tsk->pi_lock. So we need to check for both: state
		 * and pi_blocked_on.
		 */
		raw_spin_lock(&tsk->pi_lock);

		if (!tsk->pi_blocked_on && !(tsk->state == TASK_RUNNING))
			warnpending = 1;

		raw_spin_unlock(&tsk->pi_lock);
	}

	if (warnpending) {
		printk(KERN_ERR "NOHZ: local_softirq_pending %02x\n",
		       pending);
		rate_limit++;
	}
}
# else
/*
 * On !PREEMPT_RT we just printk rate limited:
 */
void softirq_check_pending_idle(void)
{
	static int rate_limit;

	if (rate_limit < 10) {
		printk(KERN_ERR "NOHZ: local_softirq_pending %02x\n",
		       local_softirq_pending());
		rate_limit++;
	}
}
# endif
#endif

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static void wakeup_softirqd(void)
{
	/* Interrupts are disabled: no need to stop preemption */
	struct task_struct *tsk = __this_cpu_read(ksoftirqd);

	if (tsk && tsk->state != TASK_RUNNING)
		wake_up_process(tsk);
}

static void handle_pending_softirqs(u32 pending, int cpu, int need_rcu_bh_qs)
{
	struct softirq_action *h = softirq_vec;
	unsigned int prev_count = preempt_count();

	local_irq_enable();
	for ( ; pending; h++, pending >>= 1) {
		unsigned int vec_nr = h - softirq_vec;

		if (!(pending & 1))
			continue;

		kstat_incr_softirqs_this_cpu(vec_nr);
		trace_softirq_entry(vec_nr);
		h->action(h);
		trace_softirq_exit(vec_nr);
		if (unlikely(prev_count != preempt_count())) {
			printk(KERN_ERR
 "huh, entered softirq %u %s %p with preempt_count %08x exited with %08x?\n",
			       vec_nr, softirq_to_name[vec_nr], h->action,
			       prev_count, (unsigned int) preempt_count());
			preempt_count() = prev_count;
		}
		if (need_rcu_bh_qs)
			rcu_bh_qs(cpu);
	}
	local_irq_disable();
}

#ifndef CONFIG_PREEMPT_RT_FULL
/*
 * preempt_count and SOFTIRQ_OFFSET usage:
 * - preempt_count is changed by SOFTIRQ_OFFSET on entering or leaving
 *   softirq processing.
 * - preempt_count is changed by SOFTIRQ_DISABLE_OFFSET (= 2 * SOFTIRQ_OFFSET)
 *   on local_bh_disable or local_bh_enable.
 * This lets us distinguish between whether we are currently processing
 * softirq and whether we just have bh disabled.
 */

/*
 * This one is for softirq.c-internal use,
 * where hardirqs are disabled legitimately:
 */
#ifdef CONFIG_TRACE_IRQFLAGS
static void __local_bh_disable(unsigned long ip, unsigned int cnt)
{
	unsigned long flags;

	WARN_ON_ONCE(in_irq());

	raw_local_irq_save(flags);
	/*
	 * The preempt tracer hooks into add_preempt_count and will break
	 * lockdep because it calls back into lockdep after SOFTIRQ_OFFSET
	 * is set and before current->softirq_enabled is cleared.
	 * We must manually increment preempt_count here and manually
	 * call the trace_preempt_off later.
	 */
	preempt_count() += cnt;
	/*
	 * Were softirqs turned off above:
	 */
	if (softirq_count() == cnt)
		trace_softirqs_off(ip);
	raw_local_irq_restore(flags);

	if (preempt_count() == cnt)
		trace_preempt_off(CALLER_ADDR0, get_parent_ip(CALLER_ADDR1));
}
#else /* !CONFIG_TRACE_IRQFLAGS */
static inline void __local_bh_disable(unsigned long ip, unsigned int cnt)
{
	add_preempt_count(cnt);
	barrier();
}
#endif /* CONFIG_TRACE_IRQFLAGS */

void local_bh_disable(void)
{
	__local_bh_disable((unsigned long)__builtin_return_address(0),
				SOFTIRQ_DISABLE_OFFSET);
}

EXPORT_SYMBOL(local_bh_disable);

static void __local_bh_enable(unsigned int cnt)
{
	WARN_ON_ONCE(in_irq());
	WARN_ON_ONCE(!irqs_disabled());

	if (softirq_count() == cnt)
		trace_softirqs_on((unsigned long)__builtin_return_address(0));
	sub_preempt_count(cnt);
}

/*
 * Special-case - softirqs can safely be enabled in
 * cond_resched_softirq(), or by __do_softirq(),
 * without processing still-pending softirqs:
 */
void _local_bh_enable(void)
{
	__local_bh_enable(SOFTIRQ_DISABLE_OFFSET);
}

EXPORT_SYMBOL(_local_bh_enable);

static inline void _local_bh_enable_ip(unsigned long ip)
{
	WARN_ON_ONCE(in_irq() || irqs_disabled());
#ifdef CONFIG_TRACE_IRQFLAGS
	local_irq_disable();
#endif
	/*
	 * Are softirqs going to be turned on now:
	 */
	if (softirq_count() == SOFTIRQ_DISABLE_OFFSET)
		trace_softirqs_on(ip);
	/*
	 * Keep preemption disabled until we are done with
	 * softirq processing:
 	 */
	sub_preempt_count(SOFTIRQ_DISABLE_OFFSET - 1);

	if (unlikely(!in_interrupt() && local_softirq_pending()))
		do_softirq();

	dec_preempt_count();
#ifdef CONFIG_TRACE_IRQFLAGS
	local_irq_enable();
#endif
	preempt_check_resched();
}

void local_bh_enable(void)
{
	_local_bh_enable_ip((unsigned long)__builtin_return_address(0));
}
EXPORT_SYMBOL(local_bh_enable);

void local_bh_enable_ip(unsigned long ip)
{
	_local_bh_enable_ip(ip);
}
EXPORT_SYMBOL(local_bh_enable_ip);

/*
 * We restart softirq processing MAX_SOFTIRQ_RESTART times,
 * and we fall back to softirqd after that.
 *
 * This number has been established via experimentation.
 * The two things to balance is latency against fairness -
 * we want to handle softirqs as soon as possible, but they
 * should not be able to lock up the box.
 */
#define MAX_SOFTIRQ_RESTART 10

asmlinkage void __do_softirq(void)
{
	__u32 pending;
	int max_restart = MAX_SOFTIRQ_RESTART;
	int cpu;

	pending = local_softirq_pending();
	account_system_vtime(current);

	__local_bh_disable((unsigned long)__builtin_return_address(0),
			   SOFTIRQ_OFFSET);
	lockdep_softirq_enter();

	cpu = smp_processor_id();
restart:
	/* Reset the pending bitmask before enabling irqs */
	set_softirq_pending(0);

	handle_pending_softirqs(pending, cpu, 1);

	pending = local_softirq_pending();
	if (pending && --max_restart)
		goto restart;

	if (pending)
		wakeup_softirqd();

	lockdep_softirq_exit();

	account_system_vtime(current);
	__local_bh_enable(SOFTIRQ_OFFSET);
}

/*
 * Called with preemption disabled from run_ksoftirqd()
 */
static int ksoftirqd_do_softirq(int cpu)
{
	/*
	 * Preempt disable stops cpu going offline.
	 * If already offline, we'll be on wrong CPU:
	 * don't process.
	 */
	if (cpu_is_offline(cpu))
		return -1;

	local_irq_disable();
	if (local_softirq_pending())
		__do_softirq();
	local_irq_enable();
	return 0;
}

#ifndef __ARCH_HAS_DO_SOFTIRQ

asmlinkage void do_softirq(void)
{
	__u32 pending;
	unsigned long flags;

	if (in_interrupt())
		return;

	local_irq_save(flags);

	pending = local_softirq_pending();

	if (pending)
		__do_softirq();

	local_irq_restore(flags);
}

#endif

static inline void local_bh_disable_nort(void) { local_bh_disable(); }
static inline void _local_bh_enable_nort(void) { _local_bh_enable(); }
static inline void ksoftirqd_set_sched_params(void) { }
static inline void ksoftirqd_clr_sched_params(void) { }

#else /* !PREEMPT_RT_FULL */

/*
 * On RT we serialize softirq execution with a cpu local lock
 */
static DEFINE_LOCAL_IRQ_LOCK(local_softirq_lock);
static DEFINE_PER_CPU(struct task_struct *, local_softirq_runner);

static void __do_softirq_common(int need_rcu_bh_qs);

void __do_softirq(void)
{
	__do_softirq_common(0);
}

void __init softirq_early_init(void)
{
	local_irq_lock_init(local_softirq_lock);
}

void local_bh_disable(void)
{
	migrate_disable();
	current->softirq_nestcnt++;
}
EXPORT_SYMBOL(local_bh_disable);

void local_bh_enable(void)
{
	if (WARN_ON(current->softirq_nestcnt == 0))
		return;

	if ((current->softirq_nestcnt == 1) &&
	    local_softirq_pending() &&
	    local_trylock(local_softirq_lock)) {

		local_irq_disable();
		if (local_softirq_pending())
			__do_softirq();
		local_irq_enable();
		local_unlock(local_softirq_lock);
		WARN_ON(current->softirq_nestcnt != 1);
	}
	current->softirq_nestcnt--;
	migrate_enable();
}
EXPORT_SYMBOL(local_bh_enable);

void local_bh_enable_ip(unsigned long ip)
{
	local_bh_enable();
}
EXPORT_SYMBOL(local_bh_enable_ip);

void _local_bh_enable(void)
{
	current->softirq_nestcnt--;
	migrate_enable();
}
EXPORT_SYMBOL(_local_bh_enable);

/* For tracing */
int notrace __in_softirq(void)
{
	if (__get_cpu_var(local_softirq_lock).owner == current)
		return __get_cpu_var(local_softirq_lock).nestcnt;
	return 0;
}

int in_serving_softirq(void)
{
	int res;

	preempt_disable();
	res = __get_cpu_var(local_softirq_runner) == current;
	preempt_enable();
	return res;
}
EXPORT_SYMBOL(in_serving_softirq);

/*
 * Called with bh and local interrupts disabled. For full RT cpu must
 * be pinned.
 */
static void __do_softirq_common(int need_rcu_bh_qs)
{
	u32 pending = local_softirq_pending();
	int cpu = smp_processor_id();

	current->softirq_nestcnt++;

	/* Reset the pending bitmask before enabling irqs */
	set_softirq_pending(0);

	__get_cpu_var(local_softirq_runner) = current;

	lockdep_softirq_enter();

	handle_pending_softirqs(pending, cpu, need_rcu_bh_qs);

	pending = local_softirq_pending();
	if (pending)
		wakeup_softirqd();

	lockdep_softirq_exit();
	__get_cpu_var(local_softirq_runner) = NULL;

	current->softirq_nestcnt--;
}

static int __thread_do_softirq(int cpu)
{
	/*
	 * Prevent the current cpu from going offline.
	 * pin_current_cpu() can reenable preemption and block on the
	 * hotplug mutex. When it returns, the current cpu is
	 * pinned. It might be the wrong one, but the offline check
	 * below catches that.
	 */
	pin_current_cpu();
	/*
	 * If called from ksoftirqd (cpu >= 0) we need to check
	 * whether we are on the wrong cpu due to cpu offlining. If
	 * called via thread_do_softirq() no action required.
	 */
	if (cpu >= 0 && cpu_is_offline(cpu)) {
		unpin_current_cpu();
		return -1;
	}
	preempt_enable();
	local_lock(local_softirq_lock);
	local_irq_disable();
	/*
	 * We cannot switch stacks on RT as we want to be able to
	 * schedule!
	 */
	if (local_softirq_pending())
		__do_softirq_common(cpu >= 0);
	local_unlock(local_softirq_lock);
	unpin_current_cpu();
	preempt_disable();
	local_irq_enable();
	return 0;
}

/*
 * Called from netif_rx_ni(). Preemption enabled.
 */
void thread_do_softirq(void)
{
	if (!in_serving_softirq()) {
		preempt_disable();
		__thread_do_softirq(-1);
		preempt_enable();
	}
}

static int ksoftirqd_do_softirq(int cpu)
{
	return __thread_do_softirq(cpu);
}

static inline void local_bh_disable_nort(void) { }
static inline void _local_bh_enable_nort(void) { }

static inline void ksoftirqd_set_sched_params(void)
{
	struct sched_param param = { .sched_priority = 1 };

	sched_setscheduler(current, SCHED_FIFO, &param);
}

static inline void ksoftirqd_clr_sched_params(void)
{
	struct sched_param param = { .sched_priority = 0 };

	sched_setscheduler(current, SCHED_NORMAL, &param);
}

#endif /* PREEMPT_RT_FULL */
/*
 * Enter an interrupt context.
 */
void irq_enter(void)
{
	int cpu = smp_processor_id();

	rcu_irq_enter();
	if (is_idle_task(current) && !in_interrupt()) {
		/*
		 * Prevent raise_softirq from needlessly waking up ksoftirqd
		 * here, as softirq will be serviced on return from interrupt.
		 */
		local_bh_disable_nort();
		tick_check_idle(cpu);
		_local_bh_enable_nort();
	}

	__irq_enter();
}

static inline void invoke_softirq(void)
{
#ifndef CONFIG_PREEMPT_RT_FULL
	if (!force_irqthreads) {
#ifdef __ARCH_IRQ_EXIT_IRQS_DISABLED
		__do_softirq();
#else
		do_softirq();
#endif
	} else {
		__local_bh_disable((unsigned long)__builtin_return_address(0),
				SOFTIRQ_OFFSET);
		wakeup_softirqd();
		__local_bh_enable(SOFTIRQ_OFFSET);
	}
#else
		wakeup_softirqd();
#endif
}

/*
 * Exit an interrupt context. Process softirqs if needed and possible:
 */
void irq_exit(void)
{
	account_system_vtime(current);
	trace_hardirq_exit();
	sub_preempt_count(IRQ_EXIT_OFFSET);
	if (!in_interrupt() && local_softirq_pending())
		invoke_softirq();

#ifdef CONFIG_NO_HZ
	/* Make sure that timer wheel updates are propagated */
	if (idle_cpu(smp_processor_id()) && !in_interrupt() && !need_resched())
		tick_nohz_irq_exit();
#endif
	rcu_irq_exit();
	sched_preempt_enable_no_resched();
}

/*
 * This function must run with irqs disabled!
 */
inline void raise_softirq_irqoff(unsigned int nr)
{
	__raise_softirq_irqoff(nr);

	/*
	 * If we're in an interrupt or softirq, we're done
	 * (this also catches softirq-disabled code). We will
	 * actually run the softirq once we return from
	 * the irq or softirq.
	 *
	 * Otherwise we wake up ksoftirqd to make sure we
	 * schedule the softirq soon.
	 */
	if (!in_interrupt())
		wakeup_softirqd();
}

void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}

void __raise_softirq_irqoff(unsigned int nr)
{
	trace_softirq_raise(nr);
	or_softirq_pending(1UL << nr);
}

void open_softirq(int nr, void (*action)(struct softirq_action *))
{
	softirq_vec[nr].action = action;
}

/*
 * Tasklets
 */
struct tasklet_head
{
	struct tasklet_struct *head;
	struct tasklet_struct **tail;
};

static DEFINE_PER_CPU(struct tasklet_head, tasklet_vec);
static DEFINE_PER_CPU(struct tasklet_head, tasklet_hi_vec);

static void inline
__tasklet_common_schedule(struct tasklet_struct *t, struct tasklet_head *head, unsigned int nr)
{
	if (tasklet_trylock(t)) {
again:
		/* We may have been preempted before tasklet_trylock
		 * and __tasklet_action may have already run.
		 * So double check the sched bit while the takslet
		 * is locked before adding it to the list.
		 */
		if (test_bit(TASKLET_STATE_SCHED, &t->state)) {
			t->next = NULL;
			*head->tail = t;
			head->tail = &(t->next);
			raise_softirq_irqoff(nr);
			tasklet_unlock(t);
		} else {
			/* This is subtle. If we hit the corner case above
			 * It is possible that we get preempted right here,
			 * and another task has successfully called
			 * tasklet_schedule(), then this function, and
			 * failed on the trylock. Thus we must be sure
			 * before releasing the tasklet lock, that the
			 * SCHED_BIT is clear. Otherwise the tasklet
			 * may get its SCHED_BIT set, but not added to the
			 * list
			 */
			if (!tasklet_tryunlock(t))
				goto again;
		}
	}
}

void __tasklet_schedule(struct tasklet_struct *t)
{
	unsigned long flags;

	local_irq_save(flags);
	__tasklet_common_schedule(t, &__get_cpu_var(tasklet_vec), TASKLET_SOFTIRQ);
	local_irq_restore(flags);
}

EXPORT_SYMBOL(__tasklet_schedule);

void __tasklet_hi_schedule(struct tasklet_struct *t)
{
	unsigned long flags;

	local_irq_save(flags);
	__tasklet_common_schedule(t, &__get_cpu_var(tasklet_hi_vec), HI_SOFTIRQ);
	local_irq_restore(flags);
}

EXPORT_SYMBOL(__tasklet_hi_schedule);

void __tasklet_hi_schedule_first(struct tasklet_struct *t)
{
	__tasklet_hi_schedule(t);
}

EXPORT_SYMBOL(__tasklet_hi_schedule_first);

void  tasklet_enable(struct tasklet_struct *t)
{
	if (!atomic_dec_and_test(&t->count))
		return;
	if (test_and_clear_bit(TASKLET_STATE_PENDING, &t->state))
		tasklet_schedule(t);
}

EXPORT_SYMBOL(tasklet_enable);

void  tasklet_hi_enable(struct tasklet_struct *t)
{
	if (!atomic_dec_and_test(&t->count))
		return;
	if (test_and_clear_bit(TASKLET_STATE_PENDING, &t->state))
		tasklet_hi_schedule(t);
}

EXPORT_SYMBOL(tasklet_hi_enable);

static void
__tasklet_action(struct softirq_action *a, struct tasklet_struct *list)
{
	int loops = 1000000;

	while (list) {
		struct tasklet_struct *t = list;

		list = list->next;

		/*
		 * Should always succeed - after a tasklist got on the
		 * list (after getting the SCHED bit set from 0 to 1),
		 * nothing but the tasklet softirq it got queued to can
		 * lock it:
		 */
		if (!tasklet_trylock(t)) {
			WARN_ON(1);
			continue;
		}

		t->next = NULL;

		/*
		 * If we cannot handle the tasklet because it's disabled,
		 * mark it as pending. tasklet_enable() will later
		 * re-schedule the tasklet.
		 */
		if (unlikely(atomic_read(&t->count))) {
out_disabled:
			/* implicit unlock: */
			wmb();
			t->state = TASKLET_STATEF_PENDING;
			continue;
		}

		/*
		 * After this point on the tasklet might be rescheduled
		 * on another CPU, but it can only be added to another
		 * CPU's tasklet list if we unlock the tasklet (which we
		 * dont do yet).
		 */
		if (!test_and_clear_bit(TASKLET_STATE_SCHED, &t->state))
			WARN_ON(1);

again:
		t->func(t->data);

		/*
		 * Try to unlock the tasklet. We must use cmpxchg, because
		 * another CPU might have scheduled or disabled the tasklet.
		 * We only allow the STATE_RUN -> 0 transition here.
		 */
		while (!tasklet_tryunlock(t)) {
			/*
			 * If it got disabled meanwhile, bail out:
			 */
			if (atomic_read(&t->count))
				goto out_disabled;
			/*
			 * If it got scheduled meanwhile, re-execute
			 * the tasklet function:
			 */
			if (test_and_clear_bit(TASKLET_STATE_SCHED, &t->state))
				goto again;
			if (!--loops) {
				printk("hm, tasklet state: %08lx\n", t->state);
				WARN_ON(1);
				tasklet_unlock(t);
				break;
			}
		}
	}
}

static void tasklet_action(struct softirq_action *a)
{
	struct tasklet_struct *list;

	local_irq_disable();
	list = __get_cpu_var(tasklet_vec).head;
	__get_cpu_var(tasklet_vec).head = NULL;
	__get_cpu_var(tasklet_vec).tail = &__get_cpu_var(tasklet_vec).head;
	local_irq_enable();

	__tasklet_action(a, list);
}

static void tasklet_hi_action(struct softirq_action *a)
{
	struct tasklet_struct *list;

	local_irq_disable();
	list = __this_cpu_read(tasklet_hi_vec.head);
	__this_cpu_write(tasklet_hi_vec.head, NULL);
	__this_cpu_write(tasklet_hi_vec.tail, &__get_cpu_var(tasklet_hi_vec).head);
	local_irq_enable();

	__tasklet_action(a, list);
}


void tasklet_init(struct tasklet_struct *t,
		  void (*func)(unsigned long), unsigned long data)
{
	t->next = NULL;
	t->state = 0;
	atomic_set(&t->count, 0);
	t->func = func;
	t->data = data;
}

EXPORT_SYMBOL(tasklet_init);

void tasklet_kill(struct tasklet_struct *t)
{
	if (in_interrupt())
		printk("Attempt to kill tasklet from interrupt\n");

	while (test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		do {
			msleep(1);
		} while (test_bit(TASKLET_STATE_SCHED, &t->state));
	}
	tasklet_unlock_wait(t);
	clear_bit(TASKLET_STATE_SCHED, &t->state);
}

EXPORT_SYMBOL(tasklet_kill);

/*
 * tasklet_hrtimer
 */

/*
 * The trampoline is called when the hrtimer expires. It schedules a tasklet
 * to run __tasklet_hrtimer_trampoline() which in turn will call the intended
 * hrtimer callback, but from softirq context.
 */
static enum hrtimer_restart __hrtimer_tasklet_trampoline(struct hrtimer *timer)
{
	struct tasklet_hrtimer *ttimer =
		container_of(timer, struct tasklet_hrtimer, timer);

	tasklet_hi_schedule(&ttimer->tasklet);
	return HRTIMER_NORESTART;
}

/*
 * Helper function which calls the hrtimer callback from
 * tasklet/softirq context
 */
static void __tasklet_hrtimer_trampoline(unsigned long data)
{
	struct tasklet_hrtimer *ttimer = (void *)data;
	enum hrtimer_restart restart;

	restart = ttimer->function(&ttimer->timer);
	if (restart != HRTIMER_NORESTART)
		hrtimer_restart(&ttimer->timer);
}

/**
 * tasklet_hrtimer_init - Init a tasklet/hrtimer combo for softirq callbacks
 * @ttimer:	 tasklet_hrtimer which is initialized
 * @function:	 hrtimer callback function which gets called from softirq context
 * @which_clock: clock id (CLOCK_MONOTONIC/CLOCK_REALTIME)
 * @mode:	 hrtimer mode (HRTIMER_MODE_ABS/HRTIMER_MODE_REL)
 */
void tasklet_hrtimer_init(struct tasklet_hrtimer *ttimer,
			  enum hrtimer_restart (*function)(struct hrtimer *),
			  clockid_t which_clock, enum hrtimer_mode mode)
{
	hrtimer_init(&ttimer->timer, which_clock, mode);
	ttimer->timer.function = __hrtimer_tasklet_trampoline;
	tasklet_init(&ttimer->tasklet, __tasklet_hrtimer_trampoline,
		     (unsigned long)ttimer);
	ttimer->function = function;
}
EXPORT_SYMBOL_GPL(tasklet_hrtimer_init);

/*
 * Remote softirq bits
 */

DEFINE_PER_CPU(struct list_head [NR_SOFTIRQS], softirq_work_list);
EXPORT_PER_CPU_SYMBOL(softirq_work_list);

static void __local_trigger(struct call_single_data *cp, int softirq)
{
	struct list_head *head = &__get_cpu_var(softirq_work_list[softirq]);

	list_add_tail(&cp->list, head);

	/* Trigger the softirq only if the list was previously empty.  */
	if (head->next == &cp->list)
		raise_softirq_irqoff(softirq);
}

#ifdef CONFIG_USE_GENERIC_SMP_HELPERS
static void remote_softirq_receive(void *data)
{
	struct call_single_data *cp = data;
	unsigned long flags;
	int softirq;

	softirq = cp->priv;

	local_irq_save(flags);
	__local_trigger(cp, softirq);
	local_irq_restore(flags);
}

static int __try_remote_softirq(struct call_single_data *cp, int cpu, int softirq)
{
	if (cpu_online(cpu)) {
		cp->func = remote_softirq_receive;
		cp->info = cp;
		cp->flags = 0;
		cp->priv = softirq;

		__smp_call_function_single(cpu, cp, 0);
		return 0;
	}
	return 1;
}
#else /* CONFIG_USE_GENERIC_SMP_HELPERS */
static int __try_remote_softirq(struct call_single_data *cp, int cpu, int softirq)
{
	return 1;
}
#endif

/**
 * __send_remote_softirq - try to schedule softirq work on a remote cpu
 * @cp: private SMP call function data area
 * @cpu: the remote cpu
 * @this_cpu: the currently executing cpu
 * @softirq: the softirq for the work
 *
 * Attempt to schedule softirq work on a remote cpu.  If this cannot be
 * done, the work is instead queued up on the local cpu.
 *
 * Interrupts must be disabled.
 */
void __send_remote_softirq(struct call_single_data *cp, int cpu, int this_cpu, int softirq)
{
	if (cpu == this_cpu || __try_remote_softirq(cp, cpu, softirq))
		__local_trigger(cp, softirq);
}
EXPORT_SYMBOL(__send_remote_softirq);

/**
 * send_remote_softirq - try to schedule softirq work on a remote cpu
 * @cp: private SMP call function data area
 * @cpu: the remote cpu
 * @softirq: the softirq for the work
 *
 * Like __send_remote_softirq except that disabling interrupts and
 * computing the current cpu is done for the caller.
 */
void send_remote_softirq(struct call_single_data *cp, int cpu, int softirq)
{
	unsigned long flags;
	int this_cpu;

	local_irq_save(flags);
	this_cpu = smp_processor_id();
	__send_remote_softirq(cp, cpu, this_cpu, softirq);
	local_irq_restore(flags);
}
EXPORT_SYMBOL(send_remote_softirq);

static int __cpuinit remote_softirq_cpu_notify(struct notifier_block *self,
					       unsigned long action, void *hcpu)
{
	/*
	 * If a CPU goes away, splice its entries to the current CPU
	 * and trigger a run of the softirq
	 */
	if (action == CPU_DEAD || action == CPU_DEAD_FROZEN) {
		int cpu = (unsigned long) hcpu;
		int i;

		local_irq_disable();
		for (i = 0; i < NR_SOFTIRQS; i++) {
			struct list_head *head = &per_cpu(softirq_work_list[i], cpu);
			struct list_head *local_head;

			if (list_empty(head))
				continue;

			local_head = &__get_cpu_var(softirq_work_list[i]);
			list_splice_init(head, local_head);
			raise_softirq_irqoff(i);
		}
		local_irq_enable();
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata remote_softirq_cpu_notifier = {
	.notifier_call	= remote_softirq_cpu_notify,
};

void __init softirq_init(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		int i;

		per_cpu(tasklet_vec, cpu).tail =
			&per_cpu(tasklet_vec, cpu).head;
		per_cpu(tasklet_hi_vec, cpu).tail =
			&per_cpu(tasklet_hi_vec, cpu).head;
		for (i = 0; i < NR_SOFTIRQS; i++)
			INIT_LIST_HEAD(&per_cpu(softirq_work_list[i], cpu));
	}

	register_hotcpu_notifier(&remote_softirq_cpu_notifier);

	open_softirq(TASKLET_SOFTIRQ, tasklet_action);
	open_softirq(HI_SOFTIRQ, tasklet_hi_action);
}

#if defined(CONFIG_SMP) || defined(CONFIG_PREEMPT_RT_FULL)
void tasklet_unlock_wait(struct tasklet_struct *t)
{
	while (test_bit(TASKLET_STATE_RUN, &(t)->state)) {
		/*
		 * Hack for now to avoid this busy-loop:
		 */
#ifdef CONFIG_PREEMPT_RT_FULL
		msleep(1);
#else
		barrier();
#endif
	}
}
EXPORT_SYMBOL(tasklet_unlock_wait);
#endif

static int run_ksoftirqd(void * __bind_cpu)
{
	ksoftirqd_set_sched_params();

	set_current_state(TASK_INTERRUPTIBLE);

	while (!kthread_should_stop()) {
		preempt_disable();
		if (!local_softirq_pending())
			schedule_preempt_disabled();

		__set_current_state(TASK_RUNNING);

		while (local_softirq_pending()) {
			if (ksoftirqd_do_softirq((long) __bind_cpu))
				goto wait_to_die;
			sched_preempt_enable_no_resched();
			cond_resched();
			preempt_disable();
			rcu_note_context_switch((long)__bind_cpu);
		}
		preempt_enable();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;

wait_to_die:
	preempt_enable();
	ksoftirqd_clr_sched_params();
	/* Wait for kthread_stop */
	set_current_state(TASK_INTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_INTERRUPTIBLE);
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
/*
 * tasklet_kill_immediate is called to remove a tasklet which can already be
 * scheduled for execution on @cpu.
 *
 * Unlike tasklet_kill, this function removes the tasklet
 * _immediately_, even if the tasklet is in TASKLET_STATE_SCHED state.
 *
 * When this function is called, @cpu must be in the CPU_DEAD state.
 */
void tasklet_kill_immediate(struct tasklet_struct *t, unsigned int cpu)
{
	struct tasklet_struct **i;

	BUG_ON(cpu_online(cpu));
	BUG_ON(test_bit(TASKLET_STATE_RUN, &t->state));

	if (!test_bit(TASKLET_STATE_SCHED, &t->state))
		return;

	/* CPU is dead, so no lock needed. */
	for (i = &per_cpu(tasklet_vec, cpu).head; *i; i = &(*i)->next) {
		if (*i == t) {
			*i = t->next;
			/* If this was the tail element, move the tail ptr */
			if (*i == NULL)
				per_cpu(tasklet_vec, cpu).tail = i;
			return;
		}
	}
	BUG();
}

static void takeover_tasklets(unsigned int cpu)
{
	/* CPU is dead, so no lock needed. */
	local_irq_disable();

	/* Find end, append list for that CPU. */
	if (&per_cpu(tasklet_vec, cpu).head != per_cpu(tasklet_vec, cpu).tail) {
		*__this_cpu_read(tasklet_vec.tail) = per_cpu(tasklet_vec, cpu).head;
		this_cpu_write(tasklet_vec.tail, per_cpu(tasklet_vec, cpu).tail);
		per_cpu(tasklet_vec, cpu).head = NULL;
		per_cpu(tasklet_vec, cpu).tail = &per_cpu(tasklet_vec, cpu).head;
	}
	raise_softirq_irqoff(TASKLET_SOFTIRQ);

	if (&per_cpu(tasklet_hi_vec, cpu).head != per_cpu(tasklet_hi_vec, cpu).tail) {
		*__this_cpu_read(tasklet_hi_vec.tail) = per_cpu(tasklet_hi_vec, cpu).head;
		__this_cpu_write(tasklet_hi_vec.tail, per_cpu(tasklet_hi_vec, cpu).tail);
		per_cpu(tasklet_hi_vec, cpu).head = NULL;
		per_cpu(tasklet_hi_vec, cpu).tail = &per_cpu(tasklet_hi_vec, cpu).head;
	}
	raise_softirq_irqoff(HI_SOFTIRQ);

	local_irq_enable();
}
#endif /* CONFIG_HOTPLUG_CPU */

static int __cpuinit cpu_callback(struct notifier_block *nfb,
				  unsigned long action,
				  void *hcpu)
{
	int hotcpu = (unsigned long)hcpu;
	struct task_struct *p;

	switch (action & ~CPU_TASKS_FROZEN) {
	case CPU_UP_PREPARE:
		p = kthread_create_on_node(run_ksoftirqd,
					   hcpu,
					   cpu_to_node(hotcpu),
					   "ksoftirqd/%d", hotcpu);
		if (IS_ERR(p)) {
			printk("ksoftirqd for %i failed\n", hotcpu);
			return notifier_from_errno(PTR_ERR(p));
		}
		kthread_bind(p, hotcpu);
  		per_cpu(ksoftirqd, hotcpu) = p;
 		break;
	case CPU_ONLINE:
		wake_up_process(per_cpu(ksoftirqd, hotcpu));
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_UP_CANCELED:
		if (!per_cpu(ksoftirqd, hotcpu))
			break;
		/* Unbind so it can run.  Fall thru. */
		kthread_bind(per_cpu(ksoftirqd, hotcpu),
			     cpumask_any(cpu_online_mask));
	case CPU_POST_DEAD: {
		static const struct sched_param param = {
			.sched_priority = MAX_RT_PRIO-1
		};

		p = per_cpu(ksoftirqd, hotcpu);
		per_cpu(ksoftirqd, hotcpu) = NULL;
		sched_setscheduler_nocheck(p, SCHED_FIFO, &param);
		kthread_stop(p);
		takeover_tasklets(hotcpu);
		break;
	}
#endif /* CONFIG_HOTPLUG_CPU */
 	}
	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata cpu_nfb = {
	.notifier_call = cpu_callback
};

static __init int spawn_ksoftirqd(void)
{
	void *cpu = (void *)(long)smp_processor_id();
	int err = cpu_callback(&cpu_nfb, CPU_UP_PREPARE, cpu);

	BUG_ON(err != NOTIFY_OK);
	cpu_callback(&cpu_nfb, CPU_ONLINE, cpu);
	register_cpu_notifier(&cpu_nfb);
	return 0;
}
early_initcall(spawn_ksoftirqd);

/*
 * [ These __weak aliases are kept in a separate compilation unit, so that
 *   GCC does not inline them incorrectly. ]
 */

int __init __weak early_irq_init(void)
{
	return 0;
}

#ifdef CONFIG_GENERIC_HARDIRQS
int __init __weak arch_probe_nr_irqs(void)
{
	return NR_IRQS_LEGACY;
}

int __init __weak arch_early_irq_init(void)
{
	return 0;
}
#endif
