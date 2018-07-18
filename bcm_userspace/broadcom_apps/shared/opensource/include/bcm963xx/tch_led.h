#if !defined(_TCH_LED_H)
#define _TCH_LED_H

enum tch_leds {
  kLedPowerRed,
  kLedPowerGreen,
  kLedPowerBlue,
  kLedBroadbandGreen,
  kLedBroadbandRed,
  kLedBroadband2Green,
  kLedBroadband2Red,
  kLedDectGreen,
  kLedDectRed,
  kLedDectBlue,
  kLedEthernetGreen,
  kLedEthernetRed,
  kLedEthernetBlue,
  kLedIPTVGreen,
  kLedWirelessGreen,
  kLedWirelessRed,
  kLedWirelessBlue,
  kLedWireless5GHzGreen,
  kLedWireless5GHzRed,
  kLedInternetGreen,
  kLedInternetRed,
  kLedInternetBlue,
  kLedVoip1Green,
  kLedVoip1Red,
  kLedVoip1Blue,
  kLedVoip2Green,
  kLedVoip2Red,
  kLedWPSGreen,
  kLedWPSBlue,
  kLedWPSRed,
  kLedUsbGreen,
  kLedUpgradeBlue,
  kLedUpgradeGreen,
  kLedLteGreen,
  kLedLteRed,
  kLedLteBlue,
  kLedMoCAGreen,
  kLedSFPGreen,
  kLedAmbient1White,
  kLedAmbient2White,
  kLedAmbient3White,
  kLedAmbient4White,
  kLedAmbient5White,
};


/**
 * API to set leds
 *
 *  led: ID of the TCH led
 *  state: 1 means on, 1 means off
 *
 *  returns: 0 if SUCCESS, -1 otherwise
 */
int tch_led_set(enum tch_leds led, unsigned short state);

/**
 * API to determine whether a certain LED is available on this platform
 *
 *  led: ID of the TCH led
 *
 *  returns: 1 if available, 0 otherwise
 */
int tch_led_is_available(enum tch_leds led);

#endif
