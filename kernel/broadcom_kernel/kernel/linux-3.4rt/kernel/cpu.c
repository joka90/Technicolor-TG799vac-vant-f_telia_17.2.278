/* CPU control.
 * (C) 2001, 2002, 2003, 2004 Rusty Russell
 *
 * This code is licenced under the GPL.
 */
#include <linux/proc_fs.h>
#include <linux/smp.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/cpu.h>
#include <linux/export.h>
#include <linux/kthread.h>
#include <linux/stop_machine.h>
#include <linux/mutex.h>
#include <linux/gfp.h>
#include <linux/suspend.h>

#ifdef CONFIG_SMP
/* Serializes the updates to cpu_online_mask, cpu_present_mask */
static DEFINE_MUTEX(cpu_add_remove_lock);

/*
 * The following two API's must be used when attempting
 * to serialize the updates to cpu_online_mask, cpu_present_mask.
 */
void cpu_maps_update_begin(void)
{
	mutex_lock(&cpu_add_remove_lock);
}

void cpu_maps_update_done(void)
{
	mutex_unlock(&cpu_add_remove_lock);
}

static RAW_NOTIFIER_HEAD(cpu_chain);

/* If set, cpu_up and cpu_down will return -EBUSY and do nothing.
 * Should always be manipulated under cpu_add_remove_lock
 */
static int cpu_hotplug_disabled;

#ifdef CONFIG_HOTPLUG_CPU

static struct {
	struct task_struct *active_writer;
	struct mutex lock; /* Synchronizes accesses to refcount, */
	/*
	 * Also blocks the new readers during
	 * an ongoing cpu hotplug operation.
	 */
	int refcount;
} cpu_hotplug = {
	.active_writer = NULL,
	.lock = __MUTEX_INITIALIZER(cpu_hotplug.lock),
	.refcount = 0,
};

/**
 * hotplug_pcp - per cpu hotplug descriptor
 * @unplug:	set when pin_current_cpu() needs to sync tasks
 * @sync_tsk:	the task that waits for tasks to finish pinned sections
 * @refcount:	counter of tasks in pinned sections
 * @grab_lock:	set when the tasks entering pinned sections should wait
 * @synced:	notifier for @sync_tsk to tell cpu_down it's finished
 * @mutex:	the mutex to make tasks wait (used when @grab_lock is true)
 * @mutex_init:	zero if the mutex hasn't been initialized yet.
 *
 * Although @unplug and @sync_tsk may point to the same task, the @unplug
 * is used as a flag and still exists after @sync_tsk has exited and
 * @sync_tsk set to NULL.
 */
struct hotplug_pcp {
	struct task_struct *unplug;
	struct task_struct *sync_tsk;
	int refcount;
	int grab_lock;
	struct completion synced;
#ifdef CONFIG_PREEMPT_RT_FULL
	spinlock_t lock;
#else
	struct mutex mutex;
#endif
	int mutex_init;
};

#ifdef CONFIG_PREEMPT_RT_FULL
# define hotplug_lock(hp) rt_spin_lock(&(hp)->lock)
# define hotplug_unlock(hp) rt_spin_unlock(&(hp)->lock)
#else
# define hotplug_lock(hp) mutex_lock(&(hp)->mutex)
# define hotplug_unlock(hp) mutex_unlock(&(hp)->mutex)
#endif

static DEFINE_PER_CPU(struct hotplug_pcp, hotplug_pcp);

/**
 * pin_current_cpu - Prevent the current cpu from being unplugged
 *
 * Lightweight version of get_online_cpus() to prevent cpu from being
 * unplugged when code runs in a migration disabled region.
 *
 * Must be called with preemption disabled (preempt_count = 1)!
 */
void pin_current_cpu(void)
{
	struct hotplug_pcp *hp;
	int force = 0;

retry:
	hp = &__get_cpu_var(hotplug_pcp);

	if (!hp->unplug || hp->refcount || force || preempt_count() > 1 ||
	    hp->unplug == current || (current->flags & PF_STOMPER)) {
		hp->refcount++;
		return;
	}

	if (hp->grab_lock) {
		preempt_enable();
		hotplug_lock(hp);
		hotplug_unlock(hp);
	} else {
		preempt_enable();
		/*
		 * Try to push this task off of this CPU.
		 */
		if (!migrate_me()) {
			preempt_disable();
			hp = &__get_cpu_var(hotplug_pcp);
			if (!hp->grab_lock) {
				/*
				 * Just let it continue it's already pinned
				 * or about to sleep.
				 */
				force = 1;
				goto retry;
			}
			preempt_enable();
		}
	}
	preempt_disable();
	goto retry;
}

