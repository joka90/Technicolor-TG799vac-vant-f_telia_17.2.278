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


/*******************************************************************
 * bdmf_session.c
 *
 * BL framework - management session control
 *
 *******************************************************************/

#include <stdarg.h>
#include <bdmf_session.h>
#include <bdmf_system.h>

#define BDMF_SESSION_OUTBUF_LEN   2048

/* Management session structure */
typedef struct bdmf_session
{
    struct bdmf_session *next;
    bdmf_session_parm_t parms;
    uint32_t magic;
#define BDMF_SESSION_MAGIC    (('s'<<24)|('e'<<16)|('s'<<8)|'s')
    char outbuf[BDMF_SESSION_OUTBUF_LEN];
} bdmf_session_t;


static DEFINE_BDMF_FASTLOCK(session_lock);
static bdmf_session_t *session_list;
static int session_module_initialized;

/** Initialize session management module
 * \return
 *      0   =OK\n
 *      <0  =error code
 */
static void bdmf_session_module_init(void)
{
    session_module_initialized = 1;
}

/** Open management session
 *
 * Multiple sessions with the same or different access rights can be opened simultaneously.
 *
 * Note that there already is a default session with full administrative rights,
 * that takes input from stdin and outputs to stdout.
 * This default session can be used by device drivers.
 *
 * \param[in]   parm        Session parameters
 * \param[out]  p_session   Session handle
 * \return
 *      0   =OK\n
 *      <0  =error code
 */
int bdmf_session_open(const bdmf_session_parm_t *parm, bdmf_session_handle *p_session)
{
    bdmf_session_t *session;
    bdmf_session_t **p_last_next;
    const char *name;
    int size;

    BUG_ON(!p_session);
    BUG_ON(!parm);
    if (!p_session || !parm)
        return BDMF_ERR_PARM;
    if (!session_module_initialized)
        bdmf_session_module_init();
    name = parm->name;
    if (!name)
        name = "*unnamed*";
    size = sizeof(bdmf_session_t) + strlen(name) + 1 + parm->extra_size;
    session=bdmf_calloc(size);
    if (!session)
        return BDMF_ERR_NOMEM;
    session->parms = *parm;
    session->parms.name = (char *)session + sizeof(bdmf_session_t) + parm->extra_size;
    strcpy((char *)session->parms.name, name);
    session->magic = BDMF_SESSION_MAGIC;

    bdmf_fastlock_lock(&session_lock);
    p_last_next = &session_list;
    while(*p_last_next)
        p_last_next = &((*p_last_next)->next);
    *p_last_next = session;
    bdmf_fastlock_unlock(&session_lock);

    *p_session = session;

    return 0;
}


/** Close management session.
 * \param[in]   session         Session handle
 */
void bdmf_session_close(bdmf_session_handle session)
{
    BUG_ON(session->magic != BDMF_SESSION_MAGIC);
    bdmf_fastlock_lock(&session_lock);
    if (session==session_list)
        session_list = session->next;
    else
    {
        bdmf_session_t *prev = session_list;
        while (prev && prev->next != session)
            prev = prev->next;
        if (prev)
            prev->next = session->next;
        else
        {
            bdmf_print("%s: can't find session\n", __FUNCTION__);
            goto exit;
        }
    }
    bdmf_free(session);
    
exit:
    bdmf_fastlock_unlock(&session_lock);
}


/** Default write callback function
 * write to stdout
 */
int _bdmf_session_write(void *user_priv, const void *buf, uint32_t size)
{
    uint32_t i;
    const char *cbuf=buf;
    for(i=0; i<size; i++)
        bdmf_print("%c", cbuf[i]);
    return size;
}


/** Write function.
 * Write buffer to the current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   buffer          output buffer
 * \param[in]   size            number of bytes to be written
 * \return
 *  >=0 - number of bytes written\n
 *  <0  - output error
 */
int bdmf_session_write(bdmf_session_handle session, const void *buf, uint32_t size)
{
    int (*write_cb)(void *user_priv, const void *buf, uint32_t size);
    void *user_priv = NULL;
    if (session && session->parms.write)
    {
        BUG_ON(session->magic != BDMF_SESSION_MAGIC);
        write_cb = session->parms.write;
        user_priv = session->parms.user_priv;
    }
    else
        write_cb = _bdmf_session_write;
    return write_cb(user_priv, buf, size);
}


