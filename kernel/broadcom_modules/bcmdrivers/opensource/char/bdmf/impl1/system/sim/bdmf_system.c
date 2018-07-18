/*
* <:copyright-BRCM:2013:GPL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :> 
*/


#include <bdmf_system.h>

uint32_t jiffies = 0;

int bdmf_task_create(const char *name, int priority, int stack,
                int (*handler)(void *arg), void *arg, bdmf_task *ptask)
{
    int rc;
    rc = pthread_create(ptask, NULL, (void *(*)(void *))handler, arg);
    return rc ? BDMF_ERR_SYSCALL_ERR : 0;
}

int bdmf_task_destroy(bdmf_task task)
{
    int rc;
    void *res;
    pthread_cancel(task);
    rc = pthread_join(task, &res);
    return (rc || (res != PTHREAD_CANCELED)) ? BDMF_ERR_SYSCALL_ERR : 0;
}

/*
 * Shared memory mapping
 */
void *bdmf_mmap(const char *fname, uint32_t size)
{
    int fd;
    struct stat stat;
    void *map;
    int rc;

    fd = shm_open(fname, O_RDWR | O_CREAT, 0x1ff);
    if (fd <= 0)
    {
        bdmf_print_error("Failed to open shm file %s\n", fname);
        return NULL;
    }

    rc = fstat(fd, &stat);
    if (rc == -1)
    {
        close(fd);
        bdmf_print_error("Failed to fstat shm file %s\n", fname);
        return NULL;
    }

    if (stat.st_size < size)
    {
        /* stretch file */
        rc = lseek(fd, size-1, SEEK_SET);
        rc = (rc < 0) ? rc : write(fd, "", 1);
        if (rc == -1)
        {
            close(fd);
            bdmf_print_error("Failed to stretch shm file %s to %u bytes\n", fname, size);
            return NULL;
        }
    }

     /* Now the file is ready to be mmapped.
      */
     map = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
     close(fd);
     if (map == MAP_FAILED)
     {
         bdmf_print_error("Error mapping the file %s\n", fname);
         return NULL;
     }

     return map;
}

/*
 * Interrupt handling
 */
struct bdmf_irq
{
    f_bdmf_irq_cb cb;
    void *data;
};
static struct bdmf_irq bdmf_irq_handlers[BDMFSYS_IRQ__NUM_OF];

static void bdmf_irq_sigaction(int sig, siginfo_t *info, void *dummy)
{
    int irq;

#ifdef __CYGWIN__
    irq = info->si_sigval.sival_int;
#else
    irq = info->si_int;
#endif
    assert((unsigned)irq < BDMFSYS_IRQ__NUM_OF);
    //bdmf_print("%s: got irq%d\n", __FUNCTION__, irq);
    if (!bdmf_irq_handlers[irq].cb)
    {
        bdmf_print_error("irq%d is not connected\n", irq);
        return;
    }
    bdmf_irq_handlers[irq].cb(irq, bdmf_irq_handlers[irq].data);
}

int bdmf_irq_connect(int irq, f_bdmf_irq_cb cb, void *data)
{
    static int signal_connected;
    if ((unsigned)irq >= BDMFSYS_IRQ__NUM_OF)
        return BDMF_ERR_PARM;
    if (bdmf_irq_handlers[irq].cb)
        return BDMF_ERR_ALREADY;
    if (!signal_connected)
    {
        struct sigaction act;
        int rc;
        memset(&act, 0, sizeof(act));
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = bdmf_irq_sigaction;
        rc = sigaction(SIGUSR1, &act, NULL);
        if (rc)
            return BDMF_ERR_INTERNAL;
        signal_connected = 1;
    }
    bdmf_irq_handlers[irq].cb = cb;
    bdmf_irq_handlers[irq].data = data;
    bdmf_print("%s: connected irq%d\n", __FUNCTION__, irq);
    return 0;
}

int bdmf_irq_free(int irq, f_bdmf_irq_cb cb, void *data)
{
    if ((unsigned)irq >= BDMFSYS_IRQ__NUM_OF)
        return BDMF_ERR_PARM;
    if ((bdmf_irq_handlers[irq].cb != cb) ||
        (bdmf_irq_handlers[irq].data != data))
        return BDMF_ERR_NOT_CONNECTED;
    bdmf_irq_handlers[irq].cb = NULL;
    bdmf_irq_handlers[irq].data = NULL;
    return 0;
}

void bdmf_irq_raise(int irq)
{
    union sigval value;
    int pid = getpid();
    int rc;

again:
    value.sival_int = irq;
    rc = sigqueue(pid, SIGUSR1, value);
    if (rc)
    {
        bdmf_print_error("failed to raise irq%d on pid %d. rc=%d\n", irq, pid, rc);
        if (rc == ESRCH)
            goto again;
    }
}

/*
 * Recursive mutex support
 */

typedef struct {
    int initialized; /* Should overlap with 'initialized' member in struct bdmf_ta_mutex */
    pthread_t self;
    int count;
    pthread_mutex_t m;
} bdmf_sim_ta_mutex;

static pthread_mutex_t ta_mutex_lock = PTHREAD_MUTEX_INITIALIZER;

void bdmf_ta_mutex_init(bdmf_ta_mutex *pmutex)
{
    bdmf_sim_ta_mutex *tam = (bdmf_sim_ta_mutex *)pmutex;
    BUG_ON(sizeof(bdmf_sim_ta_mutex) > sizeof(bdmf_ta_mutex));
#ifdef __CYGWIN__
    tam->self = NULL;
#else
    tam->self = -1;
#endif
    tam->count = 0;
    pthread_mutex_init(&tam->m, NULL);
    tam->initialized = 1;
}