/**
 * unpin_current_cpu - Allow unplug of current cpu
 *
 * Must be called with preemption or interrupts disabled!
 */
void unpin_current_cpu(void)
{
	struct hotplug_pcp *hp = &__get_cpu_var(hotplug_pcp);

	WARN_ON(hp->refcount <= 0);

	/* This is safe. sync_unplug_thread is pinned to this cpu */
	if (!--hp->refcount && hp->unplug && hp->unplug != current &&
	    !(current->flags & PF_STOMPER))
		wake_up_process(hp->unplug);
}

static void wait_for_pinned_cpus(struct hotplug_pcp *hp)
{
	set_current_state(TASK_UNINTERRUPTIBLE);
	while (hp->refcount) {
		schedule_preempt_disabled();
		set_current_state(TASK_UNINTERRUPTIBLE);
	}
}

static int sync_unplug_thread(void *data)
{
	struct hotplug_pcp *hp = data;

	preempt_disable();
	hp->unplug = current;
	wait_for_pinned_cpus(hp);

	/*
	 * This thread will synchronize the cpu_down() with threads
	 * that have pinned the CPU. When the pinned CPU count reaches
	 * zero, we inform the cpu_down code to continue to the next step.
	 */
	set_current_state(TASK_UNINTERRUPTIBLE);
	preempt_enable();
	complete(&hp->synced);

	/*
	 * If all succeeds, the next step will need tasks to wait till
	 * the CPU is offline before continuing. To do this, the grab_lock
	 * is set and tasks going into pin_current_cpu() will block on the
	 * mutex. But we still need to wait for those that are already in
	 * pinned CPU sections. If the cpu_down() failed, the kthread_should_stop()
	 * will kick this thread out.
	 */
	while (!hp->grab_lock && !kthread_should_stop()) {
		schedule();
		set_current_state(TASK_UNINTERRUPTIBLE);
	}

	/* Make sure grab_lock is seen before we see a stale completion */
	smp_mb();

	/*
	 * Now just before cpu_down() enters stop machine, we need to make
	 * sure all tasks that are in pinned CPU sections are out, and new
	 * tasks will now grab the lock, keeping them from entering pinned
	 * CPU sections.
	 */
	if (!kthread_should_stop()) {
		preempt_disable();
		wait_for_pinned_cpus(hp);
		preempt_enable();
		complete(&hp->synced);
	}

	set_current_state(TASK_UNINTERRUPTIBLE);
	while (!kthread_should_stop()) {
		schedule();
		set_current_state(TASK_UNINTERRUPTIBLE);
	}
	set_current_state(TASK_RUNNING);

	/*
	 * Force this thread off this CPU as it's going down and
	 * we don't want any more work on this CPU.
	 */
	current->flags &= ~PF_THREAD_BOUND;
	do_set_cpus_allowed(current, cpu_present_mask);
	migrate_me();
	return 0;
}

static void __cpu_unplug_sync(struct hotplug_pcp *hp)
{
	wake_up_process(hp->sync_tsk);
	wait_for_completion(&hp->synced);
}

/*
 * Start the sync_unplug_thread on the target cpu and wait for it to
 * complete.
 */
static int cpu_unplug_begin(unsigned int cpu)
{
	struct hotplug_pcp *hp = &per_cpu(hotplug_pcp, cpu);
	int err;

	/* Protected by cpu_hotplug.lock */
	if (!hp->mutex_init) {
#ifdef CONFIG_PREEMPT_RT_FULL
		spin_lock_init(&hp->lock);
#else
		mutex_init(&hp->mutex);
#endif
		hp->mutex_init = 1;
	}

	/* Inform the scheduler to migrate tasks off this CPU */
	tell_sched_cpu_down_begin(cpu);

	init_completion(&hp->synced);

	hp->sync_tsk = kthread_create(sync_unplug_thread, hp, "sync_unplug/%d", cpu);
	if (IS_ERR(hp->sync_tsk)) {
		err = PTR_ERR(hp->sync_tsk);
		hp->sync_tsk = NULL;
		return err;
	}
	kthread_bind(hp->sync_tsk, cpu);

	/*
	 * Wait for tasks to get out of the pinned sections,
	 * it's still OK if new tasks enter. Some CPU notifiers will
	 * wait for tasks that are going to enter these sections and
	 * we must not have them block.
	 */
	__cpu_unplug_sync(hp);

	return 0;
}

