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
 * bdmf_mon.c
 *
 * BL framework - monitor
 *
 *******************************************************************/
#include <stdarg.h>
#include <bdmf_system.h>
#include <bdmf_shell.h>

#define BDMFMON_MAX_QUAL_NAME_LENGTH 256
#define BDMFMON_MAX_PARMS          32  
#define BDMFMON_MAX_NAME_LEN       20
#define BDMFMON_TOKEN_UP           ".."
#define BDMFMON_TOKEN_ROOT         "/"
#define BDMFMON_TOKEN_COMMENT      '#'
#define BDMFMON_TOKEN_HELP         "?"
#define BDMFMON_ROOT_HELP          "root directory"
#define BDMFMON_NAME_VAL_DELIMITER ':'

typedef enum { BDMFMON_ENTRY_DIR, BDMFMON_ENTRY_CMD } bdmfmon_entry_selector_t;

/* External table - boolean values */
bdmfmon_enum_val_t bdmfmon_enum_bool_table[] = {
    { .name="true", .val=1},
    { .name="yes", .val=1},
    { .name="on", .val=1},
    { .name="false", .val=0},
    { .name="no", .val=0},
    { .name="off", .val=0},
    BDMFMON_ENUM_LAST
};

/* Monitor token structure */
typedef struct bdmfmon_entry
{
    struct bdmfmon_entry  *next;
    char *name;                                  /* Command/directory name */
    char *help;                                  /* Command/directory help */
    bdmfmon_entry_selector_t sel;                  /* Entry selector */
    char *alias;                                 /* Alias */
    uint16_t alias_len;                          /* Alias length */
    struct bdmfmon_entry *parent;                  /* Parent directory */
    bdmf_access_right_t access_right;

    union {
        struct
        {
            struct bdmfmon_entry *first;           /* First entry in directory */
            const bdmfmon_dir_extra_parm_t *extras;  /* Optional extras */
        } dir;
        struct
        {
            bdmfmon_cmd_cb_t cmd_cb;               /* Command callback */
            bdmfmon_cmd_parm_t *parms;             /* Command parameters */
            const bdmfmon_cmd_extra_parm_t *extras;  /* Optional extras */
        } cmd;
    } u;
} bdmfmon_entry_t;


/* Token types */
typedef enum
{
    BDMFMON_BDMFMON_TOKEN_EMPTY,
    BDMFMON_BDMFMON_TOKEN_UP,
    BDMFMON_BDMFMON_TOKEN_ROOT,
    BDMFMON_BDMFMON_TOKEN_BREAK,
    BDMFMON_BDMFMON_TOKEN_HELP,
    BDMFMON_BDMFMON_TOKEN_NAME
} bdmfmon_token_type_t;


/* Monitor session structure */
typedef struct bdmfmon_session
{
    struct bdmfmon_session *next;
    bdmfmon_entry_t *curdir;
    bdmf_access_right_t access_right;
    bdmf_session_handle session;
    bdmfmon_entry_t *curcmd;
    bdmfmon_cmd_parm_t cmd_parms[BDMFMON_MAX_PARMS];
    char *p_inbuf;
    int stop_monitor;
} bdmfmon_session_t;

static bdmfmon_entry_t    *bdmfmon_root_dir;
static bdmfmon_session_t  *bdmfmon_root_session;

#define BDMFMON_MIN_NAME_LENGTH_FOR_ALIAS   3
#define BDMFMON_ROOT_NAME       "/"

/* Internal functions */
static void        _bdmfmon_alloc_root( void );
static void        _bdmfmon_display_dir( bdmfmon_session_t *session, bdmfmon_entry_t *p_dir );
static bdmfmon_token_type_t _bdmfmon_get_word(bdmfmon_session_t *session, char **p_word, int skipeol);
static bdmfmon_token_type_t _bdmfmon_analize_token( char *name );
static uint16_t    _bdmfmon_get_n_of_parms( bdmfmon_entry_t *p_token );
static int         _bdmfmon_parse_parms( bdmfmon_session_t *session, bdmfmon_entry_t *p_token, uint16_t *pn_parms );
static bdmfmon_entry_t *_bdmfmon_search_token( bdmfmon_entry_t *p_dir, char *name );
static void        _bdmfmon_dir_help( bdmfmon_session_t *session, bdmfmon_entry_t *p_dir );
static void        _bdmfmon_display_help( bdmfmon_session_t *session, bdmfmon_entry_t *p_token );
static void        _bdmfmon_choose_alias( bdmfmon_entry_t *p_dir, bdmfmon_entry_t *p_new_token );
static char       *_bdmfmon_strlwr( char *s );
static int         _bdmfmon_stricmp( const char *s1, const char *s2, int len );
static int         _check_named_parm(bdmfmon_session_t *session, bdmfmon_entry_t *entry, char **parm, int *n_parm);
static int         _bdmfmon_dft_scan_cb(bdmfmon_cmd_parm_t *parm, char *string_val);
static const char *_bdmfmon_get_type_name(bdmfmon_parm_type_t type);
static void        _bdmfmon_dft_format_cb(const bdmfmon_cmd_parm_t *parm, bdmfmon_parm_value_t value, char *buffer, int size);
static int         _bdmfmon_scan_enum_cb(bdmfmon_cmd_parm_t *parm, char *string_val);
static void        _bdmfmon_format_enum_cb(const bdmfmon_cmd_parm_t *parm, bdmfmon_parm_value_t value, char *buffer, int size);
static const char *_bdmfmon_qualified_name( bdmfmon_entry_t *token, char *buffer, int size);


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
                             const bdmfmon_dir_extra_parm_t *extras)
{
    bdmfmon_entry_t *p_dir;
    bdmfmon_entry_t **p_e;

    assert(name);
    assert(help);
    if (!name || !help)
        return NULL;

    if (!bdmfmon_root_dir)
    {
        _bdmfmon_alloc_root( );
        if (!bdmfmon_root_dir)
            return NULL;
    }

    if (!parent)
        parent = bdmfmon_root_dir;

    p_dir=(bdmfmon_entry_t *)bdmf_calloc( sizeof(bdmfmon_entry_t) + strlen(name) + strlen(help) + 2 );
    if ( !p_dir )
        return NULL;

    p_dir->name = (char *)(p_dir + 1);
    strcpy( p_dir->name, name);
    p_dir->help = p_dir->name + strlen(name) + 1;
    strcpy(p_dir->help, help);
    p_dir->sel = BDMFMON_ENTRY_DIR;
    _bdmfmon_choose_alias( parent, p_dir );
    p_dir->access_right = access_right;
    p_dir->u.dir.extras = extras;

    /* Add new directory to the parent's list */
    p_dir->parent = parent;
    p_e = &(parent->u.dir.first);
    while (*p_e)
        p_e = &((*p_e)->next);
    *p_e = p_dir;

    return p_dir;
}