void bdmf_ta_mutex_delete(bdmf_ta_mutex *pmutex)
{
    bdmf_sim_ta_mutex *tam = (bdmf_sim_ta_mutex *)pmutex;
    pthread_mutex_destroy(&tam->m);
}

int bdmf_ta_mutex_lock(bdmf_ta_mutex *pmutex)
{
    bdmf_sim_ta_mutex *tam = (bdmf_sim_ta_mutex *)pmutex;

    if (!tam->initialized)
        bdmf_ta_mutex_init(pmutex);
    pthread_mutex_lock(&ta_mutex_lock);
    if (tam->self == pthread_self())
    {
        ++tam->count;
        pthread_mutex_unlock(&ta_mutex_lock);
        return 0;
    }
    pthread_mutex_unlock(&ta_mutex_lock);

    /* not-recurring request */
    pthread_mutex_lock(&tam->m);

    tam->self = pthread_self();
    tam->count = 1;

    return 0;
}

void bdmf_ta_mutex_unlock(bdmf_ta_mutex *pmutex)
{
    bdmf_sim_ta_mutex *tam = (bdmf_sim_ta_mutex *)pmutex;

    if (!tam->initialized)
        bdmf_ta_mutex_init(pmutex);
    BUG_ON(tam->self != pthread_self());
    BUG_ON(tam->count < 1);
    if (--tam->count == 0)
    {
#ifdef __CYGWIN__
        tam->self = NULL;
#else
        tam->self = -1;
#endif
        pthread_mutex_unlock(&tam->m);
    }
}

static uint32_t sysb_headroom[bdmf_sysb_type__num_of];

/** Set headroom size for system buffer
 * \param[in]   sysb_type   System buffer type
 * \param[in]   headroom    Headroom size
 */
void bdmf_sysb_headroom_size_set(bdmf_sysb_type sysb_type, uint32_t headroom)
{
    sysb_headroom[sysb_type] = headroom;
}

/** Allocate system buffer.
 * \param[in]   sysb_type   System buffer type
 * \param[in]   length      Data length
 * \return system buffer pointer.
 * If the function returns NULL, caller is responsible for "data" deallocation
 */
bdmf_sysb bdmf_sysb_alloc(bdmf_sysb_type sysb_type, uint32_t length)
{
    if (sysb_type == bdmf_sysb_skb)
    {
        struct sk_buff *skb = dev_alloc_skb(length + sysb_headroom[bdmf_sysb_skb]);
        if (!skb)
            return NULL;
        skb_reserve(skb, sysb_headroom[bdmf_sysb_skb]);
        return (bdmf_sysb)skb;
    }
    return NULL;
}

/** Initialize platform buffer support
 * \param[in]   size    buffer size
 * \param[in]   offset  min offset
 */
void bdmf_pbuf_init(uint32_t size, uint32_t offset)
{

}

/** Allocate pbuf and fill with data
 * The function allocates platform buffer and copies data into it
 * \param[in]   data        data pointer
 * \param[in]   length      data length
 * \param[in]   source      source port
 * \param[out]  pbuf        Platform buffer
 * \return 0 if OK or error < 0
 */
int bdmf_pbuf_alloc(void *data, uint32_t length, uint16_t source, bdmf_pbuf_t *pbuf)
{
    return BDMF_ERR_NOT_SUPPORTED;
}

/** Release pbuf
 * \param[in]   pbuf        Platform buffer
 * \return 0=OK, <0-error (sysb doesn't contain pbuf info)
 */
void bdmf_pbuf_free(bdmf_pbuf_t *pbuf)
{
    BUG();
}

/** Convert sysb to platform buffer
 * \param[in]   sysb        System buffer. Released in case of success
 * \param[in]   pbuf_source BPM source port as defined by RDD
 * \param[out]  pbuf        Platform buffer
 * \return 0=OK, <0-error (sysb doesn't contain pbuf info)
 */
int bdmf_pbuf_from_sysb(const bdmf_sysb sysb, uint16_t pbuf_source, bdmf_pbuf_t *pbuf)
{
    return BDMF_ERR_NOT_SUPPORTED;
}

/** Convert platform buffer to sysb
 * \param[in]   sysb_type   System buffer type
 * \param[in]   pbuf        Platform buffer. Released in case of success or becomes "owned" by sysb
 * \return sysb pointer or NULL
 */
bdmf_sysb bdmf_pbuf_to_sysb(bdmf_sysb_type sysb_type, bdmf_pbuf_t *pbuf)
{
    return NULL;
}

/** Determine if sysb contains pbuf
 * \patam[in]   sysb        System buffer
 * \return 1 if sysb contains pbuf
 */
int bdmf_sysb_is_pbuf(bdmf_sysb sysb)
{
    return 0;
}

/*
 * Timer support
 * Not implemented
 */

void bdmf_timer_init(bdmf_timer_t *timer, bdmf_timer_cb_t cb, unsigned long priv)
{

}

int bdmf_timer_start(bdmf_timer_t *timer, uint32_t ticks)
{
    return 0;
}

void bdmf_timer_stop(bdmf_timer_t *timer)
{

}

void bdmf_timer_delete(bdmf_timer_t *timer)
{

}

uint32_t bdmf_ms_to_ticks(uint32_t ms)
{
    return ms;
}
