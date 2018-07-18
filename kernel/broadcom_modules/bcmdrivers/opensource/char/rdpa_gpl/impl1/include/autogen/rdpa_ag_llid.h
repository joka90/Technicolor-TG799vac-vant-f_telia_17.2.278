// <:copyright-BRCM:2013:DUAL/GPL:standard
// 
//    Copyright (c) 2013 Broadcom Corporation
//    All Rights Reserved
// 
// Unless you and Broadcom execute a separate written software license
// agreement governing use of this software, this software is licensed
// to you under the terms of the GNU General Public License version 2
// (the "GPL"), available at http://www.broadcom.com/licenses/GPLv2.php,
// with the following added to such license:
// 
//    As a special exception, the copyright holders of this software give
//    you permission to link this software with independent modules, and
//    to copy and distribute the resulting executable under terms of your
//    choice, provided that you also meet, for each linked independent
//    module, the terms and conditions of the license of that module.
//    An independent module is a module which is not derived from this
//    software.  The special exception does not apply to any modifications
//    of the software.
// 
// Not withstanding the above, under no circumstances may you combine
// this software in any way with any other Broadcom software provided
// under a license other than the GPL, without Broadcom's express prior
// written consent.
// 
// :>
/*
 * llid object header file.
 * This header file is generated automatically. Do not edit!
 */
#ifndef _RDPA_AG_LLID_H_
#define _RDPA_AG_LLID_H_

/** \addtogroup llid
 * @{
 */


/** Get llid type handle.
 *
 * This handle should be passed to bdmf_new_and_set() function in
 * order to create a llid object.
 * \return llid type handle
 */
bdmf_type_handle rdpa_llid_drv(void);

/* llid: Attribute types */
typedef enum {
    rdpa_llid_attr_index = 0, /* index : KRI : number/4 : LLID index */
    rdpa_llid_attr_egress_tm = 2, /* egress_tm : RW : object/4 : US data scheduler object */
    rdpa_llid_attr_control_egress_tm = 3, /* control_egress_tm : RW : object/4 : US control scheduler object */
    rdpa_llid_attr_control_enable = 4, /* control_enable : RW : bool/1 : Enable LLID control channel */
    rdpa_llid_attr_data_enable = 5, /* data_enable : RW : bool/1 : Enable LLID data channels */
    rdpa_llid_attr_ds_def_flow = 7, /* ds_def_flow : RW : aggregate/56 classification_result(rdpa_ic_result_t ) : downstream default flow configuration */
    rdpa_llid_attr_port_action = 8, /* port_action : RWF : aggregate/5[49(rdpa_if)] llid_port_action(rdpa_llid_port_action_t) : Per port vlan action configuration */
} rdpa_llid_attr_types;

extern int (*f_rdpa_llid_get)(bdmf_number index_, bdmf_object_handle *pmo);

/** Get llid object by key.

 * This function returns llid object instance by key.
 * \param[in] index_    Object key
 * \param[out] llid_obj    Object handle
 * \return    0=OK or error <0
 */
int rdpa_llid_get(bdmf_number index_, bdmf_object_handle *llid_obj);

/** Get llid/index attribute.
 *
 * Get LLID index.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_llid_index_get(bdmf_object_handle mo_, bdmf_number *index_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_llid_attr_index, &_nn_);
    *index_ = (bdmf_number)_nn_;
    return _rc_;
}


/** Set llid/index attribute.
 *
 * Set LLID index.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   index_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task and softirq contexts.
 */
static inline int rdpa_llid_index_set(bdmf_object_handle mo_, bdmf_number index_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_llid_attr_index, index_);
}


