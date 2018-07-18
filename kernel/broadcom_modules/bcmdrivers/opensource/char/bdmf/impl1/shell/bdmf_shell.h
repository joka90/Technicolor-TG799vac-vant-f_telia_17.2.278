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
 * bdmf_shell.h
 *
 * BL framework - shell API
 *
 *******************************************************************/

#ifndef BDMF_MON_H

#define BDMF_MON_H

#include <bdmf_system.h>
#include <bdmf_session.h>

/** \defgroup bdmf_mon Broadlight Monitor Module (CLI)
 * Broadlight Monitor is used for all configuration and status monitoring.\n
 * It doesn't have built-in scripting capabilities (logical expressions, loops),
 * but can be used in combination with any available scripting language.\n
 * Broadlight Monitor replaces Broadlight Shell and supports the following features:\n
 * - parameter number and type validation (simplifies command handlers development)
 * - parameter value range checking
 * - mandatory and optional parameters
 * - positional and named parameters
 * - parameters with default values
 * - enum parameters can have arbitrary values
 * - automatic command help generation
 * - automatic or user-defined command shortcuts
 * - command handlers return completion status to enable scripting
 * - multiple sessions
 * - session access rights
 * - extendible. Supports user-defined parameter types
 * - relatively low stack usage
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Monitor entry handle
 */
typedef struct bdmfmon_entry *bdmfmon_handle_t;

/* if BDMFMON_PARM_USERIO flag is set:
   low_val: t_userscanf_f function
   high_val: t_userprintf_f function
*/

/** Function parameter structure */
typedef struct bdmfmon_cmd_parm bdmfmon_cmd_parm_t;

/** Parameter type */
typedef enum
{
    BDMFMON_PARM_NONE,
    BDMFMON_PARM_DECIMAL,         /**< Decimal number */
    BDMFMON_PARM_DECIMAL64,       /**< Signed 64-bit decimal */
    BDMFMON_PARM_UDECIMAL,        /**< Unsigned decimal number */
    BDMFMON_PARM_UDECIMAL64,      /**< Unsigned 64-bit decimal number */
    BDMFMON_PARM_HEX,             /**< Hexadecimal number */
    BDMFMON_PARM_HEX64,           /**< 64-bit hexadecimal number */
    BDMFMON_PARM_NUMBER,          /**< Decimal number or hex number prefixed by 0x */
    BDMFMON_PARM_NUMBER64,        /**< 64bit decimal number or hex number prefixed by 0x */
    BDMFMON_PARM_STRING,          /**< String */
    BDMFMON_PARM_ENUM,            /**< Enumeration */
    BDMFMON_PARM_IP,              /**< IP address n.n.n.n */
    BDMFMON_PARM_IPV6,            /**< IPv6 address */
    BDMFMON_PARM_MAC,             /**< MAC address xx:xx:xx:xx:xx:xx */

    BDMFMON_PARM_USERDEF          /**< User-defined parameter. User must provide scan_cb */
} bdmfmon_parm_type_t;

/** Enum attribute value.
 *
 *  Enum values is an array of bdmfmon_enum_val_t terminated by element with name==NULL
 *
 */
typedef struct bdmfmon_enum_val
{
    const char *name;           /**< Enum symbolic name */
    long val;                   /**< Enum internal value */
} bdmfmon_enum_val_t;
#define BDMFMON_MAX_ENUM_VALUES   128     /**< Max number of enum values */
#define BDMFMON_ENUM_LAST     { NULL, 0}  /**< Last entry in enum table */

/** Boolean values (true/false, yes/no, on/off)
 *
 */
extern bdmfmon_enum_val_t bdmfmon_enum_bool_table[];

/* Monitor data types */
typedef long bdmfmon_number;      /**< Type underlying BDMFMON_PARM_NUMBER, BDMFMON_PARM_DECIMAL */
typedef long bdmfmon_unumber;     /**< Type underlying BDMFMON_PARM_HEX, BDMFMON_PARM_UDECIMAL */
typedef long bdmfmon_number64;    /**< Type underlying BDMFMON_PARM_NUMBER64, BDMFMON_PARM_DECIMAL64 */
typedef long bdmfmon_unumber64;   /**< Type underlying BDMFMON_PARM_HEX64, BDMFMON_PARM_UDECIMAL64 */