static void cpu_unplug_sync(unsigned int cpu)
{
	struct hotplug_pcp *hp = &per_cpu(hotplug_pcp, cpu);

	init_completion(&hp->synced);
	/* The completion needs to be initialzied before setting grab_lock */
	smp_wmb();

	/* Grab the mutex before setting grab_lock */
	hotplug_lock(hp);
	hp->grab_lock = 1;

	/*
	 * The CPU notifiers have been completed.
	 * Wait for tasks to get out of pinned CPU sections and have new
	 * tasks block until the CPU is completely down.
	 */
	__cpu_unplug_sync(hp);

	/* All done with the sync thread */
	kthread_stop(hp->sync_tsk);
	hp->sync_tsk = NULL;
}

static void cpu_unplug_done(unsigned int cpu)
{
	struct hotplug_pcp *hp = &per_cpu(hotplug_pcp, cpu);

	hp->unplug = NULL;
	/* Let all tasks know cpu unplug is finished before cleaning up */
	smp_wmb();

	if (hp->sync_tsk)
		kthread_stop(hp->sync_tsk);

	if (hp->grab_lock) {
		hotplug_unlock(hp);
		/* protected by cpu_hotplug.lock */
		hp->grab_lock = 0;
	}
	tell_sched_cpu_down_done(cpu);
}

void get_online_cpus(void)
{
	might_sleep();
	if (cpu_hotplug.active_writer == current)
		return;
	mutex_lock(&cpu_hotplug.lock);
	cpu_hotplug.refcount++;
	mutex_unlock(&cpu_hotplug.lock);

}
EXPORT_SYMBOL_GPL(get_online_cpus);

void put_online_cpus(void)
{
	if (cpu_hotplug.active_writer == current)
		return;
	mutex_lock(&cpu_hotplug.lock);
	if (!--cpu_hotplug.refcount && unlikely(cpu_hotplug.active_writer))
		wake_up_process(cpu_hotplug.active_writer);
	mutex_unlock(&cpu_hotplug.lock);

}
EXPORT_SYMBOL_GPL(put_online_cpus);

/*
 * This ensures that the hotplug operation can begin only when the
 * refcount goes to zero.
 *
 * Note that during a cpu-hotplug operation, the new readers, if any,
 * will be blocked by the cpu_hotplug.lock
 *
 * Since cpu_hotplug_begin() is always called after invoking
 * cpu_maps_update_begin(), we can be sure that only one writer is active.
 *
 * Note that theoretically, there is a possibility of a livelock:
 * - Refcount goes to zero, last reader wakes up the sleeping
 *   writer.
 * - Last reader unlocks the cpu_hotplug.lock.
 * - A new reader arrives at this moment, bumps up the refcount.
 * - The writer acquires the cpu_hotplug.lock finds the refcount
 *   non zero and goes to sleep again.
 *
 * However, this is very difficult to achieve in practice since
 * get_online_cpus() not an api which is called all that often.
 *
 */
static void cpu_hotplug_begin(void)
{
	cpu_hotplug.active_writer = current;

	for (;;) {
		mutex_lock(&cpu_hotplug.lock);
		if (likely(!cpu_hotplug.refcount))
			break;
		__set_current_state(TASK_UNINTERRUPTIBLE);
		mutex_unlock(&cpu_hotplug.lock);
		schedule();
	}
}

static void cpu_hotplug_done(void)
{
	cpu_hotplug.active_writer = NULL;
	mutex_unlock(&cpu_hotplug.lock);
}

#else /* #if CONFIG_HOTPLUG_CPU */
static void cpu_hotplug_begin(void) {}
static void cpu_hotplug_done(void) {}
#endif	/* #else #if CONFIG_HOTPLUG_CPU */

/* Need to know about CPUs going up/down? */
int __ref register_cpu_notifier(struct notifier_block *nb)
{
	int ret;
	cpu_maps_update_begin();
	ret = raw_notifier_chain_register(&cpu_chain, nb);
	cpu_maps_update_done();
	return ret;
}

static int __cpu_notify(unsigned long val, void *v, int nr_to_call,
			int *nr_calls)
{
	int ret;

	ret = __raw_notifier_call_chain(&cpu_chain, val, v, nr_to_call,
					nr_calls);

	return notifier_to_errno(ret);
}