/** Get llid/egress_tm attribute.
 *
 * Get US data scheduler object.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  egress_tm_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_egress_tm_get(bdmf_object_handle mo_, bdmf_object_handle *egress_tm_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_llid_attr_egress_tm, &_nn_);
    *egress_tm_ = (bdmf_object_handle)(long)_nn_;
    return _rc_;
}


/** Set llid/egress_tm attribute.
 *
 * Set US data scheduler object.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   egress_tm_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_egress_tm_set(bdmf_object_handle mo_, bdmf_object_handle egress_tm_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_llid_attr_egress_tm, (long)egress_tm_);
}


/** Get llid/control_egress_tm attribute.
 *
 * Get US control scheduler object.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  control_egress_tm_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_control_egress_tm_get(bdmf_object_handle mo_, bdmf_object_handle *control_egress_tm_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_llid_attr_control_egress_tm, &_nn_);
    *control_egress_tm_ = (bdmf_object_handle)(long)_nn_;
    return _rc_;
}


/** Set llid/control_egress_tm attribute.
 *
 * Set US control scheduler object.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   control_egress_tm_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_control_egress_tm_set(bdmf_object_handle mo_, bdmf_object_handle control_egress_tm_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_llid_attr_control_egress_tm, (long)control_egress_tm_);
}


/** Get llid/control_enable attribute.
 *
 * Get Enable LLID control channel.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  control_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_control_enable_get(bdmf_object_handle mo_, bdmf_boolean *control_enable_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_llid_attr_control_enable, &_nn_);
    *control_enable_ = (bdmf_boolean)_nn_;
    return _rc_;
}


/** Set llid/control_enable attribute.
 *
 * Set Enable LLID control channel.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   control_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_control_enable_set(bdmf_object_handle mo_, bdmf_boolean control_enable_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_llid_attr_control_enable, control_enable_);
}


/** Get llid/data_enable attribute.
 *
 * Get Enable LLID data channels.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  data_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_data_enable_get(bdmf_object_handle mo_, bdmf_boolean *data_enable_)
{
    bdmf_number _nn_;
    int _rc_;
    _rc_ = bdmf_attr_get_as_num(mo_, rdpa_llid_attr_data_enable, &_nn_);
    *data_enable_ = (bdmf_boolean)_nn_;
    return _rc_;
}


/** Set llid/data_enable attribute.
 *
 * Set Enable LLID data channels.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   data_enable_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_data_enable_set(bdmf_object_handle mo_, bdmf_boolean data_enable_)
{
    return bdmf_attr_set_as_num(mo_, rdpa_llid_attr_data_enable, data_enable_);
}


/** Get llid/ds_def_flow attribute.
 *
 * Get downstream default flow configuration.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[out]  ds_def_flow_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_ds_def_flow_get(bdmf_object_handle mo_, rdpa_ic_result_t  * ds_def_flow_)
{
    return bdmf_attr_get_as_buf(mo_, rdpa_llid_attr_ds_def_flow, ds_def_flow_, sizeof(*ds_def_flow_));
}


/** Set llid/ds_def_flow attribute.
 *
 * Set downstream default flow configuration.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   ds_def_flow_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_ds_def_flow_set(bdmf_object_handle mo_, const rdpa_ic_result_t  * ds_def_flow_)
{
    return bdmf_attr_set_as_buf(mo_, rdpa_llid_attr_ds_def_flow, ds_def_flow_, sizeof(*ds_def_flow_));
}


/** Get llid/port_action attribute entry.
 *
 * Get Per port vlan action configuration.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[out]  port_action_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_port_action_get(bdmf_object_handle mo_, rdpa_if ai_, rdpa_llid_port_action_t * port_action_)
{
    return bdmf_attrelem_get_as_buf(mo_, rdpa_llid_attr_port_action, (bdmf_index)ai_, port_action_, sizeof(*port_action_));
}


/** Set llid/port_action attribute entry.
 *
 * Set Per port vlan action configuration.
 * \param[in]   mo_ llid object handle or mattr transaction handle
 * \param[in]   ai_ Attribute array index
 * \param[in]   port_action_ Attribute value
 * \return 0 or error code < 0
 * The function can be called in task context only.
 */
static inline int rdpa_llid_port_action_set(bdmf_object_handle mo_, rdpa_if ai_, const rdpa_llid_port_action_t * port_action_)
{
    return bdmf_attrelem_set_as_buf(mo_, rdpa_llid_attr_port_action, (bdmf_index)ai_, port_action_, sizeof(*port_action_));
}

/** @} end of llid Doxygen group */




#endif /* _RDPA_AG_LLID_H_ */