static bdmfmon_entry_t * find_entry_in_dir( bdmfmon_entry_t *dir, const char *name,
        bdmfmon_entry_selector_t type, uint16_t recursive_search)
{
    bdmfmon_entry_t *p1, *p;

    if ( !dir )
    {
        dir = bdmfmon_root_dir;
        if (!dir)
            return NULL;
    }
    p = dir->u.dir.first;
    while (p)
    {
        if ( !_bdmfmon_stricmp(p->name, (char *) name, strlen(name)) && type == p->sel )
            return p;
        if ( recursive_search && p->sel == BDMFMON_ENTRY_DIR )
        {
            p1 = find_entry_in_dir(p, name , type, 1 );
            if ( p1 )
                return p1;
        }
        p = p->next;
    }
    return NULL;
}


/* Scan directory tree and look for directory with name starts from
 * root directory with name root_name
 */
bdmfmon_handle_t bdmfmon_dir_find(bdmfmon_handle_t parent, const char  *name)
{
    if ( !parent )
        parent = bdmfmon_root_dir;
    return find_entry_in_dir(parent, name, BDMFMON_ENTRY_DIR, 0 );
}





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
                  const bdmfmon_cmd_extra_parm_t *extras, bdmfmon_cmd_parm_t parms[])
{
    bdmfmon_entry_t *p_token;
    bdmfmon_entry_t **p_e;
    uint16_t       i;

    assert(name);
    assert(help);
    assert(cmd_cb);
    if (!name || !cmd_cb || !help)
        return BDMF_ERR_PARM;

    if (!bdmfmon_root_dir)
    {
        _bdmfmon_alloc_root( );
        if (!bdmfmon_root_dir)
            return BDMF_ERR_NOMEM;
    }

    if (!dir)
        dir = bdmfmon_root_dir;

    p_token=(bdmfmon_entry_t *)bdmf_calloc( sizeof(bdmfmon_entry_t) + strlen(name) + strlen(help) + 2 );
    if ( !p_token )
        return BDMF_ERR_NOMEM;

    /* Copy name */
    p_token->name = (char *)(p_token + 1);
    strcpy( p_token->name, name );
    p_token->help = p_token->name + strlen(name) + 1;
    strcpy(p_token->help, help);
    p_token->sel = BDMFMON_ENTRY_CMD;
    p_token->u.cmd.cmd_cb = cmd_cb;
    p_token->u.cmd.parms = parms;
    p_token->u.cmd.extras = extras;
    p_token->access_right = access_right;

    /* Convert name to lower case and choose alias */
    _bdmfmon_choose_alias(dir, p_token );


    /* Check parameters */
    i = 0;
    if ( parms )
    {
        while ( parms->name && parms->name[0] && ( i < BDMFMON_MAX_PARMS) )
        {
            /* Pointer parameter must have an address */
            if ((parms->type==BDMFMON_PARM_USERDEF) && !parms->scan_cb)
            {
                bdmf_print("MON: %s> scan_cb callback must be set for user-defined parameter %s\n", name, parms->name);
                goto cmd_add_error;
            }
            if (parms->type==BDMFMON_PARM_ENUM)
            {
                if (!parms->low_val.enum_table)
                {
                    bdmf_print("MON: %s> value table must be set in low_val for enum parameter %s\n", name, parms->name);
                    goto cmd_add_error;
                }
                /* Check default value if any */
                if ((parms->flags & BDMFMON_PARM_FLAG_DEFVAL))
                {
                    if (_bdmfmon_scan_enum_cb(parms, parms->value.string) < 0)
                    {
                        bdmf_print("MON: %s> default value %s doesn't match any value of enum parameter %s\n", name, parms->value.string, parms->name);
                        goto cmd_add_error;
                    }
                }
                parms->scan_cb = _bdmfmon_scan_enum_cb;
                parms->format_cb = _bdmfmon_format_enum_cb;
            }
            if (!parms->scan_cb)
                parms->scan_cb = _bdmfmon_dft_scan_cb;
            if (!parms->format_cb)
                parms->format_cb = _bdmfmon_dft_format_cb;
            ++parms;
            ++i;
        }
        if ((i == BDMFMON_MAX_PARMS) && parms->name[0])
        {
            bdmf_print("MON: %s> too many parameters\n", name);
            goto cmd_add_error;
        }
    }

    /* Add token to the directory */
    p_token->parent = dir;
    p_e = &(dir->u.dir.first);
    while (*p_e)
        p_e = &((*p_e)->next);
    *p_e = p_token;

    return 0;

cmd_add_error:
    bdmf_free( p_token );
    return BDMF_ERR_PARM;
}


/** Destroy token (command or directory)
 * \param[in]   token           Directory or command token. NULL=root
 */
void bdmfmon_token_destroy(bdmfmon_handle_t token)
{
    if (!token)
    {
        if (!bdmfmon_root_dir)
            return;
        token = bdmfmon_root_dir;
    }
    /* Remove from parent's list */
    if (token->parent)
    {
        bdmfmon_entry_t **p_e;
        p_e = &(token->parent->u.dir.first);
        while (*p_e)
        {
            if (*p_e == token)
            {
                *p_e = token->next;
                break;
            }
            p_e = &((*p_e)->next);
        }
    }

    /* Remove all directory entries */
    if (token->sel == BDMFMON_ENTRY_DIR)
    {
        bdmfmon_entry_t *e = token->u.dir.first;
        while((e = token->u.dir.first))
            bdmfmon_token_destroy(e);
    }

    /* Release the token */
    bdmf_free(token);

    if (token == bdmfmon_root_dir)
    {
        bdmfmon_root_dir = NULL;
        if (bdmfmon_root_session && bdmfmon_root_session->session)
        {
            bdmf_session_close(bdmfmon_root_session->session);
            bdmfmon_root_session = NULL;
        }
    }
}

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
int bdmfmon_session_open(const bdmf_session_parm_t *parm,
                       bdmf_session_handle *p_session)
{
    bdmf_session_handle session;
    bdmfmon_session_t *mon_session;
    bdmfmon_session_t *last;
    bdmf_session_parm_t session_parms;
    int rc;

    assert(p_session);
    if (!p_session)
        return BDMF_ERR_PARM;
    
    if (!bdmfmon_root_dir)
    {
        _bdmfmon_alloc_root( );
        if (!bdmfmon_root_dir)
            return BDMF_ERR_NOMEM;
    }
    if (parm)
        session_parms = *parm;
    else
    {
        memset(&session_parms, 0, sizeof(session_parms));
        session_parms.name = "unnamed";
    }

    /* Open comm session */
    session_parms.extra_size = sizeof(bdmfmon_session_t);
    rc = bdmf_session_open(&session_parms, &session);
    if (rc)
        return rc;
    mon_session = bdmf_session_data(session);
    mon_session->session = session;
    mon_session->access_right = session_parms.access_right;
    mon_session->curdir = bdmfmon_root_dir;

    /* Add to session list */
    last = bdmfmon_root_session;
    while(last->next)
        last = last->next;
    last->next = mon_session;

    *p_session = session;

    return 0;
}


