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
 * bdmf_session.h
 *
 * BL framework - management session
 *
 *******************************************************************/

#ifndef BDMF_SESSION_H

#define BDMF_SESSION_H

#include <bdmf_system.h>
#include <stdarg.h>

/** \defgroup bdmf_session Management Session Control
 *
 * APIs in this header file allow to create/destroy management sessions.
 * Management session is characterized by its access level and also
 * input/output functions.
 * Management sessions allow managed entities in the system to communicate
 * with local or remote managers (e.g., local or remote shell or NMS)
 * @{
 */

/** Access rights */
typedef enum
{
    BDMF_ACCESS_GUEST,     /**< Guest. Doesn't have access to commands and directories registered with ADMIN rights */
    BDMF_ACCESS_ADMIN,     /**< Administrator: full access */
    BDMF_ACCESS_DEBUG,     /**< Administrator: full access + extended debug features */
} bdmf_access_right_t;

/** Session parameters structure.
 * See \ref bdmf_session_open
 */
typedef struct bdmf_session_parm
{
    const char *name;       /**< Session name */
    void *user_priv;        /**< Private user's data */

    /** Session's output function. NULL=use write(stdout)
     * returns the number of bytes written or <0 if error
     */
    int (*write)(void *user_priv, const void *buf, uint32_t size);

    /** Access rights */
    bdmf_access_right_t access_right;

    /** Extra data size to be allocated along with session control block.
     * The extra data is accessible using bdmf_session_data()  */
    uint32_t extra_size;
} bdmf_session_parm_t;


/** Management session handle
 */
typedef struct bdmf_session *bdmf_session_handle;


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
int bdmf_session_open(const bdmf_session_parm_t *parm, bdmf_session_handle *p_session);


/** Close management session.
 * \param[in]   session         Session handle
 */
void bdmf_session_close(bdmf_session_handle session);


/** Write function.
 * Write buffer to the current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   buf             output buffer
 * \param[in]   size            number of bytes to be written
 * \return
 *  >=0 - number of bytes written\n
 *  <0  - output error
 */
int bdmf_session_write(bdmf_session_handle session, const void *buf, uint32_t size);


/** Print function.
 * Prints in the context of current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   format          print format - as in printf
 */
void bdmf_session_print(bdmf_session_handle session, const char *format, ...)
#ifndef BDMF_SESSION_DISABLE_FORMAT_CHECK
__attribute__((format(printf, 2, 3)))
#endif
;


/** Print function.
 * Prints in the context of current session.
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   format          print format - as in printf
 * \param[in]   ap              parameters list. Undefined after the call
 */
void bdmf_session_vprint(bdmf_session_handle session, const char *format, va_list ap);

/** Print buffer in hexadecimal format
 * \param[in]   session         Session handle. NULL=use stdout
 * \param[in]   buffer          Buffer address
 * \param[in]   offset          Start offset in the buffer
 * \param[in]   count           Number of bytes to dump
 */
void bdmf_session_hexdump(bdmf_session_handle session, void *buffer, uint32_t offset, uint32_t count);

/** Get extra data associated with the session
 * \param[in]       session         Session handle. NULL=default session
 * \return extra_data pointer or NULL if there is no extra data
 */
void *bdmf_session_data(bdmf_session_handle session);


/** Get user_priv provided in session parameters when it was registered
 * \param[in]       session         Session handle. NULL=default session
 * \return usr_priv value
 */
void *bdmf_session_user_priv(bdmf_session_handle session);


/** Get session name
 * \param[in]       session         Session handle. NULL=use stdin
 * \return session name
 */
const char *bdmf_session_name(bdmf_session_handle session);


/** Get session access rights
 * \param[in]       session         Session handle. NULL=default debug session
 * \return session access right
 */
bdmf_access_right_t bdmf_session_access_right(bdmf_session_handle session);

/** @} end of bdmf_session group */

#endif /* #ifndef BDMF_SESSION_H */
