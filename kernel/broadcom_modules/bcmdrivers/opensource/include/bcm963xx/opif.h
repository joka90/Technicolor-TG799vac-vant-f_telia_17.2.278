/************************************************************************
**                                                                     **
**                          TECHNICOLOR                                **
**                                                                     **
************************************************************************/

/************************ COPYRIGHT INFORMATION *************************
**                                                                     **
** This program contains proprietary information which is a trade      **
** secret of TECHNICOLOR and also is protected as an unpublished       **
** work under applicable Copyright laws. Recipient is to retain this   **
** program in confidence and is not permitted to use or make copies    **
** thereof other than as permitted in a written agreement with         **
** TECHNICOLOR.                                                        **
**                                                                     **
************************************************************************/

#ifndef __OPIF_H__
#define __OPIF_H__

#define TRIPLEXER_BANK_SIZE 128

typedef enum 
{
    OP_T_TRI_SP,        /** Triplexer, SourcePhotonics */
    OP_T_TRI_NP,        /** Triplexer, NeoPhotonics */
    OP_T_DI_SP,         /** Diplexer, SourcePhotonics */
    OP_T_DI_NP,         /** Diplexer, NeoPhotonics */
    OP_T_SFP,           /** SFP */
    OP_T_NOTP,          /*  NO Transceiver Present */
    OP_T_NUM
}OpticalType;

typedef enum
{
    OP_BANK_A0,         /** 0xA0 */
    OP_BANK_A2,         /** 0xA2 */
    OP_BANK_NUM
}OpEepromBank;

typedef enum
{
     OP_A0_CPN,			
     OP_A0_VENDOR_NAME,			
     OP_A0_VENDOR_REV,			
     OP_A0_VENDOR_SN,			
     OP_A0_DATE,			
     OP_A0_MONITOR_TYPE,			
     OP_A0_VENDOR_PN,			
     OP_A0_BITRATE_UP,			
     OP_A0_BITRATE_DOWN,			
     OP_A0_WAVELEN_TX,			
     OP_A0_WAVELEN_RX,			
     OP_A0_MINIMUM_RF,			
     OP_A0_VIDEO_INPUT_PWR_LOW,			
     OP_A0_VIDEO_INPUT_PWR_HIGH,			
     OP_A0_RF_TILT,			
     OP_A0_RF_BANDWIDTH,			
     OP_A0_RESPONSIVITY,			
     OP_A0_TRANSC_ID,			
     OP_A0_TRANSC_EXT_ID,			
     OP_A0_TRANSC_TYPE,			
     OP_A0_VENDOR_OUI,			
     OP_A0_CONNECTOR,			
     OP_A0_1X_COPPER_PASSIVE,			
     OP_A0_1X_COPPER_ACTIVE,			
     OP_A0_1X_LX,			
     OP_A0_1X_SX,			
     OP_A0_10G_BASE_SR,			
     OP_A0_10G_BASE_LR,			
     OP_A0_10G_BASE_LRM,			
     OP_A0_10G_BASE_ER,			
     OP_A0_OC48_SHORT_REACH,			
     OP_A0_OC48_INTER_REACH,			
     OP_A0_OC48_LONG_REACH,			
     OP_A0_SONET_REACH_SPEC_BIT2,			
     OP_A0_SONET_REACH_SPEC_BIT1,			
     OP_A0_OC192_SHORT_REACH,			
     OP_A0_ESCON_SMF,
     OP_A0_ESCON_MMF,			
     OP_A0_OC3_SHORT_REACH,			
     OP_A0_OC3_INTER_REACH,			
     OP_A0_OC3_LONG_REACH,			
     OP_A0_OC12_SHORT_REACH,			
     OP_A0_OC12_INTER_REACH,			
     OP_A0_OC12_LONG_REACH,			
     OP_A0_1000BASE_SX,			
     OP_A0_1000BASE_LX,			
     OP_A0_1000BASE_CX,			
     OP_A0_1000BASE_T,			
     OP_A0_100BASE_LX,			
     OP_A0_100BASE_FX,			
     OP_A0_BASE_BX10,			
     OP_A0_BASE_PX,			
     OP_A0_ELECT_INTER_ENCL,			
     OP_A0_LONGWAVE_LASER_LC,			
     OP_A0_SHORTWAVE_LASER,			
     OP_A0_MEDIUM_DIST,			
     OP_A0_LONG_DIST,			
     OP_A0_INTER_DIST,			
     OP_A0_SHORT_DIST,			
     OP_A0_VERYLONG_DIST,			
     OP_A0_PASSIVE_CABLE,			
     OP_A0_ACTIVE_CABLE,			
     OP_A0_LONGWAVE_LASER_LL,
     OP_A0_SHORTWAVE_LASER_SL,			
     OP_A0_SHORTWAVE_LASER_SN,			
     OP_A0_ELECT_INTRA_ENCL,			
     OP_A0_SINGLE_MODE,			
     OP_A0_MULTI_MODE_50UM,			
     OP_A0_MULTI_MODE_625UM,			
     OP_A0_VIDEO_COAX,			
     OP_A0_MINI_COAX,			
     OP_A0_TWIST_PAIR,			
     OP_A0_TWIN_AXIA_PAIR,			
     OP_A0_100MBPS,			
     OP_A0_200MBPS,			
     OP_A0_400MBPS,			
     OP_A0_1600MBPS,			
     OP_A0_800MBPS,			
     OP_A0_1200MBPS,			
     OP_A0_ENC_CODE,			
     OP_A0_RATE_ID,			
     OP_A0_LEN_SFM_KM,			
     OP_A0_LEN_SMF,			
     OP_A0_LEN_50UM,			
     OP_A0_LEN_625UM,			
     OP_A0_LEN_CABLE,			
     OP_A0_LEN_OM3,			
     OP_A0_BR_MAX,			
     OP_A0_BR_MIN,			
     OP_A0_LINR_REC_OUT_IMP,			
     OP_A0_PWR_LVL_DECL_IMP,			
     OP_A0_COOLED_TRANS_DECL_IMP,			
     OP_A0_RX_LOS_IMP,			
     OP_A0_SIG_DETC_IMP,			
     OP_A0_TX_FAULT_IMP,			
     OP_A0_TX_DIS_IMP,			
     OP_A0_RATE_SEL_IMP,			
     OP_A0_ADDR_CHG_REQ,			
     OP_A0_PWR_MEAS_TYPE,
     OP_A0_EXT_CALIB,			
     OP_A0_INT_CALIB,			
     OP_A0_DGT_DIAG_MON_IMP,			
     OP_A0_LEGACY_DIAG_IMP,			
     OP_A0_RATE_SEL_IMP_8431,			
     OP_A0_APP_SEL_IMP,			
     OP_A0_RATE_SEL_MON_IMP,			
     OP_A0_RX_LOS_MON_IMP,			
     OP_A0_TX_FAULT_MON_IMP,			
     OP_A0_TX_DIS_MON_IMP,			
     OP_A0_ALMWRN_ALL_IMP,			
     OP_A0_SFF8472_COMPLI,			
     OP_A0_CC_BASE,			
     OP_A0_CC_EXT,			

     //alarms & warnings			
     OP_A2_THRESH_TEMP_ALARM_HI,			
     OP_A2_THRESH_TEMP_ALARM_LO,			
     OP_A2_THRESH_TEMP_WARN_HI,			
     OP_A2_THRESH_TEMP_WARN_LO,			
     OP_A2_THRESH_VCC_ALARM_HI,			
     OP_A2_THRESH_VCC_ALARM_LO,			
     OP_A2_THRESH_VCC_WARN_HI,			
     OP_A2_THRESH_VCC_WARN_LO,			
     OP_A2_THRESH_VPDMON_ALARM_HI,			
     OP_A2_THRESH_VPDMON_ALARM_LO,			
     OP_A2_THRESH_VPDMON_WARN_HI,			
     OP_A2_THRESH_VPDMON_WARN_LO,			
     OP_A2_THRESH_RFMON_ALARM_HI,			
     OP_A2_THRESH_RFMON_ALARM_LO,			
     OP_A2_THRESH_RFMON_WARN_HI,			
     OP_A2_THRESH_RFMON_WARN_LO,			
     OP_A2_THRESH_RSSI_ALARM_HI,			
     OP_A2_THRESH_RSSI_ALARM_LO,			
     OP_A2_THRESH_RSSI_WARN_HI,			
     OP_A2_THRESH_RSSI_WARN_LO,			
     OP_A2_THRESH_BIAS_ALARM_HI,			
     OP_A2_THRESH_BIAS_ALARM_LO,			
     OP_A2_THRESH_BIAS_WARN_HI,			
     OP_A2_THRESH_BIAS_WARN_LO,			
     OP_A2_THRESH_TX_PWR_ALARM_HI,			
     OP_A2_THRESH_TX_PWR_ALARM_LO,			
     OP_A2_THRESH_TX_PWR_WARN_HI,			
     OP_A2_THRESH_TX_PWR_WARN_LO,			

     OP_A2_ISR_TEMP_ALARM_HI,			
     OP_A2_ISR_TEMP_ALARM_LO,			
     OP_A2_ISR_VCC_ALARM_HI,			
     OP_A2_ISR_VCC_ALARM_LO,			
     OP_A2_ISR_VPDMON_ALARM_HI,			
     OP_A2_ISR_VPDMON_ALARM_LO,			
     OP_A2_ISR_RFMON_ALARM_HI,			
     OP_A2_ISR_RFMON_ALARM_LO,			
     OP_A2_ISR_RSSI_ALARM_HI,			
     OP_A2_ISR_RSSI_ALARM_LO,			

     OP_A2_ISR_TX_PWR_ALARM_HI,			
     OP_A2_ISR_TX_PWR_ALARM_LO,			
     OP_A2_ISR_BIAS_ALARM_HI,			
     OP_A2_ISR_BIAS_ALARM_LO,			


     OP_A2_ISR_SD_OFF,			
     OP_A2_ISR_SD_ON,			
     OP_A2_ISR_TXFAIL_OFF,			
     OP_A2_ISR_TXFAIL_ON,			
     OP_A2_ISR_TXDIS_OFF,			
     OP_A2_ISR_TXDIS_ON,			
     OP_A2_ISR_TEMP_WARN_HI,			
     OP_A2_ISR_TEMP_WARN_LO,			
     OP_A2_ISR_VCC_WARN_HI,			
     OP_A2_ISR_VCC_WARN_LO,			
     OP_A2_ISR_VPDMON_WARN_HI,			
     OP_A2_ISR_VPDMON_WARN_LO,			
     OP_A2_ISR_RFMON_WARN_HI,			
     OP_A2_ISR_RFMON_WARN_LO,			
     OP_A2_ISR_RSSI_WARN_HI,			
     OP_A2_ISR_RSSI_WARN_LO,			

     OP_A2_ISR_TX_PWR_WARN_HI,			
     OP_A2_ISR_TX_PWR_WARN_LO,			
     OP_A2_ISR_BIAS_WARN_HI,			
     OP_A2_ISR_BIAS_WARN_LO,			

     OP_A2_ISR_IBIAS_HI,			
     OP_A2_ISR_IBIAS_LO,			
     OP_A2_ISR_EYESAFE_ON,

     OP_A2_LLWD_State,			
     OP_A2_HLWD_State,			
     OP_A2_Reset_State,	
     OP_A2_IMASK0,			
     OP_A2_IMASK1,			
     OP_A2_IMASK2,			
     OP_A2_IMASK3,			
                     			
     //operational data			
     OP_A2_RF_OFFSET,			
     OP_A2_RSSI_C4,			
     OP_A2_RSSI_C3,			
     OP_A2_RSSI_C2,			
     OP_A2_RSSI_C1,			
     OP_A2_RSSI_C0,			
     OP_A2_RFMON_FB_DIFF,			
     OP_A2_FIRMWARE_VER,			
     OP_A2_TEMPERATURE,			
     OP_A2_VCC,			
     OP_A2_VPDMON,			
     OP_A2_RFMON,			
     OP_A2_RSSI,			
     OP_A2_BIASDEGRFACT,			
     OP_A2_BIASDACMON,			
     OP_A2_SD,			
     OP_A2_INTERRUPT,			
     OP_A2_TX_FAIL,			
     OP_A2_V_EN,			
     OP_A2_TX_DIS,			
     OP_A2_HOLD_AGC,			
     OP_A2_EYESAFE_FAULT,			
     OP_A2_RF_SQUELCH_EN,			
     OP_A2_LLWD_RESETN,			
     OP_A2_HLWD_RESETN,			
     OP_A2_EYESAFE_DIS,			
     OP_A2_RX_OUT_EN,			
     OP_A2_RX_SQUELCH_DIS,			
     OP_A2_RESET,			
     OP_A2_SAVE,			
			
     OP_A2_EXT_CALIB_TX_ISLOP,			
     OP_A2_EXT_CALIB_TX_IOFFS,			
     OP_A2_EXT_CALIB_TX_PWR_SLOP,			
     OP_A2_EXT_CALIB_TX_PWR_OFFS,			
     OP_A2_EXT_CALIB_T_SLOP,			
     OP_A2_EXT_CALIB_T_OFFS,			
     OP_A2_EXT_CALIB_V_SLOP,			
     OP_A2_EXT_CALIB_V_OFFS,			
     OP_A2_ANALOG_TX_OUTPWR,			
     OP_A2_OPT_DATA_READY_BAR_STA,			
     OP_A2_OPT_RS0_SEL,			
     OP_A2_OPT_RS0_STA,			
     OP_A2_OPT_RS1_STA,			
     OP_A2_OPT_TX_DIS_STA,			
     OP_A2_EXT_PWR_LVL_SEL,			
     OP_A2_EXT_PWR_LVL_STA,			
     OP_A2_EXT_RS1_SEL,			
     OP_A2_CC_DMI,			
     OP_A2_PASSWORD,
     OP_A2_TABLE_SELECT,
     OP_A2_TABLE2_CALIBR_MODE,
     OP_A2_SOFT_RESET,
     OP_DATAFIELD_NUM			
}OpDataField;