/** Close monitor session.
 * \param[in]   session         Session handle
 */
void bdmfmon_session_close(bdmf_session_handle session )
{
    bdmfmon_session_t *prev = bdmfmon_root_session;
    while(prev && prev->next->session!=session)
        prev = prev->next;
    if (prev)
    {
        bdmfmon_session_t *mon_session = bdmf_session_data(session);
        assert(mon_session==prev->next);
        prev->next = mon_session->next;
        bdmf_session_close(session);
    }
}

static inline bdmfmon_session_t *bdmfmon_session(bdmf_session_handle session)
{
    bdmfmon_session_t *mon_session=bdmf_session_data(session);
    if (!mon_session)
        mon_session = bdmfmon_root_session;
    return mon_session;
}


/** Parse and execute input string.
 * input_string can contain multiple commands delimited by ';'
 *
 * \param[in]   session         Session handle
 * \param[in]   input_string    String to be parsed
 * \return
 *      =0  - OK \n
 *      BDMF_ERR_PARM - parsing error\n
 *      other - return code - as returned from command handler.
 *            It is recommended to return -EINTR to interrupt monitor loop.
 */
int bdmfmon_parse(bdmf_session_handle session, char* input_string)
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    bdmfmon_entry_t  *p_token;
    char *name;
    bdmfmon_token_type_t token_type;
    uint16_t n_parms;
    int rc = 0;

    if (!mon_session || !mon_session->curdir)
        return BDMF_ERR_PARM;
    
    if (input_string[strlen(input_string)-1]=='\n')
        input_string[strlen(input_string)-1]=0;
    if (input_string[strlen(input_string)-1]=='\r')
        input_string[strlen(input_string)-1]=0;
    mon_session->p_inbuf = input_string;
    mon_session->stop_monitor = 0;

    /* Interpret empty string as "display directory" */
    if ( mon_session->p_inbuf && !mon_session->p_inbuf[0] )
    {
        _bdmfmon_display_dir( mon_session, mon_session->curdir );
        return 0;
    }

    while (!mon_session->stop_monitor && mon_session->p_inbuf && mon_session->p_inbuf[0] && mon_session->p_inbuf[0]!='\n')
    {
        token_type = _bdmfmon_get_word(mon_session, &name, 1);
        switch ( token_type )
        {
            
        case BDMFMON_BDMFMON_TOKEN_NAME:
            p_token = _bdmfmon_search_token( mon_session->curdir, name );
            if (p_token == NULL)
            {
                bdmf_session_print(session, "**Error**\n");
                mon_session->p_inbuf = NULL;
                rc = BDMF_ERR_PARM;
            }
            else if (p_token->sel == BDMFMON_ENTRY_DIR)
            {
                mon_session->curdir = p_token;
                if (! mon_session->p_inbuf[0] || mon_session->p_inbuf[0]=='\n')
                    _bdmfmon_display_dir( mon_session, mon_session->curdir );
            }
            else
            {
                /* Function token */
                mon_session->curcmd = p_token;
                if (_bdmfmon_parse_parms( mon_session, p_token, &n_parms ) < 0)
                {
                    _bdmfmon_display_help( mon_session, p_token );
                    rc = BDMF_ERR_PARM;
                }
                else
                {
                    rc = p_token->u.cmd.cmd_cb(session, mon_session->cmd_parms, n_parms );
                    if (rc)
                    {
                        char buffer[BDMFMON_MAX_QUAL_NAME_LENGTH];
                        bdmf_session_print(session, "MON: %s> failed with error code %s(%d)\n",
                            _bdmfmon_qualified_name(p_token, buffer, sizeof(buffer)),
                                         bdmf_strerror(rc), rc);
                    }
                }
            }
            break;

        case BDMFMON_BDMFMON_TOKEN_UP: /* Go to upper directory */
            if (mon_session->curdir->parent)
                mon_session->curdir = mon_session->curdir->parent;
            if (!mon_session->p_inbuf[0] || mon_session->p_inbuf[0]=='\n')
                _bdmfmon_display_dir( mon_session, mon_session->curdir );
            break;

        case BDMFMON_BDMFMON_TOKEN_ROOT: /* Go to the root directory */
            mon_session->curdir = bdmfmon_root_dir;
            if (!mon_session->p_inbuf[0] || mon_session->p_inbuf[0]=='\n')
                _bdmfmon_display_dir( mon_session, mon_session->curdir );
            break;

        case BDMFMON_BDMFMON_TOKEN_HELP: /* Display help */
            if (( _bdmfmon_get_word(mon_session, &name, 0) == BDMFMON_BDMFMON_TOKEN_NAME) &&
                    ((p_token = _bdmfmon_search_token( mon_session->curdir, name )) != NULL ))
                _bdmfmon_display_help( mon_session, p_token );
            else
                _bdmfmon_dir_help( mon_session, mon_session->curdir );
            break;

        case BDMFMON_BDMFMON_TOKEN_BREAK: /* Clear buffer */
            mon_session->p_inbuf = NULL;
            break;

        case BDMFMON_BDMFMON_TOKEN_EMPTY:
            break;

        }
    }

    return rc;
}


/* Stop monitor driver */
void bdmfmon_stop( bdmf_session_handle session )
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    assert(mon_session);
    mon_session->stop_monitor = 1;
}

/** Returns 1 if monitor session is stopped
 * \param[in]   session         Session handle
 * \returns 1 if monitor session stopped by bdmfmon_stop()\n
 * 0 otherwise
 */
int bdmfmon_is_stopped(bdmf_session_handle session)
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    return mon_session->stop_monitor;
}


/** Get parameter number given its name.
 * The function is intended for use by command handlers
 * \param[in]       session         Session handle
 * \param[in,out]   parm_name       Parameter name
 * \return
 *  >=0 - parameter number\n
 *  <0  - parameter with this name doesn't exist
 */
int bdmfmon_parm_number(bdmf_session_handle session, const char *parm_name)
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    int i;
    if (!parm_name < 0 || !mon_session || !mon_session->curcmd)
        return BDMF_ERR_PARM;
    for(i=0;
        mon_session->cmd_parms[i].name &&
            _bdmfmon_stricmp( parm_name, mon_session->cmd_parms[i].name, -1);
        i++)
        ;
    if (!mon_session->cmd_parms[i].name)
        return BDMF_ERR_PARM;
    return i;
}


