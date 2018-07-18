#ifndef INC_BP_TCH_H
#define INC_BP_TCH_H

#define TCH_LEDS \
  bp_usGpioLedPowerRed_tch, \
  bp_usGpioLedPowerGreen_tch, \
  bp_usGpioLedPowerBlue_tch, \
  bp_usGpioLedBroadbandGreen_tch, \
  bp_usGpioLedBroadbandRed_tch, \
  bp_usGpioLedBroadband2Green_tch, \
  bp_usGpioLedBroadband2Red_tch, \
  bp_usGpioLedDectGreen_tch, \
  bp_usGpioLedDectBlue_tch, \
  bp_usGpioLedDectRed_tch, \
  bp_usGpioLedEthernetGreen_tch, \
  bp_usGpioLedEthernetRed_tch, \
  bp_usGpioLedEthernetBlue_tch, \
  bp_usGpioLedIPTVGreen_tch, \
  bp_usGpioLedWirelessGreen_tch, \
  bp_usGpioLedWirelessRed_tch, \
  bp_usGpioLedWirelessBlue_tch, \
  bp_usGpioLedWireless5GHzGreen_tch, \
  bp_usGpioLedWireless5GHzRed_tch, \
  bp_usGpioLedInternetGreen_tch, \
  bp_usGpioLedInternetRed_tch, \
  bp_usGpioLedInternetBlue_tch, \
  bp_usGpioLedVoip1Green_tch, \
  bp_usGpioLedVoip1Red_tch, \
  bp_usGpioLedVoip1Blue_tch, \
  bp_usGpioLedVoip2Green_tch, \
  bp_usGpioLedVoip2Red_tch, \
  bp_usGpioLedWPSGreen_tch, \
  bp_usGpioLedWPSRed_tch, \
  bp_usGpioLedWPSBlue_tch, \
  bp_usGpioLedUsbGreen_tch, \
  bp_usGpioLedUpgradeBlue_tch, \
  bp_usGpioLedUpgradeGreen_tch, \
  bp_usGpioLedLteBlue_tch, \
  bp_usGpioLedLteGreen_tch, \
  bp_usGpioLedLteRed_tch, \
  bp_usGpioLedMoCAGreen_tch, \
  bp_usGpioLedSFPGreen_tch, \
  bp_usGpioLedAmbient1White_tch, \
  bp_usGpioLedAmbient2White_tch, \
  bp_usGpioLedAmbient3White_tch, \
  bp_usGpioLedAmbient4White_tch, \
  bp_usGpioLedAmbient5White_tch


#define TCH_BPELEMS \
  bp_cpVethPortmap_tch, \
  bp_usRxRgmiiClockDelayAtMac_tch, \
  bp_usExtSwLedCfg_tch, \
  bp_usGpioHardRST_tch, \
  bp_ulEthLedMap_tch, \
  bp_usGpioIntAFELDMode_tch, \
  bp_usAELinkLed_tch, \
  TCH_LEDS

#define ETH_LED_MAP_NO_LINK_SHIFT 0
#define ETH_LED_MAP_10M_SHIFT     2
#define ETH_LED_MAP_100M_SHIFT    4
#define ETH_LED_MAP_1000M_SHIFT   6

#define ETH_LED_MAP_SPD0_OFF (1 << 0)
#define ETH_LED_MAP_SPD0_ON  (0 << 0)
#define ETH_LED_MAP_SPD1_OFF (1 << 1)
#define ETH_LED_MAP_SPD1_ON  (0 << 1)

#endif
