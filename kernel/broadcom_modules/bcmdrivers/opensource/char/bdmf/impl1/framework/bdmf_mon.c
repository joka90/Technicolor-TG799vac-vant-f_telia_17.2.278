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
 * BDMF framework - flow monitor commands
 *
 * This file is Copyright (c) 2011, Broadlight Communications.
 * This file is licensed under GNU Public License, except that if
 * you have entered in to a signed, written license agreement with
 * Broadlight covering this file, that agreement applies to this
 * file instead of the GNU Public License.
 *
 * This file is free software: you can redistribute and/or modify it
 * under the terms of the GNU Public License, Version 2, as published
 * by the Free Software Foundation, unless a different license
 * applies as provided above.
 *
 * This program is distributed in the hope that it will be useful,
 * but AS-IS and WITHOUT ANY WARRANTY; without even the implied
 * warranties of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * TITLE or NONINFRINGEMENT. Redistribution, except as permitted by
 * the GNU Public License or another license agreement between you
 * and Broadlight, is prohibited.
 *
 * You should have received a copy of the GNU Public License,
 * Version 2 along with this file; if not, see
 * <http://www.gnu.org/licenses>.
 *
 * Author: Igor Ternovsky
 *******************************************************************/

#include <bdmf_dev.h>
#include <bdmf_session.h>
#include <bdmf_shell.h>

/* Examine output format */
#define BDMFMON_EXAMINE_AG_FORMAT_INPUT  0       /* The same output format as input format */
#define BDMFMON_EXAMINE_AG_FORMAT_SPACE  1       /* Similar to input format, with extra space between fields */
#define BDMFMON_EXAMINE_AG_FORMAT_LINE   2       /* Each field in a separate line */

/*
 * object menu
 */

/*
 * Helper functions
 */

#define OBJHLP_TMPBUF_SIZE 1024
static int _bdmf_obj_help(bdmf_session_handle session, struct bdmf_type *drv,
                           const char *what)
{
    const struct bdmf_attr *attrs=drv->aattr;
    char *tmp_buf;
    int rc;
    
    tmp_buf = bdmf_alloc(OBJHLP_TMPBUF_SIZE);
    if (!tmp_buf)
        return BDMF_ERR_NOMEM;

    rc = bdmf_help(drv, what, tmp_buf, OBJHLP_TMPBUF_SIZE);
    if (rc)
    {
        bdmf_free(tmp_buf);
        return rc;
    }
    bdmf_session_print(session, "%s\n", tmp_buf);

    bdmf_session_print(session, "Attributes: (M-mandatory,K=key,R-read,W-write,I-write_on_init,A-add,D-delete,F-find)\n");
    while(attrs && attrs->name)
    {
        bdmf_attr_help(attrs, tmp_buf, OBJHLP_TMPBUF_SIZE);
        if (!(attrs->flags & BDMF_ATTR_DEPRECATED))
            bdmf_session_print(session, "   %s\n", tmp_buf);
        ++attrs;
    }
    bdmf_free(tmp_buf);
    return 0;
}


static void _bdmf_split_descriptor(char *type_attrs, char **ptype, char **pattrs)
{
    static char *nullstr="";
    char *attrs;
    attrs = strchr(type_attrs, '/');
    if (attrs)
        *(attrs++) = 0;
    else
        attrs = nullstr;
    *ptype = type_attrs;
    *pattrs = attrs;
}


static int _bdmf_get_type_attrs(bdmf_session_handle session,
                                char *type_attrs,
                                struct bdmf_type **pdrv,
                                char **pattrs)
{
    struct bdmf_type *drv;
    char *type;
    char *attrs;
    int rc;

    *pdrv=NULL;
    *pattrs=NULL;
    
    _bdmf_split_descriptor(type_attrs, &type, &attrs);
    rc = bdmf_type_find_get(type, &drv);
    if (rc)
    {
        bdmf_session_print(session, "Type %s is not registered\n", type);
        return rc;
    }
    if (attrs[0]=='?')
    {
        _bdmf_obj_help(session, drv, attrs+1);
        bdmf_type_put(drv);
        return 0;
    }
    *pdrv = drv;
    *pattrs = attrs;
    return 0;
}


static int _bdmf_get_object(bdmf_session_handle session,
                            char *type_attrs, struct bdmf_object **pobj)
{
    char *attrs;
    struct bdmf_type *drv;
    int rc;
    *pobj = NULL;
    rc = _bdmf_get_type_attrs(session, type_attrs, &drv, &attrs);
    if (rc || !drv)
        return rc;
    rc = bdmf_find_get(drv, NULL, attrs, pobj);
    bdmf_type_put(drv);
    if (rc)
        bdmf_session_print(session, "Cannot find object %s/%s\n", type_attrs, attrs);
    return rc;
}


/* Create a new object
    BDMFMON_MAKE_PARM("obj",   "type/attributes | type/?", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("parent","type/attributes", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
*/
static int bdmf_mon_object_new(bdmf_session_handle session,
                               const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    char *type_attrs=parm[0].value.string;
    char *parent_attrs=parm[1].value.string;
    char *attrs;
    struct bdmf_type *drv;
    struct bdmf_object *obj;
    struct bdmf_object *parent=NULL;
    int rc;
    if (parent_attrs)
    {
        rc = _bdmf_get_object(session, parent_attrs, &parent);
        if (rc)
            return rc;
    }
    rc = _bdmf_get_type_attrs(session, type_attrs, &drv, &attrs);
    if (rc || !drv)
        return rc;
    rc = bdmf_new_and_configure(drv, parent, attrs, &obj);
    if (rc)
        bdmf_session_print(session, "# Object creation failed: %s\n", bdmf_strerror(rc));
    else
        bdmf_session_print(session, "# Created object <%s>\n", obj->name);
    bdmf_type_put(drv);
    if (parent)
        bdmf_put(parent);
    return rc;
}


/* Delete object
    BDMFMON_MAKE_PARM("obj",   "type/discriminator", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_object_delete(bdmf_session_handle session,
                                  const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    char *type_attrs=parm[0].value.string;
    struct bdmf_object *obj;
    int rc;

    rc = _bdmf_get_object(session, type_attrs, &obj);
    if (rc || !obj)
        return rc;
    bdmf_put(obj);
    bdmf_destroy(obj);
    return 0;
}


/* Configure object
    BDMFMON_MAKE_PARM("obj",   "type/discriminator | type/?", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attrs", "attributes", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_object_configure(bdmf_session_handle session,
                                     const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    char *type_discr=parm[0].value.string;
    char *attr=parm[1].value.string;
    struct bdmf_object *obj;
    int rc;

    rc = _bdmf_get_object(session, type_discr, &obj);
    if (rc || !obj)
        return rc;
    rc = bdmf_configure(obj, attr);
    if (rc)
        bdmf_session_print(session, "Object configuration failed: %s\n", bdmf_strerror(rc));
    bdmf_put(obj);
    return rc;
}


/* Link objects
    BDMFMON_MAKE_PARM("ds_obj", "ds_type/discriminator", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("us_obj", "us_type/discriminator", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attrs",  "link attributes",     BDMFMON_PARM_STRING, BDMFMON_PARM_OPTIONAL),
*/
static int bdmf_mon_object_link(bdmf_session_handle session,
                            const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type_discr1=parm[0].value.string;
    char *type_discr2=parm[1].value.string;
    char *attr=(n_parms>2) ? parm[2].value.string : NULL;
    struct bdmf_object *obj1;
    struct bdmf_object *obj2;
    int rc;

    rc = _bdmf_get_object(session, type_discr1, &obj1);
    if (rc || !obj1)
        return rc;
    rc = _bdmf_get_object(session, type_discr2, &obj2);
    if (rc || !obj2)
    {
        bdmf_put(obj1);
        return rc;
    }
    rc = bdmf_link(obj1, obj2, attr);
    if (rc)
        bdmf_session_print(session, "Object link failed: %s\n", bdmf_strerror(rc));
    bdmf_put(obj1);
    bdmf_put(obj2);
    return rc;
}