/** Parameter value */
typedef union bdmfmon_parm_value
{
    long number;                    /**< Signed number */
    unsigned long unumber;          /**< Unsigned number */
    long long number64;             /**< Signed 64-bit number */
    unsigned long long unumber64;   /**< Unsigned 64-bit number */
    char *string;                   /**< 0-terminated string */
    double d;                       /**< Double-precision floating point number */
    bdmfmon_enum_val_t *enum_table;   /**< Table containing { enum_name, enum_value } pairs */
    char mac[6];
    /* ToDo: add other types */
}  bdmfmon_parm_value_t;

/** User-defined scan function.
 * The function is used for parsing user-defined parameter types
 * Returns: 0-ok, <=error
 *
 */
typedef int (*bdmfmon_scan_cb_t)(bdmfmon_cmd_parm_t *parm, char *string_val);

/** User-defined print function.
 * The function is used for printing user-defined parameter types
 *
 */
typedef void (*bdmfmon_format_cb_t)(const bdmfmon_cmd_parm_t *parm, bdmfmon_parm_value_t value, char *buffer, int size);


/** Function parameter structure */
struct bdmfmon_cmd_parm
{
   const char *name;             /**< Parameter name. Shouldn't be allocated on stack! */
   const char *description;      /**< Parameter description. Shouldn't be allocated on stack! */
   bdmfmon_parm_type_t type;       /**< Parameter type */
   uint8_t flags;                /**< Combination of BDMFMON_PARM_xx flags */
#define BDMFMON_PARM_FLAG_OPTIONAL   0x01 /**< Parameter is optional */
#define BDMFMON_PARM_FLAG_DEFVAL     0x02 /**< Default value is set */
#define BDMFMON_PARM_FLAG_RANGE      0x04 /**< Range is set */
#define BDMFMON_PARM_FLAG_EOL        0x20 /**< String from the current parser position till EOL */
#define BDMFMON_PARM_FLAG_NOVAL      0x80 /**< Internal flag: parameter is anassigned */

   bdmfmon_parm_value_t low_val;   /**< Low val for range checking */
   bdmfmon_parm_value_t hi_val;    /**< Hi val for range checking */
   bdmfmon_parm_value_t value;     /**< Value */
   bdmfmon_scan_cb_t scan_cb;      /**< User-defined scan function for BDMFMON_PARM_USERDEF parameter type */
   bdmfmon_format_cb_t format_cb;  /**< User-defined format function for BDMFMON_PARM_USERDEF parameter type */
};

/** Command parameter list terminator */
#define BDMFMON_PARM_LIST_TERMINATOR  { NULL, NULL, BDMFMON_PARM_NONE, 0, {0}, {0}, {0}, NULL, NULL }

/** Helper macro: make simple parameter
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _type     Parameter type
 * \param[in] _flags    Parameter flags
 */
#define BDMFMON_MAKE_PARM(_name, _descr, _type, _flags) \
    { (_name), (_descr), (_type), (_flags), {0}, {0}, {0}, NULL, NULL }

/** Helper macro: make range parameter
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _type     Parameter type
 * \param[in] _flags    Parameter flags
 * \param[in] _min      Min value
 * \param[in] _max      Max value
 */
#define BDMFMON_MAKE_PARM_RANGE(_name, _descr, _type, _flags, _min, _max) \
    { (_name), (_descr), (_type), (_flags) | BDMFMON_PARM_FLAG_RANGE, {_min}, {_max}, {0}, NULL, NULL }

/** Helper macro: make parameter with default value
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _type     Parameter type
 * \param[in] _flags    Parameter flags
 * \param[in] _dft      Default value
 */
#define BDMFMON_MAKE_PARM_DEFVAL(_name, _descr, _type, _flags, _dft) \
    { (_name), (_descr), (_type), (_flags) | BDMFMON_PARM_FLAG_DEFVAL, {0}, {0}, {_dft}, NULL, NULL }

/** Helper macro: make range parameter with default value
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _type     Parameter type
 * \param[in] _flags    Parameter flags
 * \param[in] _min      Min value
 * \param[in] _max      Max value
 * \param[in] _dft      Default value
 */
#define BDMFMON_MAKE_PARM_RANGE_DEFVAL(_name, _descr, _type, _flags, _min, _max, _dft) \
    { (_name), (_descr), (_type), (_flags) | BDMFMON_PARM_FLAG_RANGE | BDMFMON_PARM_FLAG_DEFVAL, {_min}, {_max}, {_dft}, NULL, NULL }


/** Helper macro: make enum parameter
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _values   Enum values table
 * \param[in] _flags    Parameter flags
 */