/** Check if parameter is set
 * \param[in]       session         Session handle
 * \param[in]       parm_number     Parameter number
 * \return
 *  1 if parameter is set\n
 *  0 if parameter is not set or parm_number is invalid
 */
int bdmfmon_parm_is_set(bdmf_session_handle session, int parm_number)
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    int i;
    if (parm_number < 0 || !mon_session || !mon_session->curcmd)
        return 0;
    for(i=0; i<parm_number && mon_session->cmd_parms[i].name; i++)
        ;
    if (i < parm_number)
        return 0;
    return ((mon_session->cmd_parms[parm_number].flags & BDMFMON_PARM_FLAG_NOVAL)==0);
}



/** Get enum's string value given its internal value
 * \param[in]       table           Enum table
 * \param[in]       value           Internal value
 * \return
 *      enum string value or NULL if internal value is invalid
 */
const char *bdmfmon_enum_stringval(const bdmfmon_enum_val_t table[], long value)
{
    while(table->name)
    {
        if (table->val==value)
            return table->name;
        ++table;
    }
    return NULL;
}


/** Get enum's string value given its internal value
 * \param[in]       session         Session handle
 * \param[in]       parm_number     Parameter number
 * \param[in]       value           Internal value
 * \return
 *      enum string value or NULL if parameter is not enum or
 *      internal value is invalid
 */
const char *bdmfmon_enum_parm_stringval(bdmf_session_handle session, int parm_number, long value)
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    int i;
    bdmfmon_enum_val_t *values;
    if (parm_number < 0 || !mon_session || !mon_session->curcmd)
        return NULL;
    for(i=0; i<parm_number && mon_session->cmd_parms[i].name; i++)
        ;
    if (i < parm_number)
        return NULL;
    if (mon_session->cmd_parms[parm_number].type != BDMFMON_PARM_ENUM)
        return NULL;
    values = mon_session->cmd_parms[parm_number].low_val.enum_table;
    return bdmfmon_enum_stringval(values, value);
}


/* Get current directory handle */
bdmfmon_handle_t bdmfmon_dir_get( bdmf_session_handle session )
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    if (!mon_session)
        return NULL;
    return mon_session->curdir;
}

/* Set current directory */
int bdmfmon_dir_set( bdmf_session_handle session, bdmfmon_handle_t dir )
{
    bdmfmon_session_t *mon_session=bdmfmon_session(session);
    assert(mon_session);
    if (!mon_session)
        return BDMF_ERR_PARM;
    /* Check access rights */
    if (!dir)
        dir = bdmfmon_root_dir;
    if (dir->access_right > mon_session->access_right)
        return BDMF_ERR_PERM;
    mon_session->curdir = dir;
    return 0;
}

/** Get token name
 * \param[in]   token           Directory or command token
 * \return      directory token name
 */
const char *bdmfmon_token_name(bdmfmon_handle_t token)
{
    if (!token)
        return NULL;
    return token->name;
}

/*********************************************************/
/* Internal functions                                    */
/*********************************************************/

/* Allocate root directory and default session */
void _bdmfmon_alloc_root( void )
{
    bdmf_session_parm_t session_parms;
    bdmf_session_handle session;
    int rc;
    
    /* The very first call. Allocate root structure */
    if ((bdmfmon_root_dir=(bdmfmon_entry_t *)bdmf_calloc(sizeof(bdmfmon_entry_t) + strlen(BDMFMON_ROOT_HELP) + 2 )) == NULL)
        return;
    bdmfmon_root_dir->name = (char *)(bdmfmon_root_dir + 1);
    bdmfmon_root_dir->help = (char *)(bdmfmon_root_dir->name + 1);
    strcpy(bdmfmon_root_dir->help, BDMFMON_ROOT_HELP);
    bdmfmon_root_dir->sel = BDMFMON_ENTRY_DIR;
    bdmfmon_root_dir->access_right = BDMF_ACCESS_GUEST;

    memset(&session_parms, 0, sizeof(session_parms));
    session_parms.access_right = BDMF_ACCESS_ADMIN;
    session_parms.extra_size = sizeof(bdmfmon_session_t);
    session_parms.name = "monroot";
    rc = bdmf_session_open(&session_parms, &session);
    if (rc)
    {
        bdmf_free(bdmfmon_root_dir);
        bdmfmon_root_dir = NULL;
        bdmfmon_root_session = NULL;
        return;
    }
    bdmfmon_root_session = bdmf_session_data(session);
    bdmfmon_root_session->session = session;
    bdmfmon_root_session->access_right = BDMF_ACCESS_ADMIN;
    bdmfmon_root_session->curdir = bdmfmon_root_dir;
}

/* Display directory */
void _bdmfmon_display_dir( bdmfmon_session_t *session, bdmfmon_entry_t *p_dir )
{
    bdmfmon_entry_t *p_token;
    bdmfmon_entry_t *prev=NULL;

    bdmf_session_print(session->session, "%s%s> ", (p_dir==bdmfmon_root_dir)?"":".../", p_dir->name );
    p_token = p_dir->u.dir.first;
    while ( p_token )
    {
        if (p_token->access_right <= session->access_right)
        {
            if (prev)
                bdmf_session_print(session->session, ", ");
            bdmf_session_print(session->session, "%s", p_token->name );
            if (p_token->sel == BDMFMON_ENTRY_DIR )
                bdmf_session_print(session->session, "/");
            prev = p_token;
        }
        p_token = p_token->next;
    }
    bdmf_session_print(session->session, "\n");
}


/* Cut the first word from <p_inbuf>.
 * - Return pointer to start of the word in p_word
 * - 0 terminator is inserted in the end of the word
 * - session->p_inbuf is updated to point after the word
 * Returns token type
 */
bdmfmon_token_type_t _bdmfmon_get_word(bdmfmon_session_t *session, char **p_word, int skipeol)
{
    bdmfmon_token_type_t token_type;
    char *p_inbuf = session->p_inbuf;

    if (*p_inbuf==';' && skipeol)
        ++p_inbuf;

    /* Skip leading blanks */
    while (*p_inbuf && isspace(*p_inbuf))
        ++p_inbuf;

    /* Quoted string ? */
    if (*p_inbuf == '"')
    {
        *p_word = ++p_inbuf;

        while ( *p_inbuf && *p_inbuf!='"' )
            ++p_inbuf;
        if (*p_inbuf != '"')
        {
            bdmf_session_print(session->session, "MON: unterminated string %s\n", *p_word);
            return BDMFMON_BDMFMON_TOKEN_EMPTY;
        }
    }
    else
    {
        *p_word = p_inbuf;
        while ( *p_inbuf && !isspace(*p_inbuf) && *p_inbuf!=';' )
            ++p_inbuf;
    }
    if (*p_inbuf)
        *(p_inbuf++) = 0;
    session->p_inbuf = p_inbuf;
    token_type   = _bdmfmon_analize_token( *p_word );
    return token_type;
}