/* Unlink objects
    BDMFMON_MAKE_PARM("ds_obj", "ds_type/discriminator", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("us_obj", "us_type/discriminator", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_object_unlink(bdmf_session_handle session,
                                const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type_discr1=parm[0].value.string;
    char *type_discr2=parm[1].value.string;
    struct bdmf_object *obj1;
    struct bdmf_object *obj2;
    int rc;

    rc = _bdmf_get_object(session, type_discr1, &obj1);
    if (rc || !obj1)
        return rc;
    rc = _bdmf_get_object(session, type_discr2, &obj2);
    if (rc || !obj2)
    {
        bdmf_put(obj1);
        return rc;
    }
    rc = bdmf_unlink(obj1, obj2);
    if (rc)
        bdmf_session_print(session, "Object un-link failed: %s\n", bdmf_strerror(rc));
    bdmf_put(obj1);
    bdmf_put(obj2);
    return rc;
}

/* types - hierarchical view */
static void _bdmf_mon_type_tree(bdmf_session_handle session, struct bdmf_type *drv, int level)
{
    struct bdmf_type *d=NULL;
    if (drv->po)
    {
        int i;
        for(i=0; i<level; i++)
            bdmf_session_print(session, "    ");
        bdmf_session_print(session, "%-16s %s\n", drv->name, drv->description);
    }
    /* find and print all child types */
    while((d=bdmf_type_get_next(d)))
    {
        if (d->po == drv)
            _bdmf_mon_type_tree(session, d, level+1);
    }
}

/* List object types
 */
static int bdmf_mon_types(bdmf_session_handle session,
                          const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    struct bdmf_type *drv=NULL;
    while((drv=bdmf_type_get_next(drv)))
    {
        if (!drv->po)
        {
            bdmf_session_print(session, "%-16s %s\n", drv->name, drv->description);
            _bdmf_mon_type_tree(session, drv, 0);
        }
    }
    return 0;
}

/* recursively print object's children */
static void _bdmf_mon_object_children(bdmf_session_handle session, struct bdmf_object *obj, int level)
{
    struct bdmf_object *child=NULL;
    int i;
    while((child=bdmf_get_next_child(obj, NULL, child)))
    {
        for(i=0; i<level; i++)
            bdmf_session_print(session, "    ");
        bdmf_session_print(session, "%s\n", child->name);
        _bdmf_mon_object_children(session, child, level+1);
    }
}

/* List objects
    BDMFMON_MAKE_PARM("type",   "object type [/attributes]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM_ENUM_DEFVAL("children", "Hierarchical children view", bdmfmon_enum_bool_table,
                    BDMFMON_PARM_FLAG_OPTIONAL, "yes"),
*/
static int bdmf_mon_objects(bdmf_session_handle session,
                            const bdmfmon_cmd_parm_t parm[], uint16_t n_parms)
{
    char *type_attrs=parm[0].value.string;
    long children=parm[1].value.number;
    struct bdmf_type *drv;
    struct bdmf_object *obj=NULL;
    char *attrs;
    int n=0;
    int rc;

    rc = _bdmf_get_type_attrs(session, type_attrs, &drv, &attrs);
    if (rc || !drv)
        return rc;
    bdmf_session_print(session, "Objects of type %s (%s)", drv->name, drv->description);
    if (attrs && *attrs)
        bdmf_session_print(session, " with filter %s", attrs);
    bdmf_session_print(session, "\n");
    while((obj=bdmf_get_next(drv, obj, attrs)))
    {
        bdmf_session_print(session, "%s\n", obj->name);
        if (children)
            _bdmf_mon_object_children(session, obj, 1);
        ++n;
    }
    bdmf_type_put(drv);
    if (!n && attrs && *attrs)
        return BDMF_ERR_NOENT;
    return 0;
}

#define BDMF_MON_MAX_ATTRELEM_SHOW    8

static int _bdmf_mon_check_aggr_nzstat(bdmf_session_handle session, struct bdmf_object *obj,
    struct bdmf_attr *attr, char *val, char *prefix)
{
    struct bdmf_attr *a;
    int rc = 0;

    for(a=attr->aggr_type->fields; a->name; ++a)
    {
        char *data = val + a->offset;
        char full_prefix[128];

        snprintf(full_prefix, sizeof(full_prefix), "%s.%s", prefix, a->name);

        if (a->type == bdmf_attr_aggregate)
            rc += _bdmf_mon_check_aggr_nzstat(session, obj, a, data, full_prefix);
        else
        {
            bdmf_number n = 0;
            switch(a->size)
            {
            case 1: n = *(uint8_t *)data; break;
            case 2: n = *(uint16_t *)data; break;
            case 4: n = *(uint32_t *)data; break;
            case 8: n = *(uint64_t *)data; break;
            default: break;
            }
            if (!n)
                continue;
            bdmf_session_print(session, "%s = %lld\n", full_prefix, n);
            ++rc;
        }
    }
    return rc;
}

#define BDMFMON_MAX_SVAL_SIZE   4096
#define BDMFMON_MAX_SVAL1_SIZE  256

static inline void _bdmf_print_long_val(bdmf_session_handle session, const char *val)
{
    /* print value char by char to overcome internal limitations on max string length
     * for %s format
     */
    const char *pv;
    for(pv = val; *pv; ++pv)
        bdmf_session_print(session, "%c", *pv);
}

static int _bdmf_mon_format_aggr_val(bdmf_session_handle session, struct bdmf_object *obj,
    struct bdmf_attr *attr, char *val, char *buffer, uint32_t size, int format, int level)
{
    char *buf0 = buffer;
    struct bdmf_attr *a;
    char *sval;
    int rc = 0;
    int o = 0;
    int is_first = 1;
    char column[128]="";

    sval = bdmf_alloc(BDMFMON_MAX_SVAL1_SIZE);
    if (!sval)
        return BDMF_ERR_NOMEM;
    if (format == BDMFMON_EXAMINE_AG_FORMAT_LINE)
    {
        int i;
        for(i=0; i<=level; i++)
            strncat(column, "    ", sizeof(column));
    }

    if (format == BDMFMON_EXAMINE_AG_FORMAT_SPACE)
        o = snprintf(buffer, size, "{ ");
    else
        o = snprintf(buffer, size, "\n");
    buffer += o;
    size -= o;
    for(a=attr->aggr_type->fields; a->name; ++a)
    {
        char *data = val + a->offset;

        /* Skip disabled fields */
        if (attr->is_field_visible && !attr->is_field_visible(obj, attr, val, attr->aggr_type, a))
            continue;

        if (a->type == bdmf_attr_aggregate)
        {
            if (format == BDMFMON_EXAMINE_AG_FORMAT_SPACE)
                o = snprintf(buffer, size, "%s%s=", is_first ? "" : ", ", a->name);
            else
                o = snprintf(buffer, size, "%s%s", column, a->name);
            buffer += o;
            size -= o;
            o = rc = _bdmf_mon_format_aggr_val(session, obj, a, data, buffer, size, format, level + 1);
        }
        else
        {
            rc = a->val_to_s(obj, a, data, sval, BDMFMON_MAX_SVAL1_SIZE);
            if (format == BDMFMON_EXAMINE_AG_FORMAT_SPACE)
            {
                o = snprintf(buffer, size, "%s%s=%s", is_first ? "" : ", ",
                    a->name, (rc < 0) ? "invalid" : sval);
            }
            else
            {
                char format[32];
                if (level > 3)
                    strcpy(format, "%s%s : %s\n");
                else
                    snprintf(format, sizeof(format), "%%s%%-%ds : %%s\n", 40 - (level + 1)*4);
                o = snprintf(buffer, size, format, column, a->name, (rc < 0) ? "invalid" : sval);
            }
        }
        if (rc >= 0)
        {
            buffer += o;
            size -= o;
        }
        is_first = 0;
    }
    if (format == BDMFMON_EXAMINE_AG_FORMAT_SPACE)
    {
        o = snprintf(buffer, size, " }");
        buffer += o;
    }
    bdmf_free(sval);