static int cpu_notify(unsigned long val, void *v)
{
	return __cpu_notify(val, v, -1, NULL);
}

#ifdef CONFIG_HOTPLUG_CPU

static void cpu_notify_nofail(unsigned long val, void *v)
{
	BUG_ON(cpu_notify(val, v));
}
EXPORT_SYMBOL(register_cpu_notifier);

void __ref unregister_cpu_notifier(struct notifier_block *nb)
{
	cpu_maps_update_begin();
	raw_notifier_chain_unregister(&cpu_chain, nb);
	cpu_maps_update_done();
}
EXPORT_SYMBOL(unregister_cpu_notifier);

static inline void check_for_tasks(int cpu)
{
	struct task_struct *p;

	write_lock_irq(&tasklist_lock);
	for_each_process(p) {
		if (task_cpu(p) == cpu && p->state == TASK_RUNNING &&
		    (p->utime || p->stime))
			printk(KERN_WARNING "Task %s (pid = %d) is on cpu %d "
				"(state = %ld, flags = %x)\n",
				p->comm, task_pid_nr(p), cpu,
				p->state, p->flags);
	}
	write_unlock_irq(&tasklist_lock);
}

struct take_cpu_down_param {
	unsigned long mod;
	void *hcpu;
};

/* Take this CPU down. */
static int __ref take_cpu_down(void *_param)
{
	struct take_cpu_down_param *param = _param;
	int err;

	/* Ensure this CPU doesn't handle any more interrupts. */
	err = __cpu_disable();
	if (err < 0)
		return err;

	cpu_notify(CPU_DYING | param->mod, param->hcpu);
	return 0;
}

/* Requires cpu_add_remove_lock to be held */
static int __ref _cpu_down(unsigned int cpu, int tasks_frozen)
{
	int mycpu, err, nr_calls = 0;
	void *hcpu = (void *)(long)cpu;
	unsigned long mod = tasks_frozen ? CPU_TASKS_FROZEN : 0;
	struct take_cpu_down_param tcd_param = {
		.mod = mod,
		.hcpu = hcpu,
	};
	cpumask_var_t cpumask;

	if (num_online_cpus() == 1)
		return -EBUSY;

	if (!cpu_online(cpu))
		return -EINVAL;

	/* Move the downtaker off the unplug cpu */
	if (!alloc_cpumask_var(&cpumask, GFP_KERNEL))
		return -ENOMEM;
	cpumask_andnot(cpumask, cpu_online_mask, cpumask_of(cpu));
	set_cpus_allowed_ptr(current, cpumask);
	free_cpumask_var(cpumask);
#if defined(CONFIG_BCM_KF_CPU_DOWN_PREEMPT_ON)
	migrate_disable_preempt_on();
#else
	migrate_disable();
#endif
	mycpu = smp_processor_id();
	if (mycpu == cpu) {
		printk(KERN_ERR "Yuck! Still on unplug CPU\n!");
#if defined(CONFIG_BCM_KF_CPU_DOWN_PREEMPT_ON)
		migrate_enable_preempt_on();
#else
		migrate_enable();
#endif
		return -EBUSY;
	}

	cpu_hotplug_begin();
	err = cpu_unplug_begin(cpu);
	if (err) {
		printk("cpu_unplug_begin(%d) failed\n", cpu);
		goto out_cancel;
	}

	err = __cpu_notify(CPU_DOWN_PREPARE | mod, hcpu, -1, &nr_calls);
	if (err) {
		nr_calls--;
		__cpu_notify(CPU_DOWN_FAILED | mod, hcpu, nr_calls, NULL);
		printk("%s: attempt to take down CPU %u failed\n",
				__func__, cpu);
		goto out_release;
	}

	/* Notifiers are done. Don't let any more tasks pin this CPU. */
	cpu_unplug_sync(cpu);

	err = __stop_machine(take_cpu_down, &tcd_param, cpumask_of(cpu));
	if (err) {
		/* CPU didn't die: tell everyone.  Can't complain. */
		cpu_notify_nofail(CPU_DOWN_FAILED | mod, hcpu);

		goto out_release;
	}
	BUG_ON(cpu_online(cpu));

	/*
	 * The migration_call() CPU_DYING callback will have removed all
	 * runnable tasks from the cpu, there's only the idle task left now
	 * that the migration thread is done doing the stop_machine thing.
	 *
	 * Wait for the stop thread to go away.
	 */
	while (!idle_cpu(cpu))
		cpu_relax();

	/* This actually kills the CPU. */
	__cpu_die(cpu);

	/* CPU is completely dead: tell everyone.  Too late to complain. */
	cpu_notify_nofail(CPU_DEAD | mod, hcpu);

	check_for_tasks(cpu);

out_release:
	cpu_unplug_done(cpu);
out_cancel:
#if defined(CONFIG_BCM_KF_CPU_DOWN_PREEMPT_ON)
	migrate_enable_preempt_on();
#else
	migrate_enable();
#endif
	cpu_hotplug_done();
	if (!err)
		cpu_notify_nofail(CPU_POST_DEAD | mod, hcpu);
	return err;
}