/* Make a preliminary analizis of <name> token.
 *   Returns a token type (Empty, Up, Root, Break, Name)
 */
bdmfmon_token_type_t _bdmfmon_analize_token( char *name )
{
    if (!name[0])
        return BDMFMON_BDMFMON_TOKEN_EMPTY;

    if (name[0]==BDMFMON_TOKEN_COMMENT)
        return BDMFMON_BDMFMON_TOKEN_BREAK;

    if (!strcmp( name, BDMFMON_TOKEN_UP ) )
        return BDMFMON_BDMFMON_TOKEN_UP;

    if (!strcmp( name, BDMFMON_TOKEN_ROOT ) )
        return BDMFMON_BDMFMON_TOKEN_ROOT;

    if (!strcmp( name, BDMFMON_TOKEN_HELP ) )
        return BDMFMON_BDMFMON_TOKEN_HELP;

    return BDMFMON_BDMFMON_TOKEN_NAME;

}


/* Returns number of parameters of the given token
 */
static uint16_t _bdmfmon_get_n_of_parms( bdmfmon_entry_t *p_token )
{
    uint16_t i;
    if ( !p_token->u.cmd.parms )
        return 0;
    for ( i=0;
            (i<BDMFMON_MAX_PARMS-1) &&
            p_token->u.cmd.parms[i].name &&
            p_token->u.cmd.parms[i].name[0];
            i++ )
        ;
    return i;
}

/* Check if parameter *parm is named (has format name=value)
 * if yes - identify named parameter and update *parm to point to the value
 * returns:
 * <0 - error
 * =0 - not named
 *  1 - named
 */
static int _check_named_parm(bdmfmon_session_t *session, bdmfmon_entry_t *entry, char **p_parm, int *n_parm)
{
    char *name = *p_parm;
    char *parm = *p_parm;
    bdmfmon_cmd_parm_t *cmd_parm = entry->u.cmd.parms;

    /* Stop here if command doesn't support named parameters */
    if (entry->u.cmd.extras &&
        (entry->u.cmd.extras->flags & BDMFMON_CMD_FLAG_NO_NAME_PARMS) )
        return 0;

    while(*parm && *parm!=BDMFMON_NAME_VAL_DELIMITER && (isalnum(*parm) || *parm == '_'))
        ++parm;
    if (*parm != BDMFMON_NAME_VAL_DELIMITER)
        return 0;   /* not named */

    /* Named. Try to identify name */
    *(parm++) = 0;
    if (!cmd_parm)
        return BDMF_ERR_PARM;

    while(cmd_parm->name)
    {
        if (!_bdmfmon_stricmp(name, cmd_parm->name, -1))
            break;
        ++cmd_parm;
    }

    if (!cmd_parm->name)
    {
        bdmf_session_print(session->session, "MON: %s> doesn't support parameter with name <%s>\n", entry->name, name);
        return BDMF_ERR_PARM;
    }

    *p_parm = parm;
    *n_parm = (cmd_parm - entry->u.cmd.parms);

    return 1;
}


/* Parse p_inbuf string based on parameter descriptions in <p_token>.
 *   Fill parameter values in <p_token>.
 *   Returns the number of parameters filled or BDMF_ERR_PARM
 *   To Do: add a option of one-by-one user input of missing parameters.
 */
static int _bdmfmon_parse_parms( bdmfmon_session_t* session, bdmfmon_entry_t* entry, uint16_t* pn_parms )
{
    int n_total_parms = _bdmfmon_get_n_of_parms( entry );
    int n_parms=0;
    char *parm_value;
    int positional=1;
    int n_parm;
    int is_named;
    bdmfmon_cmd_parm_t *parms=session->cmd_parms;
    bdmfmon_token_type_t token_type = BDMFMON_BDMFMON_TOKEN_EMPTY;
    int i;
    
    /* Mark all parameters as don't having an explicit value */
    for (i=0; i<n_total_parms; i++)
    {
        uint32_t flags = entry->u.cmd.parms[i].flags;
        parms[i] = entry->u.cmd.parms[i];
        parms[i].flags |= BDMFMON_PARM_FLAG_NOVAL;
        if ((flags & BDMFMON_PARM_FLAG_OPTIONAL) &&
            !(flags & BDMFMON_PARM_FLAG_DEFVAL))
        {
            /* Optional enum parameters are initialized by their 1st value by default.
             * All other parameters are initialized to 0.
             */
            if (parms[i].type == BDMFMON_PARM_ENUM)
            {
                bdmfmon_enum_val_t *values=parms[i].low_val.enum_table;
                parms[i].value.number = values[0].val;
            }
            else
                parms[i].value.number64 = 0;
        }
    }

    /* Build a format string */
    for (i=0; i<n_total_parms && *session->p_inbuf; i++)
    {
        if ( (parms[i].flags & BDMFMON_PARM_FLAG_EOL) )
        {
            char *p=NULL;

            /* Line parameter takes the rest of the input buffer.
             * If BDMFMON_PARM_STRING is also set - line ends by ';'
             */

            /* Skip leading blanks */
            while ( *session->p_inbuf && *session->p_inbuf==' ')
                ++session->p_inbuf;
            if ( ! (*session->p_inbuf) )
                break;
            if (p)
                *p = 0;
            else
                p = session->p_inbuf + strlen(session->p_inbuf);
            parm_value = session->p_inbuf;
            session->p_inbuf = p+1;
            n_parm = i;
            is_named = _check_named_parm(session, entry, &parm_value, &n_parm);
            if (is_named < 0)
                return BDMF_ERR_PARM;
            if  (!is_named && !positional)
            {
                bdmf_session_print(session->session, "MON: %s> expected named parameter. Got %s\n", entry->name, parm_value);
                return BDMF_ERR_PARM;
            }
            parms[n_parm].value.string = parm_value;
            parms[n_parm].flags &= ~BDMFMON_PARM_FLAG_NOVAL;
            ++n_parms;
            break;
        }
        else
        {
            bdmfmon_cmd_parm_t *cur_parm;
            
            if ((token_type = _bdmfmon_get_word(session, &parm_value, 0)) != BDMFMON_BDMFMON_TOKEN_NAME)
            {
                if (token_type == BDMFMON_BDMFMON_TOKEN_BREAK)
                    session->p_inbuf = NULL; /* stop on comment */
                break;
            }
            n_parm = i;
            is_named = _check_named_parm(session, entry, &parm_value, &n_parm);
            if (is_named < 0)
                return BDMF_ERR_PARM;
            if  (!is_named && !positional)
            {
                bdmf_session_print(session->session, "MON: %s> Expected named parameter. Got %s\n", entry->name, parm_value);
                return BDMF_ERR_PARM;
            }
            cur_parm = &parms[n_parm];
            /* 1st appearance of named parameter switches parser to named mode */
            if (is_named)
                positional = 0;
            
            if (cur_parm->type == BDMFMON_PARM_STRING)
                cur_parm->value.string = parm_value;
            else
            {
                /* Convert based on user-supplied format */
                if (cur_parm->scan_cb(cur_parm, parm_value))
                {
                    bdmf_session_print(session->session, "MON: %s> <%s>: can't convert value %s to format %s\n",
                                entry->name, cur_parm->name, parm_value, _bdmfmon_get_type_name(cur_parm->type));
                    return BDMF_ERR_PARM;
                }

                /* Check value */
                if ((cur_parm->flags & BDMFMON_PARM_FLAG_RANGE))
                {
                    if ((cur_parm->value.number < cur_parm->low_val.number) ||
                        (cur_parm->value.number > cur_parm->hi_val.number) )
                    {
                        bdmf_session_print(session->session, "MON: %s> <%s>: %ld out of range (%ld, %ld)\n",
                                    entry->name,
                                    cur_parm->name,
                                    cur_parm->value.number,
                                    cur_parm->low_val.number,
                                    cur_parm->hi_val.number );
                        return BDMF_ERR_PARM;
                    }
                }
            }
            
            cur_parm->flags &= ~BDMFMON_PARM_FLAG_NOVAL;
            ++n_parms;
        }
    }

    /* Make sure that there are no unexpected parameters */
    if (i == n_total_parms && session->p_inbuf)
    {
        token_type = _bdmfmon_get_word(session, &parm_value, 0);
        if (token_type !=  BDMFMON_BDMFMON_TOKEN_EMPTY && token_type !=  BDMFMON_BDMFMON_TOKEN_BREAK)
        {
            bdmf_session_print(session->session, "MON: %s> \"%s\" is unexpected\n",
                entry->name, parm_value);
            return BDMF_ERR_PARM;
        }
    }

    /* Process default values */
    for (i=0; i<n_total_parms; i++)
    {
        if ((parms[i].flags & BDMFMON_PARM_FLAG_NOVAL))
        {
            if ((parms[i].flags & BDMFMON_PARM_FLAG_DEFVAL))
            {
                parms[i].flags &= ~BDMFMON_PARM_FLAG_NOVAL;
                ++n_parms;
            }
            else if (!(parms[i].flags & BDMFMON_PARM_FLAG_OPTIONAL) )
            {
                /* Mandatory parameter missing */
                bdmf_session_print(session->session, "MON: %s> Mandatory parameter <%s> is missing\n", entry->name, parms[i].name);
                return BDMF_ERR_PARM;
            }
        }
    }

    *pn_parms = n_parms;

    return i;
}