    return buffer - buf0;
}

static int _bdmf_mon_get_attr_val(bdmf_session_handle session, struct bdmf_object *obj, struct bdmf_attr *attr,
    bdmf_index aindex, char *buffer, uint32_t size, int format)
{
    bdmf_attr_id aid = bdmf_attr_to_aid(obj->drv, attr);
    char int_val[attr->size];
    int rc;

    if (attr->type != bdmf_attr_aggregate || format == BDMFMON_EXAMINE_AG_FORMAT_INPUT)
        return bdmf_attrelem_get_as_string(obj, aid, aindex, buffer, size);

    /* Aggregate attribute in beautyfied format */
    rc = bdmf_attrelem_get_as_buf(obj, aid, aindex, int_val, sizeof(int_val));
    if (rc < 0)
        return rc;

    return _bdmf_mon_format_aggr_val(session, obj, attr, int_val, buffer, size, format, 0);
}


/* Internal helper: examine a single attribute on specific object */
static int _bdmf_mon_object_examine_single(bdmf_session_handle session,
                    struct bdmf_object *obj, struct bdmf_attr *attr,
                    char *array_index, int nzstat, int max_prints, int format)
{
    bdmf_attr_id aid = bdmf_attr_to_aid(obj->drv, attr);
    char qname[64];
    char *val_buffer;
    char index_buffer[attr->index_size];
    bdmf_index aindex=-1;
    int is_numeric_index = bdmf_attr_type_is_numeric(attr->index_type);

    /* take care of array index */
    if (array_index)
    {
        if (bdmf_attr_string_to_array_index(obj, attr, array_index, index_buffer))
        {
            bdmf_session_print(session, "Array index is invalid for %s[%s] on %s\n",
                       attr->name, array_index, obj->drv->name);
            return BDMF_ERR_PARM;
        }
        aindex = is_numeric_index ? *(bdmf_index *)index_buffer : (bdmf_index)index_buffer;
    }

    val_buffer = bdmf_alloc(BDMFMON_MAX_SVAL_SIZE);
    if (!val_buffer)
        return BDMF_ERR_NOMEM;

    qname[sizeof(qname)-1]=0;
    if (aindex >= 0 || (!is_numeric_index && aindex != -1))
    {
        int rc;
        /* Specific element */
        snprintf(qname, sizeof(qname), "%s[%s]", attr->name, array_index);
        if ((rc=_bdmf_mon_get_attr_val(session, obj, attr, aindex, val_buffer, BDMFMON_MAX_SVAL_SIZE, format)) < 0)
        {
            bdmf_session_print(session, "Can't fetch attribute %s on %s. %s\n",
                        qname, obj->drv->name, bdmf_strerror(rc));
            bdmf_free(val_buffer);
            return rc;
        }
        if (format == BDMFMON_EXAMINE_AG_FORMAT_LINE)
        {
            if (attr->type == bdmf_attr_aggregate)
                bdmf_session_print(session, "%s", qname);
            else
                bdmf_session_print(session, "%-40s : ", qname);
        }
        else
            bdmf_session_print(session, "%20s : ", qname);
        _bdmf_print_long_val(session, val_buffer);
        if (format != BDMFMON_EXAMINE_AG_FORMAT_LINE || attr->type != bdmf_attr_aggregate)
            bdmf_session_print(session, "\n");
    }
    else
    {
        char *index_buf[attr->index_size];
        bdmf_index j;
        bdmf_index *pj;
        int rc = 0;
        int nprints = 0;

        /* for numerical indexes (the majority of cases) array_index is just a number
         * passed by value. For non-numerical index we pass pointer to the buffer containing index value
         */
        if (attr->index_size >= sizeof(bdmf_index))
            *(bdmf_index *)index_buf = BDMF_INDEX_UNASSIGNED;

        /* Numeric indexes are passed by value, other - by address */
        if (bdmf_attr_type_is_numeric(attr->index_type))
        {
            j = *(bdmf_index *)index_buf;
            pj = &j;
        }
        else
        {
            j = (bdmf_index)&index_buf[0];
            pj = (bdmf_index *)&index_buf[0];
        }

        /* Scalar or all array values */
        if (attr->array_size > 0)
            rc = bdmf_attrelem_get_next(obj, aid, pj);
        for(;
            !rc && (max_prints<0 || nprints<max_prints);
            rc=bdmf_attrelem_get_next(obj, aid, pj))
        {


            /* Fetch statistics as numeric if nzstat is set.
             * Note that in this case if we are here that means that
             * the attribute is of class STATISTIC and, therefore,
             * is numeric
             */
            if (nzstat)
            {
                /* Prepare attribute name[index] */
                if (attr->array_size)
                {
                    char index_sval[256]="invalid";
                    bdmf_attr_array_index_to_string(obj, attr, j, index_sval, sizeof(index_sval));
                    snprintf(qname, sizeof(qname), "%s : %s[%s]", obj->name, attr->name, index_sval);
                }
                else
                    snprintf(qname, sizeof(qname), "%s : %s", obj->name, attr->name);

                if (attr->type == bdmf_attr_aggregate)
                {
                    /* value has some reserve for possible last field's padding */
                    int val_size = (attr->size + 7) & ~0x7;
                    char val[val_size];
                    rc = bdmf_attrelem_get_as_buf(obj, aid, j, val, val_size);
                    if (!rc)
                        nprints += _bdmf_mon_check_aggr_nzstat(session, obj, attr, val, qname);
                }
                else
                {
                    bdmf_number num_val = 0;
                    rc = bdmf_attrelem_get_as_num(obj, aid, j, &num_val);
                    if (num_val)
                    {
                        bdmf_session_print(session, "%s = %lld\n", qname, num_val);
                        ++nprints;
                    }
                }
                if (!rc)
                    continue;
            }
            else
            {
                rc = _bdmf_mon_get_attr_val(session, obj, attr, j, val_buffer, BDMFMON_MAX_SVAL_SIZE, format);
            }
            if (rc < 0)
            {
                /* BDMF_ERR_NOENT might mean that there is a hole in array attribute.
                 * In this case keep iterating. Perhaps, there are some valid entries left
                 */
                if (rc == BDMF_ERR_NOENT)
                    rc = 0;
                continue;
            }

            /* Output for non - nzstat requests */
            /* Prepare attribute name[index] */
            if (attr->array_size)
            {
                char index_sval[256]="invalid";
                bdmf_attr_array_index_to_string(obj, attr, j, index_sval, sizeof(index_sval));
                snprintf(qname, sizeof(qname), "%s[%s]", attr->name, index_sval);
            }
            else
                snprintf(qname, sizeof(qname), "%s", attr->name);
            /* print attribute name : value */
            if (format == BDMFMON_EXAMINE_AG_FORMAT_LINE)
            {
                if (attr->type == bdmf_attr_aggregate)
                    bdmf_session_print(session, "%s", qname);
                else
                    bdmf_session_print(session, "%-40s : ", qname);
            }
            else
                bdmf_session_print(session, "%20s : ", qname);
            _bdmf_print_long_val(session, val_buffer);
            if (format != BDMFMON_EXAMINE_AG_FORMAT_LINE || attr->type != bdmf_attr_aggregate)
                bdmf_session_print(session, "\n");
            ++nprints;
        }
        if (rc != BDMF_ERR_NO_MORE && attr->array_size > 0 && nprints)
        {
            if (format == BDMFMON_EXAMINE_AG_FORMAT_LINE)
                bdmf_session_print(session, "%-40s\n", "  *other elements not shown*");
            else
                bdmf_session_print(session, "%20s\n", "  *other elements not shown*");
        }
    }
    bdmf_free(val_buffer);

    return 0;
}