/** Print function.
 * Prints in the context of current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   format          print format - as in printf
 * \param[in]   ap              parameters list. Undefined after the call
 */
void bdmf_session_vprint(bdmf_session_handle session, const char *format, va_list ap)
{
    if (session && session->parms.write)
    {
        BUG_ON(session->magic != BDMF_SESSION_MAGIC);
        vsnprintf(session->outbuf, sizeof(session->outbuf), format, ap);
        bdmf_session_write(session, session->outbuf, strlen(session->outbuf));
    }
    else
        bdmf_vprint(format, ap);
}


/** Print function.
 * Prints in the context of current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   format          print format - as in printf
 */
void bdmf_session_print(bdmf_session_handle session, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    bdmf_session_vprint(session, format, ap);
    va_end(ap);
}

/** Get user_priv provoded in session partameters when it was registered
 * \param[in]       session         Session handle. NULL=use stdin
 * \return usr_priv value
 */
void *bdmf_session_user_priv(bdmf_session_handle session)
{
    if (!session)
        return NULL;
    BUG_ON(session->magic != BDMF_SESSION_MAGIC);
    return session->parms.user_priv;
}


/** Get extra data associated with the session
 * \param[in]       session         Session handle. NULL=default session
 * \return extra_data pointer or NULL if there is no extra data
 */
void *bdmf_session_data(bdmf_session_handle session)
{
    if (!session)
        return NULL;
    BUG_ON(session->magic != BDMF_SESSION_MAGIC);
    if (session->parms.extra_size <= 0)
        return NULL;
    return (char *)session + sizeof(*session);
}


/** Get session namedata
 * \param[in]       session         Session handle. NULL=default session
 * \return session name
 */
const char *bdmf_session_name(bdmf_session_handle session)
{
    if (!session)
        return NULL;
    BUG_ON(session->magic != BDMF_SESSION_MAGIC);
    return session->parms.name;
}


/** Get session access righte
 * \param[in]       session         Session handle. NULL=default debug session
 * \return session access right
 */
bdmf_access_right_t bdmf_session_access_right(bdmf_session_handle session)
{
    if (!session)
        return BDMF_ACCESS_DEBUG;
    BUG_ON(session->magic != BDMF_SESSION_MAGIC);
    return session->parms.access_right;
}

/* HexPrint a single line */
#define BYTES_IN_LINE   16

#define b2a(c)  (isprint(c)?c:'.')

static void _hexprint1( bdmf_session_handle session, uint16_t o, uint8_t *p_data, uint16_t count )
{
    int  i;

    bdmf_session_print(session, "%04x: ", o);
    for( i=0; i<count; i++ )
    {
        bdmf_session_print(session, "%02x", p_data[i]);
        if (!((i+1)%4))
            bdmf_session_print(session, " ");
    }
    for( ; i<BYTES_IN_LINE; i++ )
    {
        if (!((i+1)%4))
            bdmf_session_print(session, "   ");
        else
            bdmf_session_print(session, "  ");
    }
    for( i=0; i<count; i++ )
        bdmf_session_print(session, "%c", b2a(p_data[i]));
    bdmf_session_print(session, "\n");
}

/** Print buffer in hexadecimal format
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   buffer          Buffer address
 * \param[in]   offset          Start offset in the buffer
 * \param[in]   count           Number of bytes to dump
 */
void bdmf_session_hexdump(bdmf_session_handle session, void *buffer, uint32_t offset, uint32_t count)
{
    uint8_t *p_data = buffer;
    uint16_t n;
    while( count )
    {
        n = (count > BYTES_IN_LINE) ? BYTES_IN_LINE : count;
        _hexprint1(session, offset, p_data, n );
        count -= n;
        p_data += n;
        offset += n;
    }
}


/*
 * Exports
 */
EXPORT_SYMBOL(bdmf_session_open);
EXPORT_SYMBOL(bdmf_session_close);
EXPORT_SYMBOL(bdmf_session_write);
EXPORT_SYMBOL(bdmf_session_vprint);
EXPORT_SYMBOL(bdmf_session_print);
EXPORT_SYMBOL(bdmf_session_access_right);
EXPORT_SYMBOL(bdmf_session_data);
EXPORT_SYMBOL(bdmf_session_name);
EXPORT_SYMBOL(bdmf_session_hexdump);