/* Serach a token by name in the current directory.
 * The name can be qualified (contain path)
 */
bdmfmon_entry_t *_bdmfmon_search_token( bdmfmon_entry_t *p_dir, char *name )
{
    bdmfmon_entry_t *p_token;
    int name_len=strlen(name);
    char *p_slash;

    if (!name[0])
        return p_dir;
    
    /* Check if name is qualified */
    p_slash = strchr(name, '/');
    if (p_slash)
    {
        *p_slash = 0;
        if (p_slash == name)
            p_token = bdmfmon_root_dir;
        else
        {
            bdmfmon_token_type_t type=_bdmfmon_analize_token(name);
            switch(type)
            {
                case BDMFMON_BDMFMON_TOKEN_ROOT:
                    p_token = bdmfmon_root_dir;
                    break;
                case BDMFMON_BDMFMON_TOKEN_UP:
                    if (p_dir->parent)
                        p_token = p_dir->parent;
                    else
                        p_token = p_dir;
                    break;
                case BDMFMON_BDMFMON_TOKEN_NAME:
                    p_token = _bdmfmon_search_token(p_dir, name);
                    break;
                default:
                    return NULL;
            }
        }
        if (!p_token)
            return NULL;
        if (p_token->sel == BDMFMON_ENTRY_DIR)
            return _bdmfmon_search_token(p_token, p_slash+1);
        return p_token;
    }
    
    /* Check alias */
    p_token = p_dir->u.dir.first;
    while ( p_token )
    {
        if (p_token->alias &&
                (name_len == p_token->alias_len) &&
                !_bdmfmon_stricmp( p_token->alias, name, p_token->alias_len) )
            break;
        p_token = p_token->next;
    }
    if (p_token)
        return p_token;

    /* Check name */
    p_token = p_dir->u.dir.first;
    while ( p_token )
    {
        if (!_bdmfmon_stricmp( p_token->name, name, name_len ) )
            break;
        p_token = p_token->next;
    }

    return p_token;
}


/* Display help for each entry in the current directory */
void  _bdmfmon_dir_help( bdmfmon_session_t *session, bdmfmon_entry_t *p_dir )
{
    bdmfmon_entry_t *p_token;
    char buffer[BDMFMON_MAX_QUAL_NAME_LENGTH];

    _bdmfmon_qualified_name(p_dir, buffer, sizeof(buffer));
    bdmf_session_print(session->session, "Directory %s/ - %s\n", buffer, p_dir->help);
    bdmf_session_print(session->session, "Commands:\n");
    
    p_token = p_dir->u.dir.first;
    while ( p_token )
    {
        if (session->access_right >= p_token->access_right)
        {
            if (p_token->sel == BDMFMON_ENTRY_DIR)
                bdmf_session_print(session->session, "\t%s/:  %s directory\n", p_token->name, p_token->help );
            else
            {
                char *peol = strchr(p_token->help, '\n');
                int help_len = peol ? peol - p_token->help : strlen(p_token->help);
                bdmf_session_print(session->session, "\t%s(%d parms): %.*s\n",
                            p_token->name, _bdmfmon_get_n_of_parms(p_token), help_len, p_token->help );
            }
        }
        p_token = p_token->next;
    }
    bdmf_session_print(session->session, "Type ? <name> for command help, \"/\"-root, \"..\"-upper\n" );
}