/* Internal helper: examine all matching attributes. specific object or hierarchical view */
static int _bdmf_mon_object_examine(bdmf_session_handle session,
                    struct bdmf_object *obj, char *name, char *array_index,
                    uint32_t class_level, int nzstat, int hierarchical, int format,
                    int max_prints)
{
    struct bdmf_attr *attr;
    int i;
    int nfound = 0;
    bdmf_link_handle lo;
    int rc;

    if (!nzstat)
    {
        bdmf_session_print(session, "Object: %s. Object type: %s.", obj->name, obj->drv->name);
        if (obj->owner)
            bdmf_session_print(session, " Owned by: %s", obj->owner->name);
        bdmf_session_print(session, "\n==============================\n");
    }

    /* Try for exact match 1st if name is set and object examination
     * is for single object rather than including al children
     */
    if (name && !hierarchical && !class_level)
    {
        bdmf_attr_id aid;
        /* check for exact match. if no exact match - perhaps it is pattern match */
        if ((rc=bdmf_attr_by_name(obj->drv, name, &aid)))
        {
            /* indexed pattern is not supported */
            if (array_index)
            {
                bdmf_session_print(session, "Cannot find or access indexed attribute %s\n", name);
                return BDMF_ERR_PARM;
            }
            /* scan all attributes */
            goto partial_match;
        }
        nfound = 1;
        attr = bdmf_aid_to_attr(obj->drv, aid);

        /* Examine attribute value(s) */
        rc = _bdmf_mon_object_examine_single(session, obj, attr, array_index,
                                             nzstat, -1, format);
    }
    else
    {
    partial_match:
        /* All matching attributes on the object */
        for(i=0; i<obj->drv->nattrs; i++)
        {
            attr = bdmf_aid_to_attr(obj->drv, i);
            if (name && !strstr(attr->name, name))
                continue;

            /* Attribute class/level filtering */
            if ((class_level & BDMF_ATTR_LEVEL_MASK) &&
                !(attr->flags & BDMF_ATTR_LEVEL_MASK & class_level))
                continue;
            if ((class_level & BDMF_ATTR_CLASS_MASK) &&
                !(attr->flags & BDMF_ATTR_CLASS_MASK & class_level))
                continue;
            if (attr->flags & BDMF_ATTR_DEPRECATED)
            	continue;

            ++nfound;
            rc = _bdmf_mon_object_examine_single(session, obj, attr, array_index,
                                            nzstat, nzstat ? -1 : max_prints, format);
        }
    }

    /* Print links if there is no attribute selection criteria (full object dump) */
    if (!name && !class_level && !nzstat)
    {
        /* Print US links */
        lo = bdmf_get_next_us_link(obj, NULL);
        if (lo)
        {
            bdmf_session_print(session, "US links\n");
            do {
                bdmf_session_print(session, "\t%s\n", bdmf_us_link_to_object(lo)->name);
                lo = bdmf_get_next_us_link(obj, lo);
            } while(lo);
        }

        /* Print DS links */
        lo = bdmf_get_next_ds_link(obj, NULL);
        if (lo)
        {
            bdmf_session_print(session, "DS links\n");
            do {
                bdmf_session_print(session, "\t%s\n", bdmf_ds_link_to_object(lo)->name);
                lo = bdmf_get_next_ds_link(obj, lo);
            } while(lo);
        }
    }

    /* If hierarchical match - examine all child objects */
    if (hierarchical)
    {
        struct bdmf_object *child=NULL;
        while((child=bdmf_get_next_child(obj, NULL, child)))
        {
            _bdmf_mon_object_examine(session, child, name, 
                    array_index, class_level, nzstat, hierarchical, format, max_prints);
        }
    }

    return nfound;
}

/* Examine object
    BDMFMON_MAKE_PARM("obj",   "type[/discriminator]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "attribute name or substring", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
    BDMFMON_MAKE_PARM_ENUM("class", "Attribute class", attr_class_table, BDMFMON_PARM_FLAG_OPTIONAL),
    BDMFMON_MAKE_PARM_ENUM("level", "Attribute level", attr_level_table, BDMFMON_PARM_FLAG_OPTIONAL),
    BDMFMON_MAKE_PARM_ENUM("children", "Hierarchical children view", bdmfmon_enum_bool_table, BDMFMON_PARM_FLAG_OPTIONAL),
    BDMFMON_MAKE_PARM_ENUM_DEFVAL("format", "Aggregate output format", attr_format_table, BDMFMON_PARM_FLAG_OPTIONAL, "input"),
    BDMFMON_MAKE_PARM_DEFVAL("max_prints", "Max number of array elements to print", BDMFMON_PARM_NUMBER, 0, BDMF_MON_MAX_ATTRELEM_SHOW),
*/
static int bdmf_mon_object_examine(bdmf_session_handle session,
                                    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type_discr=parm[0].value.string;
    char *aname=parm[1].value.string;
    int32_t cl=parm[2].value.number;
    int32_t lev=parm[3].value.number;
    int32_t hierarchical=parm[4].value.number;
    int32_t format=parm[5].value.number;
    int32_t max_prints=parm[6].value.number;

    uint32_t class_level;
    struct bdmf_object *obj = NULL;
    struct bdmf_type *drv = NULL;
    char *name=NULL;
    char *array_index=NULL;
    int nfound=0;
    int nzstat=0;
    int iterate_type;
    int rc;

    /* specific object or all objects of the given type ? */
    iterate_type = !strchr(type_discr, '/');
    if (!iterate_type)
    {
        rc = _bdmf_get_object(session, type_discr, &obj);
        if (rc || !obj)
            return rc;
    }
    else
    {
        char *attrs;
        rc = _bdmf_get_type_attrs(session, type_discr, &drv, &attrs);
        if (rc || !drv)
            return rc;
    }

    class_level = 0;
    if (bdmfmon_parm_is_set(session, 2))
    {
        /* non-zero statistics are represented as BDMF_ATTR_CONFIG | BDMF_ATTR_STAT */
        if (cl == (BDMF_ATTR_CONFIG | BDMF_ATTR_STAT))
        {
            cl = BDMF_ATTR_STAT;
            nzstat = 1;
        }
        class_level |= cl;
    }
    if (bdmfmon_parm_is_set(session, 3))
        class_level |= lev;

    /* Parse aname that can be in format name[array_index] */
    if (aname)
    {
        /* Show specific attribute */
        name = strsep(&aname, "["); /* Is name indexed ? */
        if (aname && *aname)
        {
            char quote, *pquote;
            /* indexed name is not supported for hierarchical match */
            if (hierarchical)
            {
                bdmf_session_print(session, "%s: Indexed name is not supported in hierarchical view\n", name);
                goto obj_show_ret;
            }
            array_index = strsep(&aname, "]");
            pquote = strchr("\'\"(", array_index[0]);
            if (pquote)
            {
                quote = *pquote;
                if (array_index[strlen(array_index) - 1] != quote)
                {
                    bdmf_session_print(session, "%s: Unterminated quote %c in %s\n", name, quote, array_index);
                    goto obj_show_ret;
                }
                ++array_index;
                array_index[strlen(array_index)-1] = 0;
            }
        }
    }

    /* Finally, examine object(s) */
    if (obj)
    {
        nfound = _bdmf_mon_object_examine(session,
                     obj, name, array_index,
                     class_level, nzstat, hierarchical, format, max_prints);
    }
    else
    {
        /* iterate all objects of the given type */
        while((obj=bdmf_get_next(drv, obj, NULL)))
        {
            nfound += _bdmf_mon_object_examine(session,
                         obj, name, array_index,
                         class_level, nzstat, hierarchical, format, max_prints);
        }
    }

    rc = 0;
    if (name && !nfound)
    {
        bdmf_session_print(session, "%s: Attribute is not supported by object type %s\n",
                   name, obj ? obj->drv->name : type_discr);
        rc = BDMF_ERR_NOENT;
    }

obj_show_ret:
    if (obj)
        bdmf_put(obj);
    if (drv)
        bdmf_type_put(drv);

    return rc;
}