int __ref cpu_down(unsigned int cpu)
{
	int err;

	cpu_maps_update_begin();

	if (cpu_hotplug_disabled) {
		err = -EBUSY;
		goto out;
	}

	err = _cpu_down(cpu, 0);

out:
	cpu_maps_update_done();
	return err;
}
EXPORT_SYMBOL(cpu_down);
#endif /*CONFIG_HOTPLUG_CPU*/

/* Requires cpu_add_remove_lock to be held */
static int __cpuinit _cpu_up(unsigned int cpu, int tasks_frozen)
{
	int ret, nr_calls = 0;
	void *hcpu = (void *)(long)cpu;
	unsigned long mod = tasks_frozen ? CPU_TASKS_FROZEN : 0;

	if (cpu_online(cpu) || !cpu_present(cpu))
		return -EINVAL;

	cpu_hotplug_begin();
	ret = __cpu_notify(CPU_UP_PREPARE | mod, hcpu, -1, &nr_calls);
	if (ret) {
		nr_calls--;
		printk(KERN_WARNING "%s: attempt to bring up CPU %u failed\n",
				__func__, cpu);
		goto out_notify;
	}

	/* Arch-specific enabling code. */
	ret = __cpu_up(cpu);
	if (ret != 0)
		goto out_notify;
	BUG_ON(!cpu_online(cpu));

	/* Now call notifier in preparation. */
	cpu_notify(CPU_ONLINE | mod, hcpu);

out_notify:
	if (ret != 0)
		__cpu_notify(CPU_UP_CANCELED | mod, hcpu, nr_calls, NULL);
	cpu_hotplug_done();

	return ret;
}

int __cpuinit cpu_up(unsigned int cpu)
{
	int err = 0;

#ifdef	CONFIG_MEMORY_HOTPLUG
	int nid;
	pg_data_t	*pgdat;
#endif

	if (!cpu_possible(cpu)) {
		printk(KERN_ERR "can't online cpu %d because it is not "
			"configured as may-hotadd at boot time\n", cpu);
#if defined(CONFIG_IA64)
		printk(KERN_ERR "please check additional_cpus= boot "
				"parameter\n");
#endif
		return -EINVAL;
	}

#ifdef	CONFIG_MEMORY_HOTPLUG
	nid = cpu_to_node(cpu);
	if (!node_online(nid)) {
		err = mem_online_node(nid);
		if (err)
			return err;
	}

	pgdat = NODE_DATA(nid);
	if (!pgdat) {
		printk(KERN_ERR
			"Can't online cpu %d due to NULL pgdat\n", cpu);
		return -ENOMEM;
	}

	if (pgdat->node_zonelists->_zonerefs->zone == NULL) {
		mutex_lock(&zonelists_mutex);
		build_all_zonelists(NULL);
		mutex_unlock(&zonelists_mutex);
	}
#endif

	cpu_maps_update_begin();

	if (cpu_hotplug_disabled) {
		err = -EBUSY;
		goto out;
	}

	err = _cpu_up(cpu, 0);

out:
	cpu_maps_update_done();
	return err;
}
EXPORT_SYMBOL_GPL(cpu_up);

#ifdef CONFIG_PM_SLEEP_SMP
static cpumask_var_t frozen_cpus;

void __weak arch_disable_nonboot_cpus_begin(void)
{
}

void __weak arch_disable_nonboot_cpus_end(void)
{
}