/* Display help a token */
void  _bdmfmon_display_help( bdmfmon_session_t *session, bdmfmon_entry_t *p_token )
{
    char tmp[80];
    char bra, ket;
    uint16_t n_total_parms = _bdmfmon_get_n_of_parms( p_token );
    uint16_t i;
    char buffer[BDMFMON_MAX_QUAL_NAME_LENGTH];

    if (p_token->sel == BDMFMON_ENTRY_DIR)
    {
        _bdmfmon_dir_help(session, p_token);
        return;
    }
    
    _bdmfmon_qualified_name(p_token, buffer, sizeof(buffer));
    bdmf_session_print(session->session, "%s: \t%s\n", buffer, p_token->help );
    if (n_total_parms)
        bdmf_session_print(session->session, "Parameters:\n");
    for ( i=0; i<n_total_parms; i++ )
    {
        bdmfmon_cmd_parm_t *cur_parm = &p_token->u.cmd.parms[i];
        if ((cur_parm->flags & BDMFMON_PARM_FLAG_OPTIONAL))
        {
            bra = '[';
            ket=']';
        }
        else
        {
            bra = '<';
            ket='>';
        }
        bdmf_session_print(session->session, "\t%c%s(%s)", bra, cur_parm->name, _bdmfmon_get_type_name(cur_parm->type) );
        if (cur_parm->type == BDMFMON_PARM_ENUM)
        {
            bdmfmon_enum_val_t *values=cur_parm->low_val.enum_table;
            bdmf_session_print(session->session, " {");
            while(values->name)
            {
                if (values!=cur_parm->low_val.enum_table)
                    bdmf_session_print(session->session, ", ");
                bdmf_session_print(session->session, "%s", values->name);
                ++values;
            }
            bdmf_session_print(session->session, "}");
        }
        if ((cur_parm->flags & BDMFMON_PARM_FLAG_DEFVAL))
        {
            bdmf_session_print(session->session, "=");
            cur_parm->format_cb(cur_parm, cur_parm->value, tmp, sizeof(tmp));
            bdmf_session_print(session->session, "%s", tmp);
        }
        if ((cur_parm->flags & BDMFMON_PARM_FLAG_RANGE))
        {
            bdmf_session_print(session->session, " (");
            cur_parm->format_cb(cur_parm, cur_parm->low_val, tmp, sizeof(tmp));
            bdmf_session_print(session->session, "%s..", tmp);
            cur_parm->format_cb(cur_parm, cur_parm->hi_val, tmp, sizeof(tmp));
            bdmf_session_print(session->session, "%s)", tmp);
        }
        bdmf_session_print(session->session, "%c ", ket);
        bdmf_session_print(session->session, "- %s\n", cur_parm->description);
    }
    bdmf_session_print(session->session, "\n");
}


/* Choose unique alias for <name> in <p_dir> */
/* Currently only single-character aliases are supported */
static void __bdmfmon_chooseAlias( bdmfmon_entry_t *p_dir, bdmfmon_entry_t *p_new_token, int from )
{
    bdmfmon_entry_t *p_token;
    int         i;
    char        c;

    _bdmfmon_strlwr( p_new_token->name );
    i = from;
    while ( p_new_token->name[i] )
    {
        c = p_new_token->name[i];
        p_token = p_dir->u.dir.first;

        while ( p_token )
        {
            if (p_token->alias &&
                    (tolower( *p_token->alias ) == c) )
                break;
            if (strlen(p_token->name)<=2 && tolower(p_token->name[0])==c)
                break;
            p_token = p_token->next;
        }
        if (p_token)
            ++i;
        else
        {
            p_new_token->name[i] = toupper( c );
            p_new_token->alias   = &p_new_token->name[i];
            p_new_token->alias_len = 1;
            break;
        }
    }
}

void _bdmfmon_choose_alias( bdmfmon_entry_t *p_dir, bdmfmon_entry_t *p_new_token)
{
    int i=0;
    p_new_token->alias_len = 0;
    p_new_token->alias = NULL;
    /* Don't try to alias something short */
    if (strlen(p_new_token->name) < BDMFMON_MIN_NAME_LENGTH_FOR_ALIAS)
        return;
    /* Try pre-set alias 1st */
    while ( p_new_token->name[i] )
    {
        if (isupper(p_new_token->name[i]))
            break;
        i++;
    }
    if (p_new_token->name[i])
        __bdmfmon_chooseAlias(p_dir, p_new_token, i);
    if (p_new_token->alias != &p_new_token->name[i])
        __bdmfmon_chooseAlias(p_dir, p_new_token, 0);
}


/* Convert string s to lower case. Return pointer to s */
char  * _bdmfmon_strlwr( char *s )
{
    char  *s0=s;

    while ( *s )
    {
        *s = tolower( *s );
        ++s;
    }

    return s0;
}


/* Compare strings case incensitive */
int _bdmfmon_stricmp( const char *s1, const char *s2, int len )
{
    int  i;

    for ( i=0; (i<len || len<0); i++ )
    {
        if (tolower( s1[i])  != tolower( s2[i] ))
            return 1;
        if (!s1[i])
            break;
    }

    return 0;
}

static const char *_bdmfmon_get_type_name(bdmfmon_parm_type_t type)
{
    static const char *type_name[] = {
        [BDMFMON_PARM_DECIMAL]    = "decimal",
        [BDMFMON_PARM_DECIMAL64]  = "decimal64",
        [BDMFMON_PARM_UDECIMAL]   = "udecimal",
        [BDMFMON_PARM_UDECIMAL64] = "udecimal64",
        [BDMFMON_PARM_HEX]        = "hex",
        [BDMFMON_PARM_HEX64]      = "hex64",
        [BDMFMON_PARM_NUMBER]     = "number",
        [BDMFMON_PARM_NUMBER64]   = "number64",
        [BDMFMON_PARM_ENUM]       = "enum",
        [BDMFMON_PARM_STRING]     = "string",
        [BDMFMON_PARM_IP]         = "IP",
        [BDMFMON_PARM_IPV6]       = "IPv6",
        [BDMFMON_PARM_MAC]        = "MAC",
        [BDMFMON_PARM_USERDEF]    = "userdef"
    };
    static const char *undefined = "undefined";
    if (type > BDMFMON_PARM_USERDEF || !type_name[type])
        return undefined;
    return type_name[type];
}
    

/* Default function for string->value conversion.
 * Returns 0 if OK
 */