static int _bdmf_display_aggr_type(bdmf_session_handle session, const struct bdmf_aggr_type *at)
{
    const struct bdmf_attr *attrs=at->fields;
    char *tmp_buf;

    tmp_buf = bdmf_alloc(OBJHLP_TMPBUF_SIZE);
    if (!tmp_buf)
        return BDMF_ERR_NOMEM;

    bdmf_session_print(session, "%-16s : %s\n", at->name, at->help ? at->help : "");
    bdmf_session_print(session, "Fields: (M-mandatory)\n");
    while(attrs && attrs->name)
    {
        bdmf_attr_help(attrs, tmp_buf, OBJHLP_TMPBUF_SIZE);
        bdmf_session_print(session, "   %s\n", tmp_buf);
        ++attrs;
    }
    bdmf_session_print(session, "\n");
    bdmf_free(tmp_buf);
    return 0;
}

/* Aggregate attribute types
    BDMFMON_MAKE_PARM("name",  "aggregate type name. not set=all", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
*/
static int bdmf_mon_aggr_types(bdmf_session_handle session,
                               const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *name=parm[0].value.string;
    const struct bdmf_aggr_type *at = NULL;
    int rc=BDMF_ERR_NOENT;
    
    /* Display all with matching name */
    while((at=bdmf_attr_aggregate_type_get_next(at)))
    {
        if (!name || strstr(at->name, name))
            rc = _bdmf_display_aggr_type(session, at);
    }

    return rc;
}


/* Display/Set trace level
    BDMFMON_MAKE_PARM_ENUM("level", "new trace level", trace_level_table, BDMFMON_PARM_FLAG_OPTIONAL),
    BDMFMON_MAKE_PARM("type",  "object type", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
*/
static int bdmf_mon_trace_level(bdmf_session_handle session,
                               const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    bdmf_trace_level_t level = (bdmf_trace_level_t)parm[0].value.number;
    char *type=parm[1].value.string;

    struct bdmf_type *drv=NULL;
    int rc=BDMF_ERR_NOENT;

    /* Type-specific trace level request ? */
    if (bdmfmon_parm_is_set(session, 1))
    {
        rc = bdmf_type_find_get(type, &drv);
        if (rc)
        {
            bdmf_session_print(session, "Type %s is not registered\n", type);
            return rc;
        }
    }

    /* Display or set ? */
    if (bdmfmon_parm_is_set(session, 0))
        bdmf_trace_level_set(drv, level);
    else
    {
        level = bdmf_trace_level(drv);
        if (drv)
            bdmf_session_print(session, "Trace level on type %s is %s\n",
                    type, bdmfmon_enum_parm_stringval(session, 0, level));
        else
            bdmf_session_print(session, "Global trace level is %s\n",
                    bdmfmon_enum_parm_stringval(session, 0, level));
    }
    return 0;
}

static int _bdmf_mon_is_aggr_present(const struct bdmf_aggr_type *at,
    const struct bdmf_aggr_type *ref_aggrs[], int naggrs)
{
    int i;
    for (i=0; i<naggrs; i++)
    {
        if (ref_aggrs[i] == at)
            return 1;
    }
    return 0;
}

static int _bdmf_mon_get_ref_aggrs(const struct bdmf_attr *attrs,
    const struct bdmf_aggr_type *ref_aggrs[], int naggrs, int max_aggrs)
{
    const struct bdmf_attr *a = attrs;
    while(a && a->name)
    {
        if (a->type == bdmf_attr_aggregate && a->aggr_type &&
            !_bdmf_mon_is_aggr_present(a->aggr_type, ref_aggrs, naggrs))
        {
            ref_aggrs[naggrs++] = a->aggr_type;
            naggrs = _bdmf_mon_get_ref_aggrs(a->aggr_type->fields, ref_aggrs, naggrs, max_aggrs);
            if (naggrs >= max_aggrs)
                break;
        }
        if (a->index_type == bdmf_attr_aggregate && a->index_aggr_type &&
            !_bdmf_mon_is_aggr_present(a->index_aggr_type, ref_aggrs, naggrs))
        {
            ref_aggrs[naggrs++] = a->index_aggr_type;
            naggrs = _bdmf_mon_get_ref_aggrs(a->index_aggr_type->fields, ref_aggrs, naggrs, max_aggrs);
            if (naggrs >= max_aggrs)
                break;
        }
        ++a;
    }
    return naggrs;
}

/* Display detailed object type help
    BDMFMON_MAKE_PARM("type",  "object type", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
*/
static int bdmf_mon_object_type_help(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type=parm[0].value.string;
    struct bdmf_type *drv;
#define BDMF_MON_MAX_REF_AGGRS 64
    const struct bdmf_aggr_type *ref_aggrs[BDMF_MON_MAX_REF_AGGRS];
    int naggrs = 0;
    int i;

    int rc;

    rc = bdmf_type_find_get(type, &drv);
    if (rc)
    {
        bdmf_session_print(session, "Type %s is not registered\n", type);
        return rc;
    }
    _bdmf_obj_help(session, drv, NULL);
    bdmf_session_print(session, "\n");

    /* Find all referenced aggregates */
    naggrs = _bdmf_mon_get_ref_aggrs(drv->aattr, ref_aggrs, naggrs, BDMF_MON_MAX_REF_AGGRS);
    for(i=0; i<naggrs; i++)
        _bdmf_display_aggr_type(session, ref_aggrs[i]);
    bdmf_type_put(drv);

    return 0;
}

/*
 * mattr testing
 */
static bdmf_mattr_handle bdmf_mon_mattr;
#define MAX_MATTR_STRING_SIZE   64
#define MAX_MATTR_STRINGS       8
static char bdmf_mon_mattr_str[MAX_MATTR_STRINGS][MAX_MATTR_STRING_SIZE];
static int bdmf_mon_mattr_nstr;