typedef enum {
    /* Possible transceiver types */
    GPON_TRANSCEIVER_TYPE_TRIPLEXER_SP_1,	   /* SOURCEPHOTONICS Triplexer ITR-D3T-TH6-4*/
    GPON_TRANSCEIVER_TYPE_DIPLEXER_NP_1,	   /* NEOPHOTONICS 3J85-5638E-ST1+ */
    GPON_TRANSCEIVER_TYPE_DIPLEXER_SP_1,	   /* SOURCEPHOTONICS Diplexer SFA3424THPCDFJ */
    GPON_TRANSCEIVER_TYPE_BOSA,	                   /* MindSpeed M2100 */
    GPON_TRANSCEIVER_TYPE_DIPLEXER_SX_1,	   /* SUPERXON Diplexer SOGQ3412-FAGA CLASS C+*/
    GPON_TRANSCEIVER_TYPE_MAX
}GponTrsvType;

#if 0
typedef enum {
    GPON_TRANSCEIVER_MODE_DIPLEXER,
    GPON_TRANSCEIVER_MODE_TRIPLEXER,
    GPON_TRANSCEIVER_MODE_BOSA,
    GPON_TRANSCEIVER_MODE_TOSA,
    GPON_TRANSCEIVER_MODE_MAX
}GponTrsvMode;
#endif
    
typedef enum
{
    OP_DATA_FEILD_RO, //Read only
    OP_DATA_FEILD_RT, //Real time parameter
    OP_DATA_FEILD_NV, //Non-Volatile parameter. Active state changes immediately after writing to the location.
    OP_DATA_FEILD_V,  //Volatile parameter. Clears on boot.
    OP_DATA_FEILD_F,   //Flash Control. Action taken upon writing, value clears immediately.
    OP_DATA_FEILD_NUM    
}OpDataFieldPermission;

typedef struct
{
    unsigned int bank;       // bank number, reference to enum OpEepromBank
    unsigned int permission; // data field operate permission, reference to enum OpDataFieldPermission
    unsigned int start;      // start address in bank
    unsigned int size;       // 0x80-0x87 stands for bit0-7
}OP_DATA_FIELD_NODE;

#define OP_SIZE_BIT0 0x80
#define OP_SIZE_BIT1 0x81
#define OP_SIZE_BIT2 0x82 
#define OP_SIZE_BIT3 0x83
#define OP_SIZE_BIT4 0x84 
#define OP_SIZE_BIT5 0x85
#define OP_SIZE_BIT6 0x86
#define OP_SIZE_BIT7 0x87
#define IS_BITFIELD_SIZE(x) ((x) >= OP_SIZE_BIT0 && (x) <= OP_SIZE_BIT7) 