static int _bdmfmon_dft_scan_cb(bdmfmon_cmd_parm_t *parm, char *string_val)
{
    char *p_end = NULL;
    int n;
    switch(parm->type)
    {
        case BDMFMON_PARM_DECIMAL:
            parm->value.number = strtol(string_val, &p_end, 10);
            break;
        case BDMFMON_PARM_UDECIMAL:
            parm->value.unumber = strtoul(string_val, &p_end, 10);
            break;
        case BDMFMON_PARM_DECIMAL64:
            parm->value.number64 = strtoll(string_val, &p_end, 10);
            break;
        case BDMFMON_PARM_UDECIMAL64:
            parm->value.unumber64 = strtoull(string_val, &p_end, 10);
            break;
        case BDMFMON_PARM_HEX:
            parm->value.unumber = strtoul(string_val, &p_end, 16);
            break;
        case BDMFMON_PARM_HEX64:
            parm->value.unumber64 = strtoull(string_val, &p_end, 16);
            break;
        case BDMFMON_PARM_NUMBER:
            parm->value.number = strtol(string_val, &p_end, 0);
            break;
        case BDMFMON_PARM_NUMBER64:
            parm->value.number64 = strtoll(string_val, &p_end, 0);
            break;
        case BDMFMON_PARM_MAC:
        {
            unsigned m0, m1, m2, m3, m4, m5;
            n = sscanf(string_val, "%02x:%02x:%02x:%02x:%02x:%02x",
                &m0, &m1, &m2, &m3, &m4, &m5);
            if (n != 6)
                return BDMF_ERR_PARM;
            if (m0 > 255 || m1 > 255 || m2 > 255 || m3 > 255 || m4 > 255 || m5 > 255)
                return BDMF_ERR_PARM;
            parm->value.mac[0] = m0;
            parm->value.mac[1] = m1;
            parm->value.mac[2] = m2;
            parm->value.mac[3] = m3;
            parm->value.mac[4] = m4;
            parm->value.mac[5] = m5;
            break;
        }
        case BDMFMON_PARM_IP:
        {
            int n1, n2, n3, n4;
            n = sscanf(string_val, "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
            if (n != 4)
                return BDMF_ERR_PARM;
            if ((unsigned)n1 > 255 || (unsigned)n2 > 255 || (unsigned)n3 > 255 || (unsigned)n4 > 255)
                return BDMF_ERR_PARM;
            parm->value.unumber = (n1 << 24) | (n2 << 16) | (n3 << 8) | n4;
            break;
        }
        default:
            return BDMF_ERR_PARM;
    }
    if (p_end && *p_end)
        return BDMF_ERR_PARM;
    return 0;
}

static void _bdmfmon_dft_format_cb(const bdmfmon_cmd_parm_t *parm, bdmfmon_parm_value_t value, char *buffer, int size)
{
    switch(parm->type)
    {
        case BDMFMON_PARM_DECIMAL:
            snprintf(buffer, size, "%ld", value.number);
            break;
        case BDMFMON_PARM_UDECIMAL:
            snprintf(buffer, size, "%lu", value.unumber);
            break;
        case BDMFMON_PARM_DECIMAL64:
            snprintf(buffer, size, "%lld", value.number64);
            break;
        case BDMFMON_PARM_UDECIMAL64:
            snprintf(buffer, size, "%llu", value.unumber64);
            break;
        case BDMFMON_PARM_HEX:
            snprintf(buffer, size, "%lx", value.unumber);
            break;
        case BDMFMON_PARM_HEX64:
            snprintf(buffer, size, "%llx", value.unumber64);
            break;
        case BDMFMON_PARM_NUMBER:
            snprintf(buffer, size, "%li", value.number);
            break;
        case BDMFMON_PARM_NUMBER64:
            snprintf(buffer, size, "%lli", value.number64);
            break;
        case BDMFMON_PARM_STRING:
            snprintf(buffer, size, "%s", value.string);
            break;
        case BDMFMON_PARM_MAC:
            snprintf(buffer, size, "%02x:%02x:%02x:%02x:%02x:%02x",
                parm->value.mac[0], parm->value.mac[1], parm->value.mac[2],
                parm->value.mac[3], parm->value.mac[4], parm->value.mac[5]);
            break;
        case BDMFMON_PARM_IP:
            snprintf(buffer, size, "%d.%d.%d.%d",
                (int)((parm->value.unumber >> 24) & 0xff), (int)((parm->value.unumber >> 16) & 0xff),
                (int)((parm->value.unumber >> 8) & 0xff), (int)(parm->value.unumber & 0xff));
            break;
        default:
            strncpy(buffer, "*unknown*", size);
    }
}

static int _bdmfmon_scan_enum_cb(bdmfmon_cmd_parm_t *parm, char *string_val)
{
    bdmfmon_enum_val_t *values=parm->low_val.enum_table;
    while(values->name)
    {
        if (!_bdmfmon_stricmp(values->name, string_val, -1))
        {
            parm->value.number = values->val;
            return 0;
        }
        ++values;
    }
    return BDMF_ERR_PARM;
}

static void _bdmfmon_format_enum_cb(const bdmfmon_cmd_parm_t *parm, bdmfmon_parm_value_t value, char *buffer, int size)
{
    bdmfmon_enum_val_t *values=parm->low_val.enum_table;
    while(values->name)
    {
        if (values->val == value.number)
            break;
        ++values;
    }
    if (values->name)
        strncpy(buffer, values->name, size);
    else
        strncpy(buffer, "*invalid*", size);
}

static const char *_bdmfmon_qualified_name( bdmfmon_entry_t *token, char *buffer, int size )
{
    bdmfmon_entry_t *parent = token->parent;
    char qual_name[BDMFMON_MAX_QUAL_NAME_LENGTH];
    *buffer=0;
    while(parent)
    {
        strncpy(qual_name, parent->name, sizeof(qual_name));
        if (parent->parent)
            strncat(qual_name, "/", sizeof(qual_name));
        strncat(qual_name, buffer, sizeof(qual_name));
        strncpy(buffer, qual_name, size);
        parent = parent->parent;
    }
    size -= strlen(buffer);
    strncat(buffer, token->name, size);
    return buffer;
}

/*
 * Exports
 */
EXPORT_SYMBOL(bdmfmon_dir_add);
EXPORT_SYMBOL(bdmfmon_dir_find);
EXPORT_SYMBOL(bdmfmon_token_name);
EXPORT_SYMBOL(bdmfmon_cmd_add);
EXPORT_SYMBOL(bdmfmon_session_open);
EXPORT_SYMBOL(bdmfmon_session_close);
EXPORT_SYMBOL(bdmfmon_parse);
EXPORT_SYMBOL(bdmfmon_stop);
EXPORT_SYMBOL(bdmfmon_is_stopped);
EXPORT_SYMBOL(bdmfmon_dir_get);
EXPORT_SYMBOL(bdmfmon_dir_set);
EXPORT_SYMBOL(bdmfmon_parm_number);
EXPORT_SYMBOL(bdmfmon_parm_is_set);
EXPORT_SYMBOL(bdmfmon_enum_stringval);
EXPORT_SYMBOL(bdmfmon_enum_parm_stringval);
EXPORT_SYMBOL(bdmfmon_token_destroy);
EXPORT_SYMBOL(bdmfmon_enum_bool_table);