/* bdmf_new_and_set
    BDMFMON_MAKE_PARM("obj_type",   "type", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_mattr_init(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *obj_type=parm[0].value.string;
    char *attrs;
    struct bdmf_type *drv;
    int rc;

    if (bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block is already allocated. Release first\n");
        return BDMF_ERR_ALREADY;
    }
    rc = _bdmf_get_type_attrs(session, obj_type, &drv, &attrs);
    if (rc || !drv)
        return rc;
    bdmf_mon_mattr = bdmf_mattr_alloc(drv);
    bdmf_type_put(drv);
    if (!bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block allocation failed\n");
        return BDMF_ERR_NOMEM;
    }
    return 0;
}


/* Add set_attr request to mattr
    BDMFMON_MAKE_PARM("attr",  "Attribute id", BDMFMON_PARM_NUMBER, 0),
    BDMFMON_MAKE_PARM("index", "Array index", BDMFMON_PARM_NUMBER, 0),
    BDMFMON_MAKE_PARM_ENUM("add_as", "Add as", as_what_enum_table, 0),
    BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_mattr_add(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    bdmf_attr_id aid = (bdmf_attr_id)parm[0].value.number;
    bdmf_index index = (bdmf_index)parm[1].value.number;
    bdmf_attr_type_t as = (bdmf_attr_type_t)parm[2].value.number;
    char *sval = (char *)parm[3].value.string;
    int rc;
    if (!bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block is not allocated. Init first\n");
        return BDMF_ERR_PARM;
    }
    if (as == bdmf_attr_number)
    {
        bdmf_number val = strtol(sval, NULL, 0);
        rc = bdmf_attrelem_set_as_num(bdmf_mon_mattr, aid, index, val);
    }
    else if (as == bdmf_attr_string)
    {
        if (bdmf_mon_mattr_nstr >= MAX_MATTR_STRINGS)
        {
            bdmf_session_print(session, "mattr: ran out of buffer strings\n");
            return BDMF_ERR_NOMEM;
        }
        strncpy(bdmf_mon_mattr_str[bdmf_mon_mattr_nstr], sval, MAX_MATTR_STRING_SIZE);
        rc = bdmf_attrelem_set_as_string(bdmf_mon_mattr, aid, index, bdmf_mon_mattr_str[bdmf_mon_mattr_nstr]);
        ++bdmf_mon_mattr_nstr;
    }
    else
    {
        bdmf_session_print(session, "mattr: set_as_buf is not supported by CLI\n");
        return BDMF_ERR_NOT_SUPPORTED;
    }
    return rc;
}

/* bdmf_mattr_set
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_mattr_set(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type_attrs = (char *)parm[0].value.string;

    struct bdmf_object *mo;
    int rc;
    if (!bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block is not allocated. Init first\n");
        return BDMF_ERR_PARM;
    }
    rc = _bdmf_get_object(session, type_attrs, &mo);
    if (rc)
        return rc;
    rc = bdmf_mattr_set(mo, bdmf_mon_mattr);
    bdmf_session_print(session, "bdmf_matr_set(%s, mattr)=%d (%s)\n",
        mo->name, rc, bdmf_strerror(rc));
    bdmf_put(mo);
    return rc;
}

/* bdmf_new_and_set
    BDMFMON_MAKE_PARM("obj_type",   "type", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("parent","type/attributes", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
*/
static int bdmf_mon_mattr_new(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *type_attrs=parm[0].value.string;
    char *parent_attrs=parm[1].value.string;
    char *attrs;
    struct bdmf_type *drv;
    struct bdmf_object *obj;
    struct bdmf_object *parent=NULL;
    int rc;
    if (!bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block is not allocated. Init first\n");
        return BDMF_ERR_PARM;
    }
    if (parent_attrs)
    {
        rc = _bdmf_get_object(session, parent_attrs, &parent);
        if (rc)
            return rc;
    }
    rc = _bdmf_get_type_attrs(session, type_attrs, &drv, &attrs);
    if (rc || !drv)
        return rc;
    rc = bdmf_new_and_set(drv, parent, bdmf_mon_mattr, &obj);
    if (rc)
        bdmf_session_print(session, "# Object creation failed: %s\n", bdmf_strerror(rc));
    else
        bdmf_session_print(session, "# Created object <%s>\n", obj->name);
    bdmf_type_put(drv);
    if (parent)
        bdmf_put(parent);
    return rc;
}

/* bdmf_mattr_free
*/
static int bdmf_mon_mattr_free(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    if (!bdmf_mon_mattr)
    {
        bdmf_session_print(session, "mattr block is not allocated. Init first\n");
        return BDMF_ERR_PARM;
    }
    bdmf_mattr_free(bdmf_mon_mattr);
    bdmf_mon_mattr = NULL;
    bdmf_mon_mattr_nstr = 0;
    return 0;
}

/*
 * Attributes directory
 */

static int _bdmf_mon_obj_attr(bdmf_session_handle session, char *obj_name, char *attr_name,
    bdmf_object_handle *pmo, bdmf_attr_id *paid, bdmf_index *pindex, void **index_buf, char **psindex)
{
    int rc;
    char *pi = attr_name;
    void *ib = NULL;
    struct bdmf_attr *attr;

    *index_buf = NULL;
    *psindex = NULL;

    rc = _bdmf_get_object(session, obj_name, pmo);
    if (rc)
        return rc;

    strsep(&pi, "["); /* Find start of index if any */

    /* Look-up attribute */
    rc = bdmf_attr_by_name((*pmo)->drv, attr_name, paid);
    if (rc)
    {
        bdmf_session_print(session, "## object %s doesn't support attribute %s\n", obj_name, attr_name);
        goto cleanup;
    }

    attr = &((*pmo)->drv->aattr[*paid]);

    /* Allocate index buffer if attribute supports index or CLI expression is indexed */
    if ((pi && *pi) || attr->array_size || attr->add || attr->delete)
    {
        int is_numeric_index = bdmf_attr_type_is_numeric(attr->index_type);

        ib = bdmf_alloc(attr->index_size);
        if (!ib)
        {
            rc = BDMF_ERR_NOMEM;
            goto cleanup;
        }
        *index_buf = ib;
        *(bdmf_index *)ib = BDMF_INDEX_UNASSIGNED; /* safe because index_size is always >= long */

        /* Indexed expression ? */
        if (pi && *pi)
        {
            char *sindex = strsep(&pi, "]");

            if (!pi)
            {
                bdmf_session_print(session, "## ] is missing after %s[%s\n", attr_name, sindex);
                rc = BDMF_ERR_PARM;
                goto cleanup;
            }
            rc = bdmf_attr_string_to_array_index(*pmo, attr, sindex, ib);
            if (rc < 0)
            {
                bdmf_session_print(session, "Array index is invalid for %s[%s] on %s\n",
                           attr->name, sindex, (*pmo)->name);
                return BDMF_ERR_PARM;
            }
            *psindex = sindex;
        }
        /* Index is either number of index structure pointer */
        *pindex = is_numeric_index ? *(bdmf_index *)ib : (bdmf_index)ib;
    }

cleanup:
    if (rc)
    {
        bdmf_put(*pmo);
        if (*index_buf)
        {
            bdmf_free(*index_buf);
            *index_buf = NULL;
        }
    }
    return rc;
}