#define BDMFMON_MAKE_PARM_ENUM(_name, _descr, _values, _flags) \
    { (_name), (_descr), BDMFMON_PARM_ENUM, (_flags), {.enum_table=(_values)}, {0}, {0}, NULL, NULL }


/** Helper macro: make enum parameter with default value
 * \param[in] _name     Parameter name
 * \param[in] _descr    Parameter description
 * \param[in] _values   Enum values table
 * \param[in] _flags    Parameter flags
 * \param[in] _dft      Default value
 */
#define BDMFMON_MAKE_PARM_ENUM_DEFVAL(_name, _descr, _values, _flags, _dft) \
    { (_name), (_descr), BDMFMON_PARM_ENUM, (_flags) | BDMFMON_PARM_FLAG_DEFVAL, {.enum_table=(_values)}, {0}, {.string=_dft}, NULL, NULL }


#define BDMFMON_MAKE_CMD_NOPARM(dir, cmd, help, cb) \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, NULL)

#define BDMFMON_MAKE_CMD(dir, cmd, help, cb, parms...)   \
{                                                           \
    static bdmfmon_cmd_parm_t cmd_parms[]={                 \
        parms,                                              \
        BDMFMON_PARM_LIST_TERMINATOR                        \
    };                                                      \
    bdmfmon_cmd_add(dir, cmd, cb, help, BDMF_ACCESS_ADMIN, NULL, cmd_parms); \
}

/** Optional custom directory handlers */
typedef void (*bdmfmon_dir_enter_leave_cb_t)(bdmf_session_handle session, bdmfmon_handle_t dir, int is_enter);

/** Optional command or directory help callback
 * \param[in]   session     Session handle
 * \param[in]   h           Command or directory handle
 * \param[in]   parms       Parameter(s) - the rest of the command string.
 *                          Can be used for example to get help on individual parameters
 */
typedef void (*bdmfmon_help_cb_t)(bdmf_session_handle session, bdmfmon_handle_t h, const char *parms);


/** Extra parameters of monitor directory.
 * See \ref bdmfmon_dir_add
 *
 */
typedef struct bdmfmon_dir_extra_parm
{
    void *user_priv;        /**< private data passed to enter_leave_cb */
    bdmfmon_dir_enter_leave_cb_t enter_leave_cb; /**< callback function to be called when session enters/leavs the directory */
    bdmfmon_help_cb_t help_cb;/**< Help function called to print directory help instead of the automatic help */
} bdmfmon_dir_extra_parm_t;


/** Extra parameters of monitor command.
 * See \ref bdmfmon_cmd_add
 *
 */
typedef struct bdmfmon_cmd_extra_parm
{
    bdmfmon_help_cb_t help_cb;    /**< Optional help callback. Can be used for more sophisticated help, e.g., help for specific parameters */
    uint32_t flags;             /**< Command flags */
#define BDMFMON_CMD_FLAG_NO_NAME_PARMS   0x00000001 /**< No named parms. Positional only. Can be useful if parameter value can contain ',' */
} bdmfmon_cmd_extra_parm_t;


/** Monitor command handler prototype */
typedef int (*bdmfmon_cmd_cb_t)(bdmf_session_handle session, const bdmfmon_cmd_parm_t parm[], uint16_t n_parms);


/** Add subdirectory to the parent directory
 *
 * \param[in]   parent          Parent directory handle. NULL=root
 * \param[in]   name            Directory name
 * \param[in]   help            Help string
 * \param[in]   access_right    Access rights
 * \param[in]   extras          Optional directory descriptor. Mustn't be allocated on the stack.
 * \return      new directory handle or NULL in case of failure
 */
bdmfmon_handle_t bdmfmon_dir_add(bdmfmon_handle_t parent, const char *name,
                             const char *help, bdmf_access_right_t access_right,
                             const bdmfmon_dir_extra_parm_t *extras);


/** Scan directory tree and look for directory named "name".
 * 
 * \param[in]   parent          Directory sub-tree root. NULL=root
 * \param[in]   name            Name of directory to be found
 * \return      directory handle if found or NULL if not found
 */
bdmfmon_handle_t bdmfmon_dir_find(bdmfmon_handle_t parent, const char *name );


/** Get token name
 * \param[in]   token           Directory or command token
 * \return      directory token name
 */
const char *bdmfmon_token_name(bdmfmon_handle_t token);