int disable_nonboot_cpus(void)
{
	int cpu, first_cpu, error = 0;

	cpu_maps_update_begin();
	first_cpu = cpumask_first(cpu_online_mask);
	/*
	 * We take down all of the non-boot CPUs in one shot to avoid races
	 * with the userspace trying to use the CPU hotplug at the same time
	 */
	cpumask_clear(frozen_cpus);
	arch_disable_nonboot_cpus_begin();

	printk("Disabling non-boot CPUs ...\n");
	for_each_online_cpu(cpu) {
		if (cpu == first_cpu)
			continue;
		error = _cpu_down(cpu, 1);
		if (!error)
			cpumask_set_cpu(cpu, frozen_cpus);
		else {
			printk(KERN_ERR "Error taking CPU%d down: %d\n",
				cpu, error);
			break;
		}
	}

	arch_disable_nonboot_cpus_end();

	if (!error) {
		BUG_ON(num_online_cpus() > 1);
		/* Make sure the CPUs won't be enabled by someone else */
		cpu_hotplug_disabled = 1;
	} else {
		printk(KERN_ERR "Non-boot CPUs are not disabled\n");
	}
	cpu_maps_update_done();
	return error;
}

void __weak arch_enable_nonboot_cpus_begin(void)
{
}

void __weak arch_enable_nonboot_cpus_end(void)
{
}

void __ref enable_nonboot_cpus(void)
{
	int cpu, error;

	/* Allow everyone to use the CPU hotplug again */
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 0;
	if (cpumask_empty(frozen_cpus))
		goto out;

	printk(KERN_INFO "Enabling non-boot CPUs ...\n");

	arch_enable_nonboot_cpus_begin();

	for_each_cpu(cpu, frozen_cpus) {
		error = _cpu_up(cpu, 1);
		if (!error) {
			printk(KERN_INFO "CPU%d is up\n", cpu);
			continue;
		}
		printk(KERN_WARNING "Error taking CPU%d up: %d\n", cpu, error);
	}

	arch_enable_nonboot_cpus_end();

	cpumask_clear(frozen_cpus);
out:
	cpu_maps_update_done();
}

static int __init alloc_frozen_cpus(void)
{
	if (!alloc_cpumask_var(&frozen_cpus, GFP_KERNEL|__GFP_ZERO))
		return -ENOMEM;
	return 0;
}
core_initcall(alloc_frozen_cpus);

/*
 * Prevent regular CPU hotplug from racing with the freezer, by disabling CPU
 * hotplug when tasks are about to be frozen. Also, don't allow the freezer
 * to continue until any currently running CPU hotplug operation gets
 * completed.
 * To modify the 'cpu_hotplug_disabled' flag, we need to acquire the
 * 'cpu_add_remove_lock'. And this same lock is also taken by the regular
 * CPU hotplug path and released only after it is complete. Thus, we
 * (and hence the freezer) will block here until any currently running CPU
 * hotplug operation gets completed.
 */
void cpu_hotplug_disable_before_freeze(void)
{
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 1;
	cpu_maps_update_done();
}


/*
 * When tasks have been thawed, re-enable regular CPU hotplug (which had been
 * disabled while beginning to freeze tasks).
 */
void cpu_hotplug_enable_after_thaw(void)
{
	cpu_maps_update_begin();
	cpu_hotplug_disabled = 0;
	cpu_maps_update_done();
}

/*
 * When callbacks for CPU hotplug notifications are being executed, we must
 * ensure that the state of the system with respect to the tasks being frozen
 * or not, as reported by the notification, remains unchanged *throughout the
 * duration* of the execution of the callbacks.
 * Hence we need to prevent the freezer from racing with regular CPU hotplug.
 *
 * This synchronization is implemented by mutually excluding regular CPU
 * hotplug and Suspend/Hibernate call paths by hooking onto the Suspend/
 * Hibernate notifications.
 */