/* Set/Add attribute
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
    BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
*/
static int _bdmf_mon_attr_set_add(int is_add, bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *obj_name = (char *)parm[0].value.string;
    char *attr_name = (char *)parm[1].value.string;
    bdmf_attr_type_t as = (bdmf_attr_type_t)parm[2].value.number;
    char *sval = (char *)parm[3].value.string;

    bdmf_object_handle mo;
    bdmf_attr_id aid;
    bdmf_index index = BDMF_INDEX_UNASSIGNED;
    bdmf_index *pindex_add;
    struct bdmf_attr *attr;
    int rc;
    void *index_buf;
    int is_numeric_index;
    char *sindex;

    rc = _bdmf_mon_obj_attr(session, obj_name, attr_name, &mo, &aid, &index, &index_buf, &sindex);
    if (rc)
        return rc;

    attr = &mo->drv->aattr[aid];
    is_numeric_index = bdmf_attr_type_is_numeric(attr->index_type);
    if (is_numeric_index)
        pindex_add = &index;
    else
        pindex_add = (bdmf_index *)index_buf;

    if (as == bdmf_attr_number)
    {
        bdmf_number val = strtol(sval, NULL, 0);
        if (is_add)
            rc = bdmf_attrelem_add_as_num(mo, aid, pindex_add, val);
        else
            rc = bdmf_attrelem_set_as_num(mo, aid, index, val);
    }
    else if (as == bdmf_attr_string)
    {
        if (is_add)
            rc = bdmf_attrelem_add_as_string(mo, aid, pindex_add, sval);
        else
            rc = bdmf_attrelem_set_as_string(mo, aid, index, sval);
    }
    else
    {
        uint8_t buf[(strlen(sval)+1) / 2];
        uint8_t *pbuf = buf;
        if (!strcmp(sval, "null"))
            pbuf = NULL;
        else
            rc = bdmf_strhex(sval, buf, sizeof(buf));
        if (is_add)
            rc = (rc < 0) ? rc : bdmf_attrelem_add_as_buf(mo, aid, pindex_add, pbuf, rc);
        else
            rc = (rc < 0) ? rc : bdmf_attrelem_set_as_buf(mo, aid, index, pbuf, rc);
    }
    if (is_add && (rc >= 0))
    {
        char index_sval[256]="invalid";
        bdmf_attr_array_index_to_string(mo, attr, index, index_sval, sizeof(index_sval));
        bdmf_session_print(session, "## %s: added %s[%s]\n", obj_name, attr_name, index_sval);
    }
    bdmf_put(mo);
    if (index_buf)
        bdmf_free(index_buf);
    return (rc < 0) ? rc : 0;
}

/* Set attribute
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
    BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_attr_set(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    return _bdmf_mon_attr_set_add(0, session, parm, n_parms);
}

/* Add attribute
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
    BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_attr_add(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    return _bdmf_mon_attr_set_add(1, session, parm, n_parms);
}

/* Get attribute
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
*/
static int bdmf_mon_attr_get(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *obj_name = (char *)parm[0].value.string;
    char *attr_name = (char *)parm[1].value.string;
    bdmf_attr_type_t as = (bdmf_attr_type_t)parm[2].value.number;

    bdmf_object_handle mo;
    bdmf_attr_id aid;
    bdmf_index index = -1;
    int rc;
    void *index_buf;
    char *sindex;

    rc = _bdmf_mon_obj_attr(session, obj_name, attr_name, &mo, &aid, &index, &index_buf, &sindex);
    if (rc)
        return rc;

    if (as == bdmf_attr_number)
    {
        bdmf_number val;
        rc = bdmf_attrelem_get_as_num(mo, aid, index, &val);
        if (!rc)
        {
            bdmf_session_print(session, "## %s: %s", obj_name, attr_name);
            if (sindex)
                bdmf_session_print(session, "[%s]", sindex);
            bdmf_session_print(session, " = %lld\n", val);
        }
    }
    else if (as == bdmf_attr_string)
    {
        char *sval;

        sval = bdmf_alloc(BDMFMON_MAX_SVAL_SIZE);
        if (!sval)
            return BDMF_ERR_NOMEM;
        rc = bdmf_attrelem_get_as_string(mo, aid, index, sval, BDMFMON_MAX_SVAL_SIZE);
        if (rc >= 0)
        {
            bdmf_session_print(session, "## %s: %s", obj_name, attr_name);
            if (sindex)
                bdmf_session_print(session, "[%s]", sindex);
            _bdmf_print_long_val(session, sval);
            bdmf_session_print(session, "\n");
        }
        bdmf_free(sval);
    }
    else
    {
        char bval[mo->drv->aattr[aid].size];
        rc = bdmf_attrelem_get_as_buf(mo, aid, index, bval, sizeof(bval));
        if (rc >= 0)
        {
            bdmf_session_print(session, "## %s: %s", obj_name, attr_name);
            if (sindex)
                bdmf_session_print(session, "[%s]", sindex);
            bdmf_session_print(session, " =\n");
            bdmf_session_hexdump(session, bval, 0, rc);
        }
    }
    bdmf_put(mo);
    if (index_buf)
        bdmf_free(index_buf);
    return (rc < 0) ? rc : 0;
}

/* Get attribute
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_attr_delete(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *obj_name = (char *)parm[0].value.string;
    char *attr_name = (char *)parm[1].value.string;

    bdmf_object_handle mo;
    bdmf_attr_id aid;
    bdmf_index index = -1;
    int rc;
    void *index_buf;
    char *sindex;

    rc = _bdmf_mon_obj_attr(session, obj_name, attr_name, &mo, &aid, &index, &index_buf, &sindex);
    if (rc)
        return rc;
    rc = bdmf_attrelem_delete(mo, aid, index);
    bdmf_put(mo);
    if (index_buf)
        bdmf_free(index_buf);
    return rc;
}

/* Find attribute array entry
    BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
    BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
*/
static int bdmf_mon_attr_find(bdmf_session_handle session,
    const bdmfmon_cmd_parm_t parm[],  uint16_t n_parms)
{
    char *obj_name = (char *)parm[0].value.string;
    char *attr_name = (char *)parm[1].value.string;
    char *value = (char *)parm[2].value.string;

    bdmf_object_handle mo;
    bdmf_attr_id aid;
    bdmf_index index = -1;
    int rc;
    void *index_buf;
    char *sindex;
    bdmf_index *pindex_find;
    struct bdmf_attr *attr;
    int is_numeric_index;

    rc = _bdmf_mon_obj_attr(session, obj_name, attr_name, &mo, &aid, &index, &index_buf, &sindex);
    if (rc)
        return rc;

    attr = &mo->drv->aattr[aid];
    is_numeric_index = bdmf_attr_type_is_numeric(attr->index_type);
    if (is_numeric_index)
        pindex_find = &index;
    else
        pindex_find = (bdmf_index *)index_buf;

    rc = bdmf_attrelem_find_by_string(mo, aid, pindex_find, value);
    if (rc)
    {
        bdmf_session_print(session, "## find %s/%s - bdmf_attrelem_find_by_string() returned error %d (%s)\n",
            obj_name, attr_name, rc, bdmf_strerror(rc));
    }
    else
    {
        char index_sval[256]="invalid";
        bdmf_attr_array_index_to_string(mo, attr, index, index_sval, sizeof(index_sval));
        bdmf_session_print(session, "## found %s/%s[%s] == %s\n", obj_name, attr_name, index_sval, value);
    }
    bdmf_put(mo);
    if (index_buf)
        bdmf_free(index_buf);
    return rc;
}

