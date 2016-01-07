/* * Copyright (c) 2004-present, Facebook, Inc.  * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */
#pragma once
#include <cstdint>
#include <map>

/*
 * Parse transceiver data fields, as outlined in various documents
 * by the Small Form Factor (SFF) committee, including SFP and QSFP+
 * modules.
 */

namespace facebook { namespace fboss {

enum class SffField {
  // Shared QSFP and SFP fields:
  IDENTIFIER, // Type of Transceiver
  STATUS, // Support flags for upper pages
  TEMPERATURE_ALARMS,
  VCC_ALARMS,  // Voltage
  CHANNEL_RX_PWR_ALARMS,
  CHANNEL_TX_BIAS_ALARMS,
  TEMPERATURE,
  VCC,  // Voltage
  CHANNEL_RX_PWR,
  CHANNEL_TX_BIAS,
  POWER_CONTROL,
  ETHERNET_COMPLIANCE,
  EXTENDED_IDENTIFIER,
  PAGE_SELECT_BYTE,
  LENGTH_SM_KM,  // Single mode, in km
  LENGTH_SM,  // Single mode in 100m (not in QSFP)
  LENGTH_OM3,
  LENGTH_OM2,
  LENGTH_OM1,
  LENGTH_COPPER,
  VENDOR_NAME, // QSFP Vendor Name (ASCII)
  VENDOR_OUI, // QSFP Vendor IEEE company ID
  PART_NUMBER, // Part NUmber provided by QSFP vendor (ASCII)
  REVISION_NUMBER, // Revision number
  VENDOR_SERIAL_NUMBER, // Vendor Serial Number (ASCII)
  MFG_DATE, // Manufacturing date code
  DIAGNOSTIC_MONITORING_TYPE, // Diagnostic monitoring implemented
  TEMPERATURE_THRESH,
  VCC_THRESH,
  RX_PWR_THRESH,
  TX_BIAS_THRESH,

  // SFP-specific Fields
  /* 0xA0 Address Fields */
  EXT_IDENTIFIER, // Extended type of transceiver
  CONNECTOR_TYPE, // Code for Connector Type
  TRANSCEIVER_CODE, // Code for Electronic or optical capability
  ENCODING_CODE, // High speed Serial encoding algo code
  SIGNALLING_RATE, // nominal signalling rate
  RATE_IDENTIFIER, // type of rate select functionality
  TRANCEIVER_CAPABILITY, // Code for Electronic or optical capability
  WAVELENGTH, // laser wavelength
  CHECK_CODE_BASEID, // Check code for the above fields
  // Extended options
  ENABLED_OPTIONS, // Indicates the optional transceiver signals enabled
  UPPER_BIT_RATE_MARGIN, // upper bit rate margin
  LOWER_BIT_RATE_MARGIN, // lower but rate margin
  ENHANCED_OPTIONS, // Enhanced options implemented
  SFF_COMPLIANCE, // revision number of SFF compliance
  CHECK_CODE_EXTENDED_OPT, // check code for the extended options
  VENDOR_EEPROM,

  /* 0xA2 address Fields */
  /* Diagnostics */
  ALARM_THRESHOLD_VALUES, // diagnostic flag alarm and warning thresh values
  EXTERNAL_CALIBRATION, // diagnostic calibration constants
  CHECK_CODE_DMI, // Check code for base Diagnostic Fields
  DIAGNOSTICS, // Diagnostic Monitor Data
  STATUS_CONTROL, // Optional Status and Control bits
  ALARM_WARN_FLAGS, // Diagnostic alarm and warning flag
  EXTENDED_STATUS_CONTROL, // Extended status and control bytes
  /* General Purpose */
  VENDOR_MEM_ADDRESS, // Vendor Specific memory address
  USER_EEPROM, // User Writable NVM
  VENDOR_CONTROL, // Vendor Specific Control
};

enum PowerControl : uint8_t {
  POWER_OVERRIDE =      1 << 0,
  POWER_SET =           1 << 1,
  HIGH_POWER_OVERRIDE = 1 << 2,
};

enum ExternalIdentifer : uint8_t {
  EXT_ID_SHIFT= 6,
  EXT_ID_MASK = 0xc0,
  EXT_ID_HI_POWER_MASK = 0x03,
};

enum EthernetCompliance : uint8_t {
  ACTIVE_CABLE = 1 << 0,
  LR4_40GBASE =  1 << 1,
  SR4_40GBASE =  1 << 2,
  CR4_40GBASE =  1 << 3,
  SR_10GBASE =   1 << 4,
  LR_10GBASE =   1 << 5,
  LRM_40GBASE =  1 << 6,
};

class SffFieldInfo {
 public:
  int dataAddress;
  std::uint32_t offset;
  std::uint32_t length;

  // Conversion routines used for both SFP and QSFP:

  // Render degrees Celcius from fix-point integer value
  static double getTemp(uint16_t temp);

  // Render Vcc in volts from fixed-point value
  static double getVcc(uint16_t temp);

  // Render TxBias in mA from fixed-point value
  static double getTxBias(uint16_t temp);

  // Render power in mW from fixed-point value
  static double getPwr(uint16_t temp);

  typedef std::map<SffField, SffFieldInfo> SffFieldMap;

  /*
   * This function takes the SfpIDPromField name and returns
   * the dataAddress, offset and the length as per the SFP DOM
   * Document mentioned above.
   */
  static SffFieldInfo getSffFieldAddress(const SffFieldMap& map,
                                         SffField field);
};

// Store multipliers for various conversion functions:

typedef std::map<SffField, std::uint32_t> SffFieldMultiplier;

}} //namespace facebook::fboss