static int
cpu_hotplug_pm_callback(struct notifier_block *nb,
			unsigned long action, void *ptr)
{
	switch (action) {

	case PM_SUSPEND_PREPARE:
	case PM_HIBERNATION_PREPARE:
		cpu_hotplug_disable_before_freeze();
		break;

	case PM_POST_SUSPEND:
	case PM_POST_HIBERNATION:
		cpu_hotplug_enable_after_thaw();
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}


static int __init cpu_hotplug_pm_sync_init(void)
{
	pm_notifier(cpu_hotplug_pm_callback, 0);
	return 0;
}
core_initcall(cpu_hotplug_pm_sync_init);

#endif /* CONFIG_PM_SLEEP_SMP */

/**
 * notify_cpu_starting(cpu) - call the CPU_STARTING notifiers
 * @cpu: cpu that just started
 *
 * This function calls the cpu_chain notifiers with CPU_STARTING.
 * It must be called by the arch code on the new cpu, before the new cpu
 * enables interrupts and before the "boot" cpu returns from __cpu_up().
 */
void __cpuinit notify_cpu_starting(unsigned int cpu)
{
	unsigned long val = CPU_STARTING;

#ifdef CONFIG_PM_SLEEP_SMP
	if (frozen_cpus != NULL && cpumask_test_cpu(cpu, frozen_cpus))
		val = CPU_STARTING_FROZEN;
#endif /* CONFIG_PM_SLEEP_SMP */
	cpu_notify(val, (void *)(long)cpu);
}

#endif /* CONFIG_SMP */

/*
 * cpu_bit_bitmap[] is a special, "compressed" data structure that
 * represents all NR_CPUS bits binary values of 1<<nr.
 *
 * It is used by cpumask_of() to get a constant address to a CPU
 * mask value that has a single bit set only.
 */

/* cpu_bit_bitmap[0] is empty - so we can back into it */
#define MASK_DECLARE_1(x)	[x+1][0] = (1UL << (x))
#define MASK_DECLARE_2(x)	MASK_DECLARE_1(x), MASK_DECLARE_1(x+1)
#define MASK_DECLARE_4(x)	MASK_DECLARE_2(x), MASK_DECLARE_2(x+2)
#define MASK_DECLARE_8(x)	MASK_DECLARE_4(x), MASK_DECLARE_4(x+4)

const unsigned long cpu_bit_bitmap[BITS_PER_LONG+1][BITS_TO_LONGS(NR_CPUS)] = {

	MASK_DECLARE_8(0),	MASK_DECLARE_8(8),
	MASK_DECLARE_8(16),	MASK_DECLARE_8(24),
#if BITS_PER_LONG > 32
	MASK_DECLARE_8(32),	MASK_DECLARE_8(40),
	MASK_DECLARE_8(48),	MASK_DECLARE_8(56),
#endif
};
EXPORT_SYMBOL_GPL(cpu_bit_bitmap);

const DECLARE_BITMAP(cpu_all_bits, NR_CPUS) = CPU_BITS_ALL;
EXPORT_SYMBOL(cpu_all_bits);

#ifdef CONFIG_INIT_ALL_POSSIBLE
static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) __read_mostly
	= CPU_BITS_ALL;
#else
static DECLARE_BITMAP(cpu_possible_bits, CONFIG_NR_CPUS) __read_mostly;
#endif
const struct cpumask *const cpu_possible_mask = to_cpumask(cpu_possible_bits);
EXPORT_SYMBOL(cpu_possible_mask);

static DECLARE_BITMAP(cpu_online_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_online_mask = to_cpumask(cpu_online_bits);
EXPORT_SYMBOL(cpu_online_mask);

static DECLARE_BITMAP(cpu_present_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_present_mask = to_cpumask(cpu_present_bits);
EXPORT_SYMBOL(cpu_present_mask);

static DECLARE_BITMAP(cpu_active_bits, CONFIG_NR_CPUS) __read_mostly;
const struct cpumask *const cpu_active_mask = to_cpumask(cpu_active_bits);
EXPORT_SYMBOL(cpu_active_mask);

void set_cpu_possible(unsigned int cpu, bool possible)
{
	if (possible)
		cpumask_set_cpu(cpu, to_cpumask(cpu_possible_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_possible_bits));
}

void set_cpu_present(unsigned int cpu, bool present)
{
	if (present)
		cpumask_set_cpu(cpu, to_cpumask(cpu_present_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_present_bits));
}

void set_cpu_online(unsigned int cpu, bool online)
{
	if (online)
		cpumask_set_cpu(cpu, to_cpumask(cpu_online_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_online_bits));
}

void set_cpu_active(unsigned int cpu, bool active)
{
	if (active)
		cpumask_set_cpu(cpu, to_cpumask(cpu_active_bits));
	else
		cpumask_clear_cpu(cpu, to_cpumask(cpu_active_bits));
}

void init_cpu_present(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_present_bits), src);
}

void init_cpu_possible(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_possible_bits), src);
}

void init_cpu_online(const struct cpumask *src)
{
	cpumask_copy(to_cpumask(cpu_online_bits), src);
}