/** Add CLI command
 *
 * \param[in]   dir             Handle of directory to add command to. NULL=root
 * \param[in]   name            Command name
 * \param[in]   cmd_cb          Command handler
 * \param[in]   help            Help string
 * \param[in]   access_right    Access rights
 * \param[in]   extras          Optional extras
 * \param[in]   parms           Optional parameters array. Must not be allocated on the stack!
 *                              If parms!=NULL, the last parameter in the array must have name==NULL.
 * \return
 *      0   =OK\n
 *      <0  =error code
 */
int bdmfmon_cmd_add(bdmfmon_handle_t dir, const char *name, bdmfmon_cmd_cb_t cmd_cb,
                  const char *help, bdmf_access_right_t access_right,
                  const bdmfmon_cmd_extra_parm_t *extras, bdmfmon_cmd_parm_t parms[]);


/** Destroy token (command or directory)
 * \param[in]   token           Directory or command token. NULL=root
 */
void bdmfmon_token_destroy(bdmfmon_handle_t token);


/** Open monitor session
 *
 * Monitor supports multiple simultaneous sessions with different
 * access rights.
 * Note that there already is a default session with full administrative rights,
 * that takes input from stdin and outputs to stdout.
 * \param[in]   parm        Session parameters. Must not be allocated on the stack.
 * \param[out]  p_session   Session handle
 * \return
 *      0   =OK\n
 *      <0  =error code
 */
int bdmfmon_session_open(const bdmf_session_parm_t *parm, bdmf_session_handle *p_session);


/** Close monitor session.
 * \param[in]   session         Session handle
 */
void bdmfmon_session_close(bdmf_session_handle session );


/** Parse and execute input string.
 * input_string can contain multiple commands delimited by ';'
 * 
 * \param[in]   session         Session handle
 * \param[in]   input_string    String to be parsed
 * \return
 *      =0  - OK \n
 *      -EINVAL - parsing error\n
 *      other - return code - as returned from command handler.
 *            It is recommended to return -EINTR to interrupt monitor loop.
 */
int bdmfmon_parse(bdmf_session_handle session, char* input_string);

/** Stop monitor driver.
 * The function stops \ref bdmfmon_driver
 * \param[in]   session         Session handle
 */
void bdmfmon_stop(bdmf_session_handle session);

/** Returns 1 if monitor session is stopped
 * \param[in]   session         Session handle
 * \returns 1 if monitor session stopped by bdmfmon_stop()\n
 * 0 otherwise
 */
int bdmfmon_is_stopped(bdmf_session_handle session);

/** Get current directory for the session,
 * \param[in]   session         Session handle
 * \return      The current directory handle
 */
bdmfmon_handle_t bdmfmon_dir_get(bdmf_session_handle session );

/** Set current directory for the session.
 * \param[in]   session         Session handle
 * \param[in]   dir             Directory that should become current
 * \return
 *      =0  - OK
 *      <0  - error
 */
int bdmfmon_dir_set(bdmf_session_handle session, bdmfmon_handle_t dir);


/** Get parameter number given its name.
 * The function is intended for use by command handlers
 * \param[in]       session         Session handle
 * \param[in,out]   parm_name       Parameter name
 * \return
 *  >=0 - parameter number\n
 *  <0  - parameter with this name doesn't exist
 */
int bdmfmon_parm_number(bdmf_session_handle session, const char *parm_name);


/** Check if parameter is set
 * \param[in]       session         Session handle
 * \param[in]       parm_number     Parameter number
 * \return
 *  1 if parameter is set\n
 *  0 if parameter is not set or parm_number is invalid
 */
int bdmfmon_parm_is_set(bdmf_session_handle session, int parm_number);


/** Get enum's string value given its internal value
 * \param[in]       table           Enum table
 * \param[in]       value           Internal value
 * \return
 *      enum string value or NULL if internal value is invalid
 */
const char *bdmfmon_enum_stringval(const bdmfmon_enum_val_t table[], long value);


/** Get enum's parameter string value given its internal value
 * \param[in]       session         Session handle
 * \param[in]       parm_number     Parameter number
 * \param[in]       value           Internal value
 * \return
 *      enum string value or NULL if parameter is not enum or
 *      internal value is invalid
 */
const char *bdmfmon_enum_parm_stringval(bdmf_session_handle session, int parm_number, long value);

#ifdef __cplusplus
}
#endif

/** @} end bdmf_mon group */

#endif /* #ifndef BDMF_MON_H */