#define TRI_EEPROM_A0 "/proc/i2c_gpon/gponPhy_eeprom0"
#define TRI_EEPROM_A2 "/proc/i2c_gpon/gponPhy_eeprom1"

#ifdef USE_STATIC_OP_DATA
// Sequence align to enum OpDataField
static const OP_DATA_FIELD_NODE op_data_field[GPON_TRANSCEIVER_MAX_TYPE][OP_DATAFIELD_NUM] = {
    /* 0 : SP Triplexer */
    {
	/* Data field in bank A0 */
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0,  12},	     //  OP_A0_CPN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x14, 16},            //  OP_A0_VENDOR_NAME,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x38, 4 },            //  OP_A0_VENDOR_REV,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x44, 16},            //  OP_A0_VENDOR_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x54, 8 },            //  OP_A0_DATE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, 1 },            //  OP_A0_MONITOR_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x60, 30},            //  OP_A0_VENDOR_PN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0c, 1 },            //  OP_A0_BITRATE_UP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0d, 1 },            //  OP_A0_BITRATE_DOWN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3c, 1 },            //  OP_A0_WAVELEN_TX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3d, 1 },            //  OP_A0_WAVELEN_RX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x10, 1 },            //  OP_A0_MINIMUM_RF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x11, 1 },            //  OP_A0_VIDEO_INPUT_PWR_LOW,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x12, 1 },            //  OP_A0_VIDEO_INPUT_PWR_HIGH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x13, 1 },	     //  OP_A0_RF_TILT,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x25, 1 },            //  OP_A0_RF_BANDWIDTH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x32, 1 },            //  OP_A0_RESPONSIVITY,
	{-1,         -1,               -1,  -1 },            //  OP_A0_TRANSC_ID,
	{-1,         -1,               -1,  -1 },            //  OP_A0_TRANSC_EXT_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x80, 1 },            //  OP_A0_TRANSC_TYPE,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VENDOR_OUI,
	{-1,         -1,               -1,  -1 },            //  OP_A0_CONNECTOR,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1X_COPPER_PASSIVE,
	{-1,         -1,               -1,  -1 },            //  OP_A0_1X_COPPER_ACTIVE,
	{-1,         -1,               -1,  -1 },            //  OP_A0_1X_LX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1X_SX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_10G_BASE_SR,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_10G_BASE_LR,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_10G_BASE_LRM,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_10G_BASE_ER,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC48_SHORT_REACH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC48_INTER_REACH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC48_LONG_REACH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_SONET_REACH_SPEC_BIT2,
	{-1,         -1,               -1,  -1 },            //  OP_A0_SONET_REACH_SPEC_BIT1,
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC192_SHORT_REACH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_ESCON_SMF,
	{-1,         -1,               -1,  -1 },            //  OP_A0_ESCON_MMF,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC3_SHORT_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC3_INTER_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC3_LONG_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC12_SHORT_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC12_INTER_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_OC12_LONG_REACH,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1000BASE_SX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1000BASE_LX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1000BASE_CX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_1000BASE_T,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_100BASE_LX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_100BASE_FX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_BASE_BX10,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_BASE_PX,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_ELECT_INTER_ENCL,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LONGWAVE_LASER_LC,
	{-1,         -1,               -1,  -1 },            //  OP_A0_SHORTWAVE_LASER,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_MEDIUM_DIST,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LONG_DIST,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_INTER_DIST,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_SHORT_DIST,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_VERYLONG_DIST,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_PASSIVE_CABLE,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_ACTIVE_CABLE,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LONGWAVE_LASER_LL,
	{-1,         -1,               -1,  -1 },            //  OP_A0_SHORTWAVE_LASER_SL,
	{-1,         -1,               -1,  -1 },            //  OP_A0_SHORTWAVE_LASER_SN,
	{-1,         -1,               -1,  -1 },            //  OP_A0_ELECT_INTRA_ENCL,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_SINGLE_MODE,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_MULTI_MODE_50UM,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_MULTI_MODE_625UM,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_COAX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_MINI_COAX,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TWIST_PAIR,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TWIN_AXIA_PAIR,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_100MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_200MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_400MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_1600MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_800MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_1200MBPS,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_ENC_CODE,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_RATE_ID,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_SFM_KM,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_SMF,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_50UM,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_625UM,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_CABLE,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_LEN_OM3,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_BR_MAX,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_BR_MIN,		
	{-1,         -1,               -1,  -1 },            //  OP_A0_LINR_REC_OUT_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_PWR_LVL_DECL_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_COOLED_TRANS_DECL_IMP,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RX_LOS_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_SIG_DETC_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TX_FAULT_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TX_DIS_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_RATE_SEL_IMP,	

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT2 },                //  OP_A0_ADDR_CHG_REQ,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT3 },                //  OP_A0_PWR_MEAS_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT4 },                //  OP_A0_EXT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT5 },                //  OP_A0_INT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT6 },                //  OP_A0_DGT_DIAG_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT7 },                //  OP_A0_LEGACY_DIAG_IMP,

	{-1,         -1,               -1,  -1 },            //  OP_A0_RATE_SEL_IMP_8431,
	{-1,         -1,               -1,  -1 },            //  OP_A0_APP_SEL_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_RATE_SEL_MON_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_RX_LOS_MON_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TX_FAULT_MON_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_TX_DIS_MON_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_ALMWRN_ALL_IMP,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_SFF8472_COMPLI,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_CC_BASE,	
	{-1,         -1,               -1,  -1 },            //  OP_A0_CC_EXT,

        /* Data field in bank A2 */                                         
	//alarms & warnings
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x00, 2 },               //  OP_A2_THRESH_TEMP_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x02, 2 },               //  OP_A2_THRESH_TEMP_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x04, 2 },               //  OP_A2_THRESH_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x06, 2 },               //  OP_A2_THRESH_TEMP_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x08, 2 },               //  OP_A2_THRESH_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0a, 2 },               //  OP_A2_THRESH_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0c, 2 },               //  OP_A2_THRESH_VCC_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0e, 2 },               //  OP_A2_THRESH_VCC_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x10, 2 },               //  OP_A2_THRESH_VPDMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x12, 2 },               //  OP_A2_THRESH_VPDMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x14, 2 },               //  OP_A2_THRESH_VPDMON_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x16, 2 },               //  OP_A2_THRESH_VPDMON_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x18, 2 },               //  OP_A2_THRESH_RFMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1a, 2 },               //  OP_A2_THRESH_RFMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1c, 2 },               //  OP_A2_THRESH_RFMON_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1e, 2 },               //  OP_A2_THRESH_RFMON_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x20, 2 },               //  OP_A2_THRESH_RSSI_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x22, 2 },               //  OP_A2_THRESH_RSSI_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x24, 2 },               //  OP_A2_THRESH_RSSI_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x26, 2 },               //  OP_A2_THRESH_RSSI_WARN_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_BIAS_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_BIAS_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_BIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_BIAS_WARN_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_TX_PWR_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_TX_PWR_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_TX_PWR_WARN_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_THRESH_TX_PWR_WARN_LO,

    
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT0 },      //  OP_A2_ISR_TEMP_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT1 },      //  OP_A2_ISR_TEMP_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT2 },      //  OP_A2_ISR_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT3 },      //  OP_A2_ISR_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },      //  OP_A2_ISR_VPDMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },      //  OP_A2_ISR_VPDMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT6 },      //  OP_A2_ISR_RFMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT7 },      //  OP_A2_ISR_RFMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT0 },	    //  OP_A2_ISR_RSSI_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT1 },	    //  OP_A2_ISR_RSSI_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_LO,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT2 },	    //  OP_A2_ISR_SD_OFF,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT3 },	    //  OP_A2_ISR_SD_ON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT4 },	    //  OP_A2_ISR_TXFAIL_OFF,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT5 },	    //  OP_A2_ISR_TXFAIL_ON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT6 },	    //  OP_A2_ISR_TXDIS_OFF,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x71, OP_SIZE_BIT7 },	    //  OP_A2_ISR_TXDIS_ON,
	

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT0 },	    //  OP_A2_ISR_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT1 },	    //  OP_A2_ISR_TEMP_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT2 },	    //  OP_A2_ISR_VCC_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT3 },	    //  OP_A2_ISR_VCC_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT4 },	    //  OP_A2_ISR_VPDMON_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT5 },	    //  OP_A2_ISR_VPDMON_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT6 },	    //  OP_A2_ISR_RFMON_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT7 },	    //  OP_A2_ISR_RFMON_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT0 },	    //  OP_A2_ISR_RSSI_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT1 },	    //  OP_A2_ISR_RSSI_WARN_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_WARN_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_WARN_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_WARN_HI,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_WARN_LO,


	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT2 },	    //  OP_A2_ISR_IBIAS_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT3 },	    //  OP_A2_ISR_IBIAS_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT4 },	    //  OP_A2_ISR_EYESAFE_ON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT5 },	    //  OP_A2_LLWD_State,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT6 },	    //  OP_A2_HLWD_State,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x75, OP_SIZE_BIT7 },	    //  OP_A2_Reset_State,		
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x72, 1 },                 //  OP_A2_IMASK0,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x73, 1 },                 //  OP_A2_IMASK1,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, 1 },                 //  OP_A2_IMASK2,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x77, 1 },                 //  OP_A2_IMASK3,

	//operational data
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x32, 1 },                   //  OP_A2_RF_OFFSET,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x38, 4 },                   //  OP_A2_RSSI_C4,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x3c, 4 },                   //  OP_A2_RSSI_C3,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x40, 4 },                   //  OP_A2_RSSI_C2,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x44, 4 },                   //  OP_A2_RSSI_C1,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x48, 4 },                   //  OP_A2_RSSI_C0,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x59, 2 },                   //  OP_A2_RFMON_FB_DIFF,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x5d, 2 },                   //  OP_A2_FIRMWARE_VER,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x60, 2 },                   //  OP_A2_TEMPERATURE,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x62, 2 },                   //  OP_A2_VCC,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x64, 2 },                   //  OP_A2_VPDMON,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x66, 2 },                   //  OP_A2_RFMON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x68, 2 },                   //  OP_A2_RSSI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6a, 1 },                   //  OP_A2_BIASDEGRFACT,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6c, 1 },                   //  OP_A2_BIASDACMON,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT0 },        //  OP_A2_SD,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT1 },        //  OP_A2_INTERRUPT,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT2 },        //  OP_A2_TX_FAIL,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT3 },        //  OP_A2_V_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT4 },        //  OP_A2_TX_DIS,
	{OP_BANK_A2, OP_DATA_FEILD_V , 0x6e, OP_SIZE_BIT5 },        //  OP_A2_HOLD_AGC,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT6 },        //  OP_A2_EYESAFE_FAULT,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT7 },        //  OP_A2_RF_SQUELCH_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6f, OP_SIZE_BIT0 },        //  OP_A2_LLWD_RESETN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6f, OP_SIZE_BIT1 },        //  OP_A2_HLWD_RESETN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6f, OP_SIZE_BIT3 },        //  OP_A2_EYESAFE_DIS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6f, OP_SIZE_BIT5 },        //  OP_A2_RX_OUT_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6f, OP_SIZE_BIT6 },        //  OP_A2_RX_SQUELCH_DIS,
	{OP_BANK_A2, OP_DATA_FEILD_F , 0x78, OP_SIZE_BIT0 },        //  OP_A2_RESET,
	{OP_BANK_A2, OP_DATA_FEILD_F , 0x78, OP_SIZE_BIT1 },        //  OP_A2_SAVE,

	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_TX_ISLOP,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_TX_IOFFS,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_TX_PWR_SLOP,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_TX_PWR_OFFS,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_T_SLOP,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_T_OFFS,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_V_SLOP,
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_CALIB_V_OFFS,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ANALOG_TX_OUTPWR,
	
	{-1,         -1,               -1,  -1 },            //  OP_A2_OPT_DATA_READY_BAR_STA,
	{-1,         -1,               -1,  -1 },            //  OP_A2_OPT_RS0_SEL,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_OPT_RS0_STA,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_OPT_RS1_STA,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_OPT_TX_DIS_STA,	

	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_PWR_LVL_SEL,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_PWR_LVL_STA,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_EXT_RS1_SEL,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_CC_DMI,		
	{-1,         -1,               -1,  -1 },            //  OP_A2_PASSWORD,
	{-1,         -1,               -1,  -1 },            //  OP_A2_TABLE_SELECT,     
	{-1,         -1,               -1,  -1 },            //  OP_A2_TABLE2_CALIBR_MODE,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_SOFT_RESET,
    },


    /* 1 : NP Diplexer */
    {

    /* Data field in bank A0 */
    /*Vendor Information*/
	{-1,         -1,               -1,  -1 },           //  OP_A0_CPN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x14, 16},           //  OP_A0_VENDOR_NAME,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x38, 4 },           //  OP_A0_VENDOR_REV,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x44, 16},           //  OP_A0_VENDOR_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x54, 8 },           //  OP_A0_DATE_CODE,
	{-1,         -1,               -1,  -1 },           //  OP_A0_MONITOR_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x28, 16},           //  OP_A0_VENDOR_PN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0c, 1 },           //  OP_A0_BITRATE_UP,
	{-1,         -1,               -1,  -1 },           //  OP_A0_BITRATE_DOWN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3c, 2 },           //  OP_A0_WAVELEN_TX
	{-1,         -1,               -1,  -1 },           //  OP_A0_WAVELEN_RX

	{-1,         -1,               -1,  -1 },            //  OP_A0_MINIMUM_RF,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_LOW,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_HIGH,
	{-1,         -1,               -1,  -1 },	     //  OP_A0_RF_TILT,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RF_BANDWIDTH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RESPONSIVITY,




	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0, 1 },             //  OP_A0_TRANSC_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x1, 1 },             //  OP_A0_TRANSC_EXT_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x80, 1 },            //  OP_A0_TRANSC_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x25, 3 },            //  OP_A0_VENDOR_OUI,


	/*Connector Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x2, 1 },                            //  OP_A0_CONNECTOR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT0 },                 //  OP_A0_1X_COPPER_PASSIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT1 },                 //  OP_A0_1X_COPPER_ACTIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT2 },                 //  OP_A0_1X_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT3 },                 //  OP_A0_1X_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT4 },                 //  OP_A0_10G_BASE_SR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT5 },                 //  OP_A0_10G_BASE_LR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT6 },                 //  OP_A0_10G_BASE_LRM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT7 },                 //  OP_A0_10G_BASE_ER,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT0 },                 //  OP_A0_OC48_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT1 },                 //  OP_A0_OC48_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT2 },                 //  OP_A0_OC48_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT3 },                 //  OP_A0_SONET_REACH_SPEC_BIT2,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT4 },                 //  OP_A0_SONET_REACH_SPEC_BIT1,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT5 },                 //  OP_A0_OC192_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT6 },                 //  OP_A0_ESCON_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT7 },                 //  OP_A0_ESCON_MMF,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT0 },                 //  OP_A0_OC3_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT1 },                 //  OP_A0_OC3_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT2 },                 //  OP_A0_OC3_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT4 },                 //  OP_A0_OC12_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT5 },                 //  OP_A0_OC12_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT6 },                 //  OP_A0_OC12_LONG_REACH,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT0 },                 //  OP_A0_1000BASE_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT1 },                 //  OP_A0_1000BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT2 },                 //  OP_A0_1000BASE_CX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT3 },                 //  OP_A0_1000BASE_T,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT4 },                 //  OP_A0_100BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT5 },                 //  OP_A0_100BASE_FX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT6 },                 //  OP_A0_BASE_BX10,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT7 },                 //  OP_A0_BASE_PX,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT0 },                 //  OP_A0_ELECT_INTER_ENCL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT1 },                 //  OP_A0_LONGWAVE_LASER_LC,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT2 },                 //  OP_A0_SHORTWAVE_LASER,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT3 },                 //  OP_A0_MEDIUM_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT4 },                 //  OP_A0_LONG_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT5 },                 //  OP_A0_INTER_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT6 },                 //  OP_A0_SHORT_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT7 },                 //  OP_A0_VERYLONG_DIST,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT2 },                 //  OP_A0_PASSIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT3 },                 //  OP_A0_ACTIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT4 },                 //  OP_A0_LONGWAVE_LASER_LL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT5 },                 //  OP_A0_SHORTWAVE_LASER_SL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT6 },                 //  OP_A0_SHORTWAVE_LASER_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT7 },                 //  OP_A0_ELECT_INTRA_ENCL,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT0 },                 //  OP_A0_SINGLE_MODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT2 },                 //  OP_A0_MULTI_MODE_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT3 },                 //  OP_A0_MULTI_MODE_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT4 },                 //  OP_A0_VIDEO_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT5 },                 //  OP_A0_MINI_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT6 },                 //  OP_A0_TWIST_PAIR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT7 },                 //  OP_A0_TWIN_AXIA_PAIR,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT0 },                 //  OP_A0_100MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT2 },                 //  OP_A0_200MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT4 },                 //  OP_A0_400MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT5 },                 //  OP_A0_1600MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT6 },                 //  OP_A0_800MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT7 },                 //  OP_A0_1200MBPS,

	/*Optical Signal Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xb, 1 },                            //  OP_A0_ENC_CODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xd, 1 },                            //  OP_A0_RATE_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xe, 1 },                            //  OP_A0_LEN_SFM_KM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xf, 1 },                            //  OP_A0_LEN_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x10, 1 },                           //  OP_A0_LEN_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x11, 1 },                           //  OP_A0_LEN_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x12, 1 },                           //  OP_A0_LEN_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x13, 1 },                           //  OP_A0_LEN_OM3,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x42, 1 },                           //  OP_A0_BR_MAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x43, 1 },                           //  OP_A0_BR_MIN,

	/*Transceiver Properties*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT0 },                //  OP_A0_LINR_REC_OUT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT1 },                //  OP_A0_PWR_LVL_DECL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT2 },                //  OP_A0_COOLED_TRANS_DECL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT1 },                //  OP_A0_RX_LOS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT2 },                //  OP_A0_SIG_DETC_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT3 },                //  OP_A0_TX_FAULT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT4 },                //  OP_A0_TX_DIS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT5 },                //  OP_A0_RATE_SEL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT2 },                //  OP_A0_ADDR_CHG_REQ,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT3 },                //  OP_A0_PWR_MEAS_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT4 },                //  OP_A0_EXT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT5 },                //  OP_A0_INT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT6 },                //  OP_A0_DGT_DIAG_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT7 },                //  OP_A0_LEGACY_DIAG_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT1 },                //  OP_A0_RATE_SEL_IMP_8431,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT2 },                //  OP_A0_APP_SEL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT3 },                //  OP_A0_RATE_SEL_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT4 },                //  OP_A0_RX_LOS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT5 },                //  OP_A0_TX_FAULT_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT6 },                //  OP_A0_TX_DIS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT7 },                //  OP_A0_ALMWRN_ALL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5e, 1 },                           //  OP_A0_SFF8472_COMPLI,

	/*Check Sum*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3f, 1 },		            //  OP_A0_CC_BASE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5f, 1 },                           //  OP_A0_CC_EXT,

/* Data field in bank A2 */

	/*Temperture Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x00, 2 },               //  OP_A2_THRESH_TEMP_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x02, 2 },               //  OP_A2_THRESH_TEMP_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x04, 2 },               //  OP_A2_THRESH_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x06, 2 },               //  OP_A2_THRESH_TEMP_WARN_LO,

	/*Voltage Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x08, 2 },               //  OP_A2_THRESH_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0a, 2 },               //  OP_A2_THRESH_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0c, 2 },               //  OP_A2_THRESH_VCC_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0e, 2 },               //  OP_A2_THRESH_VCC_WARN_LO,

	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_LO,


	/*Laser Rx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x20, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x22, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x24, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x26, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_LO,




	/*Laser Bias Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x10, 2 },               //  OP_A2_THRESH_BIAS_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x12, 2 },               //  OP_A2_THRESH_BIAS_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x14, 2 },               //  OP_A2_THRESH_BIAS_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x16, 2 },               //  OP_A2_THRESH_BIAS_WARN_LO,

	/*Laser Tx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x18, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1a, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1c, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1e, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_LO,




	/*Alarm and Warning ISR*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT7 },	//  OP_A2_ISR_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT6 },	//  OP_A2_ISR_TEMP_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },    //  OP_A2_ISR_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },    //  OP_A2_ISR_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },      //  OP_A2_ISR_VPDMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },      //  OP_A2_ISR_VPDMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT6 },      //  OP_A2_ISR_RFMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT7 },      //  OP_A2_ISR_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_LO,


	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_ON,
	

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TX_PWR_WARN_HI,
	{-1,         -1,               -1,  -1 },           //  OP_A2_ISR_TX_PWR_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_LO,

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_EYESAFE_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_LLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_HLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_Reset_State,		
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK0,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK1,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK2,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK3,





	/*Operational Data*/

	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_OFFSET,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x38, 4 },                   //  OP_A2_RSSI_C4,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x3c, 4 },                   //  OP_A2_RSSI_C3,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x40, 4 },                   //  OP_A2_RSSI_C2,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x44, 4 },                   //  OP_A2_RSSI_C1,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x48, 4 },                   //  OP_A2_RSSI_C0,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON_FB_DIFF,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_FIRMWARE_VER,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x60, 2 },                   //  OP_A2_TEMPERATURE,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x62, 2 },                   //  OP_A2_VCC,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_VPDMON,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x68, 2 },                   //  OP_A2_RSSI,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6a, 1 },                   //  OP_A2_BIASDEGRFACT,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x64, 2 },                   //  OP_A2_BIASDACMON,

	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT1 },        //  OP_A2_SD,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_INTERRUPT,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT2 },        //  OP_A2_TX_FAIL,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_V_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT6 },        //  OP_A2_TX_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HOLD_AGC,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_FAULT,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_SQUELCH_EN,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_LLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_OUT_EN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_SQUELCH_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RESET,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_SAVE,



	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4c, 2 },               //  OP_A2_EXT_CALIB_TX_ISLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4e, 2 },               //  OP_A2_EXT_CALIB_TX_IOFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x50, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x52, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x54, 2 },               //  OP_A2_EXT_CALIB_T_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x56, 2 },               //  OP_A2_EXT_CALIB_T_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x58, 2 },               //  OP_A2_EXT_CALIB_V_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x5a, 2 },               //  OP_A2_EXT_CALIB_V_OFFS,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x66, 2 },               //  OP_A2_ANALOG_TX_OUTPWR,

	{-1,         -1              , -1  , -1           },    //  OP_A2_OPT_DATA_READY_BAR_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT3 },    //  OP_A2_OPT_RS0_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT4 },    //  OP_A2_OPT_RS0_STA,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT5 },    //  OP_A2_OPT_RS1_STA,
	{-1        , -1              , -1  , -1           },    //  OP_A2_OPT_TX_DIS_STA,

	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT0 },    //  OP_A2_EXT_PWR_LVL_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x76, OP_SIZE_BIT1 },    //  OP_A2_EXT_PWR_LVL_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT3 },    //  OP_A2_EXT_RS1_SEL,

	/*Check Sum*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x5f, 1 },               //  OP_A2_CC_DMI,
	
	{-1,         -1,               -1,  -1 },            //  OP_A2_PASSWORD,
	{-1,         -1,               -1,  -1 },            //  OP_A2_TABLE_SELECT,     	
	{-1,         -1,               -1,  -1 },            //  OP_A2_TABLE2_CALIBR_MODE,	
	{-1,         -1,               -1,  -1 },            //  OP_A2_SOFT_RESET,

    },

    /* 2 : SOURCEPHOTONICS SFU-34-24T-HP-CDFJ DIPLEXER */
    {

    /* Data field in bank A0 */
    /*Vendor Information*/
	{-1,         -1,               -1,  -1 },           //  OP_A0_CPN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x14, 16},           //  OP_A0_VENDOR_NAME,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x38, 4 },           //  OP_A0_VENDOR_REV,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x44, 16},           //  OP_A0_VENDOR_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x54, 8 },           //  OP_A0_DATE_CODE,
	{-1,         -1,               -1,  -1 },           //  OP_A0_MONITOR_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x28, 16},           //  OP_A0_VENDOR_PN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0c, 1 },           //  OP_A0_BITRATE_UP,
	{-1,         -1,               -1,  -1 },           //  OP_A0_BITRATE_DOWN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3c, 2 },           //  OP_A0_WAVELEN_TX
	{-1,         -1,               -1,  -1 },           //  OP_A0_WAVELEN_RX

	{-1,         -1,               -1,  -1 },            //  OP_A0_MINIMUM_RF,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_LOW,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_HIGH,
	{-1,         -1,               -1,  -1 },	     //  OP_A0_RF_TILT,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RF_BANDWIDTH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RESPONSIVITY,




	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0, 1 },             //  OP_A0_TRANSC_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x1, 1 },             //  OP_A0_TRANSC_EXT_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x80, 1 },            //  OP_A0_TRANSC_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x25, 3 },            //  OP_A0_VENDOR_OUI,


	/*Connector Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x2, 1 },                            //  OP_A0_CONNECTOR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT0 },                 //  OP_A0_1X_COPPER_PASSIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT1 },                 //  OP_A0_1X_COPPER_ACTIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT2 },                 //  OP_A0_1X_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT3 },                 //  OP_A0_1X_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT4 },                 //  OP_A0_10G_BASE_SR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT5 },                 //  OP_A0_10G_BASE_LR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT6 },                 //  OP_A0_10G_BASE_LRM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT7 },                 //  OP_A0_10G_BASE_ER,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT0 },                 //  OP_A0_OC48_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT1 },                 //  OP_A0_OC48_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT2 },                 //  OP_A0_OC48_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT3 },                 //  OP_A0_SONET_REACH_SPEC_BIT2,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT4 },                 //  OP_A0_SONET_REACH_SPEC_BIT1,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT5 },                 //  OP_A0_OC192_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT6 },                 //  OP_A0_ESCON_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT7 },                 //  OP_A0_ESCON_MMF,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT0 },                 //  OP_A0_OC3_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT1 },                 //  OP_A0_OC3_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT2 },                 //  OP_A0_OC3_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT4 },                 //  OP_A0_OC12_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT5 },                 //  OP_A0_OC12_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT6 },                 //  OP_A0_OC12_LONG_REACH,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT0 },                 //  OP_A0_1000BASE_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT1 },                 //  OP_A0_1000BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT2 },                 //  OP_A0_1000BASE_CX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT3 },                 //  OP_A0_1000BASE_T,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT4 },                 //  OP_A0_100BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT5 },                 //  OP_A0_100BASE_FX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT6 },                 //  OP_A0_BASE_BX10,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT7 },                 //  OP_A0_BASE_PX,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT0 },                 //  OP_A0_ELECT_INTER_ENCL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT1 },                 //  OP_A0_LONGWAVE_LASER_LC,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT2 },                 //  OP_A0_SHORTWAVE_LASER,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT3 },                 //  OP_A0_MEDIUM_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT4 },                 //  OP_A0_LONG_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT5 },                 //  OP_A0_INTER_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT6 },                 //  OP_A0_SHORT_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT7 },                 //  OP_A0_VERYLONG_DIST,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT2 },                 //  OP_A0_PASSIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT3 },                 //  OP_A0_ACTIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT4 },                 //  OP_A0_LONGWAVE_LASER_LL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT5 },                 //  OP_A0_SHORTWAVE_LASER_SL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT6 },                 //  OP_A0_SHORTWAVE_LASER_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT7 },                 //  OP_A0_ELECT_INTRA_ENCL,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT0 },                 //  OP_A0_SINGLE_MODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT2 },                 //  OP_A0_MULTI_MODE_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT3 },                 //  OP_A0_MULTI_MODE_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT4 },                 //  OP_A0_VIDEO_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT5 },                 //  OP_A0_MINI_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT6 },                 //  OP_A0_TWIST_PAIR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT7 },                 //  OP_A0_TWIN_AXIA_PAIR,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT0 },                 //  OP_A0_100MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT2 },                 //  OP_A0_200MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT4 },                 //  OP_A0_400MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT5 },                 //  OP_A0_1600MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT6 },                 //  OP_A0_800MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT7 },                 //  OP_A0_1200MBPS,

	/*Optical Signal Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xb, 1 },                            //  OP_A0_ENC_CODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xd, 1 },                            //  OP_A0_RATE_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xe, 1 },                            //  OP_A0_LEN_SFM_KM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xf, 1 },                            //  OP_A0_LEN_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x10, 1 },                           //  OP_A0_LEN_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x11, 1 },                           //  OP_A0_LEN_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x12, 1 },                           //  OP_A0_LEN_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x13, 1 },                           //  OP_A0_LEN_OM3,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x42, 1 },                           //  OP_A0_BR_MAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x43, 1 },                           //  OP_A0_BR_MIN,

	/*Transceiver Properties*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT0 },                //  OP_A0_LINR_REC_OUT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT1 },                //  OP_A0_PWR_LVL_DECL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT2 },                //  OP_A0_COOLED_TRANS_DECL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT1 },                //  OP_A0_RX_LOS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT2 },                //  OP_A0_SIG_DETC_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT3 },                //  OP_A0_TX_FAULT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT4 },                //  OP_A0_TX_DIS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT5 },                //  OP_A0_RATE_SEL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT2 },                //  OP_A0_ADDR_CHG_REQ,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT3 },                //  OP_A0_PWR_MEAS_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT4 },                //  OP_A0_EXT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT5 },                //  OP_A0_INT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT6 },                //  OP_A0_DGT_DIAG_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT7 },                //  OP_A0_LEGACY_DIAG_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT1 },                //  OP_A0_RATE_SEL_IMP_8431,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT2 },                //  OP_A0_APP_SEL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT3 },                //  OP_A0_RATE_SEL_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT4 },                //  OP_A0_RX_LOS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT5 },                //  OP_A0_TX_FAULT_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT6 },                //  OP_A0_TX_DIS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT7 },                //  OP_A0_ALMWRN_ALL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5e, 1 },                           //  OP_A0_SFF8472_COMPLI,

	/*Check Sum*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3f, 1 },		            //  OP_A0_CC_BASE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5f, 1 },                           //  OP_A0_CC_EXT,

        /* Data field in bank A2 */

	/*Temperture Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x00, 2 },               //  OP_A2_THRESH_TEMP_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x02, 2 },               //  OP_A2_THRESH_TEMP_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x04, 2 },               //  OP_A2_THRESH_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x06, 2 },               //  OP_A2_THRESH_TEMP_WARN_LO,

	/*Voltage Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x08, 2 },               //  OP_A2_THRESH_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0a, 2 },               //  OP_A2_THRESH_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0c, 2 },               //  OP_A2_THRESH_VCC_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0e, 2 },               //  OP_A2_THRESH_VCC_WARN_LO,

	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_LO,


	/*Laser Rx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x20, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x22, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x24, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x26, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_LO,




	/*Laser Bias Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x10, 2 },               //  OP_A2_THRESH_BIAS_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x12, 2 },               //  OP_A2_THRESH_BIAS_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x14, 2 },               //  OP_A2_THRESH_BIAS_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x16, 2 },               //  OP_A2_THRESH_BIAS_WARN_LO,

	/*Laser Tx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x18, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1a, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1c, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1e, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_LO,




	/*Alarm and Warning ISR*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT7 },	//  OP_A2_ISR_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT6 },	//  OP_A2_ISR_TEMP_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },    //  OP_A2_ISR_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },    //  OP_A2_ISR_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },      //  OP_A2_ISR_VPDMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },      //  OP_A2_ISR_VPDMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT6 },      //  OP_A2_ISR_RFMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT7 },      //  OP_A2_ISR_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_LO,


	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_ON,
	

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TX_PWR_WARN_HI,
	{-1,         -1,               -1,  -1 },           //  OP_A2_ISR_TX_PWR_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_LO,

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_EYESAFE_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_LLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_HLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_Reset_State,		
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK0,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK1,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK2,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK3,


	/*Operational Data*/

	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_OFFSET,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x38, 4 },                   //  OP_A2_RSSI_C4,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x3c, 4 },                   //  OP_A2_RSSI_C3,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x40, 4 },                   //  OP_A2_RSSI_C2,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x44, 4 },                   //  OP_A2_RSSI_C1,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x48, 4 },                   //  OP_A2_RSSI_C0,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON_FB_DIFF,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_FIRMWARE_VER,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x60, 2 },                   //  OP_A2_TEMPERATURE,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x62, 2 },                   //  OP_A2_VCC,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_VPDMON,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x68, 2 },                   //  OP_A2_RSSI,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6a, 1 },                   //  OP_A2_BIASDEGRFACT,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x64, 2 },                   //  OP_A2_BIASDACMON,

	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT1 },        //  OP_A2_SD,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_INTERRUPT,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT2 },        //  OP_A2_TX_FAIL,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_V_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT6 },        //  OP_A2_TX_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HOLD_AGC,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_FAULT,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_SQUELCH_EN,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_LLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_OUT_EN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_SQUELCH_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RESET,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_SAVE,



	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4c, 2 },               //  OP_A2_EXT_CALIB_TX_ISLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4e, 2 },               //  OP_A2_EXT_CALIB_TX_IOFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x50, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x52, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x54, 2 },               //  OP_A2_EXT_CALIB_T_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x56, 2 },               //  OP_A2_EXT_CALIB_T_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x58, 2 },               //  OP_A2_EXT_CALIB_V_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x5a, 2 },               //  OP_A2_EXT_CALIB_V_OFFS,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x66, 2 },               //  OP_A2_ANALOG_TX_OUTPWR,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT0 },    //  OP_A2_OPT_DATA_READY_BAR_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT3 },    //  OP_A2_OPT_RS0_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT4 },    //  OP_A2_OPT_RS0_STA,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT5 },    //  OP_A2_OPT_RS1_STA,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT7 },    //  OP_A2_OPT_TX_DIS_STA,


	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT0 },    //  OP_A2_EXT_PWR_LVL_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x76, OP_SIZE_BIT1 },    //  OP_A2_EXT_PWR_LVL_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT3 },    //  OP_A2_EXT_RS1_SEL,

	/*Check Sum*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x5f, 1 },               //  OP_A2_CC_DMI,

	{-1,         -1,               -1,  -1 },            //  OP_A2_PASSWORD,
	{-1,         -1,               -1,  -1 },                                  //  OP_A2_TABLE_SELECT,         
	{-1,         -1,               -1,  -1 },                                  //  OP_A2_TABLE2_CALIBR_MODE,		
	{-1,         -1,               -1,  -1 },            //  OP_A2_SOFT_RESET,
    },





    /* BOSA M2100 */
    {

    /* Data field in bank A0 */
    /*Vendor Information*/
	{-1,         -1,               -1,  -1 },           //  OP_A0_CPN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x14, 16},           //  OP_A0_VENDOR_NAME,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x38, 4 },           //  OP_A0_VENDOR_REV,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x44, 16},           //  OP_A0_VENDOR_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x54, 8 },           //  OP_A0_DATE_CODE,
	{-1,         -1,               -1,  -1 },           //  OP_A0_MONITOR_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x28, 16},           //  OP_A0_VENDOR_PN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0c, 1 },           //  OP_A0_BITRATE_UP,
	{-1,         -1,               -1,  -1 },           //  OP_A0_BITRATE_DOWN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3c, 2 },           //  OP_A0_WAVELEN_TX
	{-1,         -1,               -1,  -1 },           //  OP_A0_WAVELEN_RX

	{-1,         -1,               -1,  -1 },            //  OP_A0_MINIMUM_RF,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_LOW,
	{-1,         -1,               -1,  -1 },            //  OP_A0_VIDEO_INPUT_PWR_HIGH,
	{-1,         -1,               -1,  -1 },	     //  OP_A0_RF_TILT,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RF_BANDWIDTH,
	{-1,         -1,               -1,  -1 },            //  OP_A0_RESPONSIVITY,




	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x0, 1 },             //  OP_A0_TRANSC_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x1, 1 },             //  OP_A0_TRANSC_EXT_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x80, 1 },            //  OP_A0_TRANSC_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x25, 3 },            //  OP_A0_VENDOR_OUI,


	/*Connector Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x2, 1 },                            //  OP_A0_CONNECTOR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT0 },                 //  OP_A0_1X_COPPER_PASSIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT1 },                 //  OP_A0_1X_COPPER_ACTIVE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT2 },                 //  OP_A0_1X_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT3 },                 //  OP_A0_1X_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT4 },                 //  OP_A0_10G_BASE_SR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT5 },                 //  OP_A0_10G_BASE_LR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT6 },                 //  OP_A0_10G_BASE_LRM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3, OP_SIZE_BIT7 },                 //  OP_A0_10G_BASE_ER,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT0 },                 //  OP_A0_OC48_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT1 },                 //  OP_A0_OC48_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT2 },                 //  OP_A0_OC48_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT3 },                 //  OP_A0_SONET_REACH_SPEC_BIT2,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT4 },                 //  OP_A0_SONET_REACH_SPEC_BIT1,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT5 },                 //  OP_A0_OC192_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT6 },                 //  OP_A0_ESCON_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x4, OP_SIZE_BIT7 },                 //  OP_A0_ESCON_MMF,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT0 },                 //  OP_A0_OC3_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT1 },                 //  OP_A0_OC3_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT2 },                 //  OP_A0_OC3_LONG_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT4 },                 //  OP_A0_OC12_SHORT_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT5 },                 //  OP_A0_OC12_INTER_REACH,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5, OP_SIZE_BIT6 },                 //  OP_A0_OC12_LONG_REACH,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT0 },                 //  OP_A0_1000BASE_SX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT1 },                 //  OP_A0_1000BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT2 },                 //  OP_A0_1000BASE_CX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT3 },                 //  OP_A0_1000BASE_T,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT4 },                 //  OP_A0_100BASE_LX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT5 },                 //  OP_A0_100BASE_FX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT6 },                 //  OP_A0_BASE_BX10,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x6, OP_SIZE_BIT7 },                 //  OP_A0_BASE_PX,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT0 },                 //  OP_A0_ELECT_INTER_ENCL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT1 },                 //  OP_A0_LONGWAVE_LASER_LC,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT2 },                 //  OP_A0_SHORTWAVE_LASER,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT3 },                 //  OP_A0_MEDIUM_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT4 },                 //  OP_A0_LONG_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT5 },                 //  OP_A0_INTER_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT6 },                 //  OP_A0_SHORT_DIST,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x7, OP_SIZE_BIT7 },                 //  OP_A0_VERYLONG_DIST,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT2 },                 //  OP_A0_PASSIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT3 },                 //  OP_A0_ACTIVE_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT4 },                 //  OP_A0_LONGWAVE_LASER_LL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT5 },                 //  OP_A0_SHORTWAVE_LASER_SL,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT6 },                 //  OP_A0_SHORTWAVE_LASER_SN,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x8, OP_SIZE_BIT7 },                 //  OP_A0_ELECT_INTRA_ENCL,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT0 },                 //  OP_A0_SINGLE_MODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT2 },                 //  OP_A0_MULTI_MODE_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT3 },                 //  OP_A0_MULTI_MODE_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT4 },                 //  OP_A0_VIDEO_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT5 },                 //  OP_A0_MINI_COAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT6 },                 //  OP_A0_TWIST_PAIR,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x9, OP_SIZE_BIT7 },                 //  OP_A0_TWIN_AXIA_PAIR,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT0 },                 //  OP_A0_100MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT2 },                 //  OP_A0_200MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT4 },                 //  OP_A0_400MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT5 },                 //  OP_A0_1600MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT6 },                 //  OP_A0_800MBPS,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xa, OP_SIZE_BIT7 },                 //  OP_A0_1200MBPS,

	/*Optical Signal Information*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xb, 1 },                            //  OP_A0_ENC_CODE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xd, 1 },                            //  OP_A0_RATE_ID,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xe, 1 },                            //  OP_A0_LEN_SFM_KM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0xf, 1 },                            //  OP_A0_LEN_SMF,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x10, 1 },                           //  OP_A0_LEN_50UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x11, 1 },                           //  OP_A0_LEN_625UM,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x12, 1 },                           //  OP_A0_LEN_CABLE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x13, 1 },                           //  OP_A0_LEN_OM3,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x42, 1 },                           //  OP_A0_BR_MAX,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x43, 1 },                           //  OP_A0_BR_MIN,

	/*Transceiver Properties*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT0 },                //  OP_A0_LINR_REC_OUT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT1 },                //  OP_A0_PWR_LVL_DECL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x40, OP_SIZE_BIT2 },                //  OP_A0_COOLED_TRANS_DECL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT1 },                //  OP_A0_RX_LOS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT2 },                //  OP_A0_SIG_DETC_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT3 },                //  OP_A0_TX_FAULT_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT4 },                //  OP_A0_TX_DIS_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x41, OP_SIZE_BIT5 },                //  OP_A0_RATE_SEL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT2 },                //  OP_A0_ADDR_CHG_REQ,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT3 },                //  OP_A0_PWR_MEAS_TYPE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT4 },                //  OP_A0_EXT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT5 },                //  OP_A0_INT_CALIB,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT6 },                //  OP_A0_DGT_DIAG_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5c, OP_SIZE_BIT7 },                //  OP_A0_LEGACY_DIAG_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT1 },                //  OP_A0_RATE_SEL_IMP_8431,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT2 },                //  OP_A0_APP_SEL_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT3 },                //  OP_A0_RATE_SEL_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT4 },                //  OP_A0_RX_LOS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT5 },                //  OP_A0_TX_FAULT_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT6 },                //  OP_A0_TX_DIS_MON_IMP,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5d, OP_SIZE_BIT7 },                //  OP_A0_ALMWRN_ALL_IMP,

	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5e, 1 },                           //  OP_A0_SFF8472_COMPLI,

	/*Check Sum*/
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x3f, 1 },		            //  OP_A0_CC_BASE,
	{OP_BANK_A0, OP_DATA_FEILD_RO, 0x5f, 1 },                           //  OP_A0_CC_EXT,

        /* Data field in bank A2 */

	/*Temperture Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x00, 2 },               //  OP_A2_THRESH_TEMP_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x02, 2 },               //  OP_A2_THRESH_TEMP_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x04, 2 },               //  OP_A2_THRESH_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x06, 2 },               //  OP_A2_THRESH_TEMP_WARN_LO,

	/*Voltage Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x08, 2 },               //  OP_A2_THRESH_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0a, 2 },               //  OP_A2_THRESH_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0c, 2 },               //  OP_A2_THRESH_VCC_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x0e, 2 },               //  OP_A2_THRESH_VCC_WARN_LO,

	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },               //  OP_A2_THRESH_RFMON_WARN_LO,


	/*Laser Rx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x20, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x22, 2 },               //  OP_A2_THRESH_RX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x24, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x26, 2 },               //  OP_A2_THRESH_RX_PWR_WARN_LO,




	/*Laser Bias Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x10, 2 },               //  OP_A2_THRESH_BIAS_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x12, 2 },               //  OP_A2_THRESH_BIAS_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x14, 2 },               //  OP_A2_THRESH_BIAS_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x16, 2 },               //  OP_A2_THRESH_BIAS_WARN_LO,

	/*Laser Tx Power Thresholds*/
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x18, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1a, 2 },               //  OP_A2_THRESH_TX_PWR_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1c, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x1e, 2 },               //  OP_A2_THRESH_TX_PWR_WARN_LO,




	/*Alarm and Warning ISR*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT7 },	//  OP_A2_ISR_TEMP_WARN_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x74, OP_SIZE_BIT6 },	//  OP_A2_ISR_TEMP_WARN_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },    //  OP_A2_ISR_VCC_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },    //  OP_A2_ISR_VCC_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT4 },      //  OP_A2_ISR_VPDMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT5 },      //  OP_A2_ISR_VPDMON_ALARM_LO,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT6 },      //  OP_A2_ISR_RFMON_ALARM_HI,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x70, OP_SIZE_BIT7 },      //  OP_A2_ISR_RFMON_ALARM_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_HI,   //TODO will be changed per "Transceiver EEPROM Simulator - BOSA.xlsx"
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_TX_PWR_ALARM_LO,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_HI,
	{-1,         -1,               -1,  -1 },            //  OP_A2_ISR_BIAS_ALARM_LO,


	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_SD_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXFAIL_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_OFF,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TXDIS_ON,
	

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TEMP_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VCC_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_VPDMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RFMON_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_RSSI_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_TX_PWR_WARN_HI,
	{-1,         -1,               -1,  -1 },           //  OP_A2_ISR_TX_PWR_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_BIAS_WARN_LO,

	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_HI,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_IBIAS_WARN_LO,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_ISR_EYESAFE_ON,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_LLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_HLWD_State,
	{-1,         -1,               -1,  -1 },	    //  OP_A2_Reset_State,		
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK0,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK1,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK2,
	{-1,         -1,               -1,  -1 },                 //  OP_A2_IMASK3,


	/*Operational Data*/

	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_OFFSET,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x38, 4 },                   //  OP_A2_RSSI_C4,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x3c, 4 },                   //  OP_A2_RSSI_C3,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x40, 4 },                   //  OP_A2_RSSI_C2,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x44, 4 },                   //  OP_A2_RSSI_C1,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x48, 4 },                   //  OP_A2_RSSI_C0,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON_FB_DIFF,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_FIRMWARE_VER,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x60, 2 },                   //  OP_A2_TEMPERATURE,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x62, 2 },                   //  OP_A2_VCC,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_VPDMON,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RFMON,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x68, 2 },                   //  OP_A2_RSSI,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6a, 1 },                   //  OP_A2_BIASDEGRFACT,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x64, 2 },                   //  OP_A2_BIASDACMON,

	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT1 },        //  OP_A2_SD,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_INTERRUPT,
	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x6e, OP_SIZE_BIT2 },        //  OP_A2_TX_FAIL,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_V_EN,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT6 },        //  OP_A2_TX_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HOLD_AGC,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_FAULT,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RF_SQUELCH_EN,

	{-1,         -1,               -1,  -1 },                   //  OP_A2_LLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_HLWD_RESETN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_EYESAFE_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_OUT_EN,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RX_SQUELCH_DIS,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_RESET,
	{-1,         -1,               -1,  -1 },                   //  OP_A2_SAVE,



	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4c, 2 },               //  OP_A2_EXT_CALIB_TX_ISLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x4e, 2 },               //  OP_A2_EXT_CALIB_TX_IOFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x50, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x52, 2 },               //  OP_A2_EXT_CALIB_TX_PWR_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x54, 2 },               //  OP_A2_EXT_CALIB_T_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x56, 2 },               //  OP_A2_EXT_CALIB_T_OFFS,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x58, 2 },               //  OP_A2_EXT_CALIB_V_SLOP,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x5a, 2 },               //  OP_A2_EXT_CALIB_V_OFFS,

	{OP_BANK_A2, OP_DATA_FEILD_RT, 0x66, 2 },               //  OP_A2_ANALOG_TX_OUTPWR,

	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT0 },    //  OP_A2_OPT_DATA_READY_BAR_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x6e, OP_SIZE_BIT3 },    //  OP_A2_OPT_RS0_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT4 },    //  OP_A2_OPT_RS0_STA,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT5 },    //  OP_A2_OPT_RS1_STA,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x6e, OP_SIZE_BIT7 },    //  OP_A2_OPT_TX_DIS_STA,


	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT0 },    //  OP_A2_EXT_PWR_LVL_SEL,
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x76, OP_SIZE_BIT1 },    //  OP_A2_EXT_PWR_LVL_STA,
	{OP_BANK_A2, OP_DATA_FEILD_NV, 0x76, OP_SIZE_BIT3 },    //  OP_A2_EXT_RS1_SEL,

	/*Check Sum*/
	{OP_BANK_A2, OP_DATA_FEILD_RO, 0x5f, 1 },               //  OP_A2_CC_DMI,

	{OP_BANK_A2, OP_DATA_FEILD_NV,  0x7b, 4 },              //  OP_A2_PASSWORD,     
	{OP_BANK_A2, OP_DATA_FEILD_NV,  0x7f, 1 },              //  OP_A2_TABLE_SELECT,     
	{OP_BANK_A2, OP_DATA_FEILD_NV,  0x81, OP_SIZE_BIT5 },   //  OP_A2_TABLE2_CALIBR_MODE,     	
	{OP_BANK_A2, OP_DATA_FEILD_NV,  0x97, 1},              //  OP_A2_SOFT_RESET,     
	
    }    
    
    
};

#else
#endif


#define VENDOR_NAME_STR_SP "SOURCEPHOTONICS"
#define VENDOR_NAME_STR_NP "NEOPHOTONICS"
#define VENDOR_NAME_STR_LU "LUMINENTOIC"

#define NP_DIPLEXER_PN_1            "3J85-5638E-ST1+"
#define SP_TRIPLEXER_PN_1           "ITR-D3T-TH6-4"
#define SP_DIPLEXER_PN_1            "SFA3424THPCDFJ"
#define NP_DIPLEXER_PN_TRIMMED      "3J85-5638"


enum { GPON_SHMEM_KEY_ID = 0x4e4f5047 }; /* "GPON" */


#if 0
int optical_get_data(unsigned char index, void *buf);
int optical_set_data(unsigned char index, const void *buf);

int gpon_get_bandwidth_up(int *bd_up);
int gpon_get_bandwidth_down(int *bd_down);
int gpon_get_wavelength_up(int *wl_up);
int gpon_get_wavelength_down(int *wl_down);
int gpon_get_rx_rssi(float *rssi);
int gpon_get_tx_laserbias(float *tx_bias);
int gpon_get_temperature(float *temp);
int gpon_get_vcc(float *vcc);
#endif


#endif /*__OPIF_H__*/