/* Create flow monitor directory in root_dir
   Returns the "flow" directory handle
*/
bdmfmon_handle_t bdmf_flow_mon_init(bdmfmon_handle_t root_dir)
{
    bdmfmon_handle_t bdmf_dir, mattr_dir, attr_dir;
    static bdmfmon_enum_val_t as_what_enum_table[] = {
        { .name="num",     .val=bdmf_attr_number },
        { .name="string",  .val=bdmf_attr_string },
        { .name="buf",     .val=bdmf_attr_buffer },
        BDMFMON_ENUM_LAST
    };

    if ((bdmf_dir=bdmfmon_dir_find(NULL, "bdmf"))!=NULL)
        return bdmf_dir;

    bdmf_dir = bdmfmon_dir_add(root_dir, "bdmf",
                             "Broadlight Device Management Framework",
                             BDMF_ACCESS_GUEST, NULL);

    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes | type/?", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("parent","type/attributes", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "new", bdmf_mon_object_new,
                      "Create a new object",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/discriminator", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "delete", bdmf_mon_object_delete,
                      "Delete object",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/discriminator | type/?", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attrs", "attributes", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_EOL),
            BDMFMON_PARM_LIST_TERMINATOR
        };

        bdmfmon_cmd_add(bdmf_dir, "configure", bdmf_mon_object_configure,
                      "Configure object",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("ds_obj", "ds_type/discriminator", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("us_obj", "us_type/discriminator", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attrs",  "link attributes",     BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "link", bdmf_mon_object_link,
                      "Link objects",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("ds_obj", "ds_type/discriminator", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("us_obj", "us_type/discriminator", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "unlink", bdmf_mon_object_unlink,
                      "Unlink objects",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        bdmfmon_cmd_add(bdmf_dir, "types", bdmf_mon_types,
                      "List registered object types",
                      BDMF_ACCESS_GUEST, NULL, NULL);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("type",   "object type [/attributes]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM_ENUM_DEFVAL("children", "Hierarchical children view", bdmfmon_enum_bool_table,
                            BDMFMON_PARM_FLAG_OPTIONAL, "yes"),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "objects", bdmf_mon_objects,
                      "List objects of the given type",
                      BDMF_ACCESS_GUEST, NULL, parms);
    }
    {
        static bdmfmon_enum_val_t attr_class_table[] = {
            { .name="config", .val=BDMF_ATTR_CONFIG},         /* Configuration */
            { .name="stat",   .val=BDMF_ATTR_STAT},           /* Statistic */
            { .name="nzstat", .val=BDMF_ATTR_STAT | BDMF_ATTR_CONFIG}, /* Non-zero statistic */
            BDMFMON_ENUM_LAST
        };
        static bdmfmon_enum_val_t attr_level_table[] = {
            { .name="major",  .val=BDMF_ATTR_MAJOR},          /* Major only */
            { .name="minor",  .val=BDMF_ATTR_MAJOR | BDMF_ATTR_MINOR }, /* Major+minor */
            { .name="debug",  .val=BDMF_ATTR_MAJOR | BDMF_ATTR_MINOR | BDMF_ATTR_HIDDEN }, /* all */
            BDMFMON_ENUM_LAST
        };
        static bdmfmon_enum_val_t attr_format_table[] = {
            { .name="input",   .val=BDMFMON_EXAMINE_AG_FORMAT_INPUT},    /* The same output format as input format */
            { .name="space",   .val=BDMFMON_EXAMINE_AG_FORMAT_SPACE },   /* Similar to input format, with extra space between fields */
            { .name="line",    .val=BDMFMON_EXAMINE_AG_FORMAT_LINE },    /* Each field in a separate line */
            BDMFMON_ENUM_LAST
        };
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type[/discriminator]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "attribute name or substring", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_MAKE_PARM_ENUM("class", "Attribute class", attr_class_table, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_MAKE_PARM_ENUM("level", "Attribute level", attr_level_table, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_MAKE_PARM_ENUM_DEFVAL("children", "Hierarchical children view", bdmfmon_enum_bool_table,
                            BDMFMON_PARM_FLAG_OPTIONAL, "no"),
            BDMFMON_MAKE_PARM_ENUM_DEFVAL("format", "Aggregate output format", attr_format_table,
                            BDMFMON_PARM_FLAG_OPTIONAL, "input"),
            BDMFMON_MAKE_PARM_DEFVAL("max_prints", "Max number of array elements to print", BDMFMON_PARM_NUMBER,
                            0, BDMF_MON_MAX_ATTRELEM_SHOW),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(bdmf_dir, "Examine", bdmf_mon_object_examine,
                      "Examine object",
                      BDMF_ACCESS_GUEST, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("name",  "aggregate type name. not set=all", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        
        bdmfmon_cmd_add(bdmf_dir, "aggregates", bdmf_mon_aggr_types,
                      "Aggregate attribute types",
                      BDMF_ACCESS_GUEST, NULL, parms);
    }

    {
        static bdmfmon_enum_val_t trace_level_table[] = {
            { .name="none",  .val=bdmf_trace_level_none},   /* None */
            { .name="error", .val=bdmf_trace_level_error }, /* Errors only */
            { .name="info",  .val=bdmf_trace_level_info },  /* Errors and Info */
            { .name="debug", .val=bdmf_trace_level_debug }, /* Everything */
            BDMFMON_ENUM_LAST
        };
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM_ENUM("level", "new trace level", trace_level_table, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_MAKE_PARM("type",  "object type", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_PARM_LIST_TERMINATOR
        };

        bdmfmon_cmd_add(bdmf_dir, "trace", bdmf_mon_trace_level,
                      "Display/Set trace level",
                      BDMF_ACCESS_GUEST, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("type",  "Object type", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(bdmf_dir, "help", bdmf_mon_object_type_help,
                      "Detailed object type help",
                      BDMF_ACCESS_GUEST, NULL, parms);
    }

    mattr_dir = bdmfmon_dir_add(bdmf_dir, "mattr",
                             "mattr (attribute groups) support",
                             BDMF_ACCESS_ADMIN, NULL);
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj_type",   "type", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(mattr_dir, "init", bdmf_mon_mattr_init,
                      "Allocate mattr descriptor",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("attr",  "Attribute id", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM("index", "Array index", BDMFMON_PARM_NUMBER, 0),
            BDMFMON_MAKE_PARM_ENUM("add_as", "Add as", as_what_enum_table, 0),
            BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(mattr_dir, "add", bdmf_mon_mattr_add,
                      "Add attribute to mattr descriptor",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(mattr_dir, "set", bdmf_mon_mattr_set,
                      "bdmf_mattr_set()",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj_type",   "type", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("parent","type/attributes", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_OPTIONAL),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(mattr_dir, "new", bdmf_mon_mattr_new,
                      "bdmf_new_and_set()",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        bdmfmon_cmd_add(mattr_dir, "free", bdmf_mon_mattr_free,
                      "bdmf_mattr_free()",
                      BDMF_ACCESS_ADMIN, NULL, NULL);
    }

    attr_dir = bdmfmon_dir_add(bdmf_dir, "attribute",
                             "Set/Get/Add/Delete individual attributes",
                             BDMF_ACCESS_ADMIN, NULL);
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
            BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, BDMFMON_PARM_FLAG_EOL),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(attr_dir, "set", bdmf_mon_attr_set,
                      "Set attribute",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(attr_dir, "get", bdmf_mon_attr_get,
                      "Get attribute",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM_ENUM("as", "Value format", as_what_enum_table, 0),
            BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(attr_dir, "add", bdmf_mon_attr_add,
                      "Add attribute array entry",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(attr_dir, "delete", bdmf_mon_attr_delete,
                      "Delete attribute array entry",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }
    {
        static bdmfmon_cmd_parm_t parms[]={
            BDMFMON_MAKE_PARM("obj",   "type/attributes", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("attr",  "Attribute name[index]", BDMFMON_PARM_STRING, 0),
            BDMFMON_MAKE_PARM("val", "Value", BDMFMON_PARM_STRING, 0),
            BDMFMON_PARM_LIST_TERMINATOR
        };
        bdmfmon_cmd_add(attr_dir, "find", bdmf_mon_attr_find,
                      "Find attribute array entry",
                      BDMF_ACCESS_ADMIN, NULL, parms);
    }

    return bdmf_dir;
}

void bdmf_flow_mon_exit(void)
{
    bdmfmon_handle_t bdmf_dir;
    bdmf_dir=bdmfmon_dir_find(NULL, "bdmf");
    if (bdmf_dir)
        bdmfmon_token_destroy(bdmf_dir);
}

