/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <cstdint>
#include <mutex>
#include <boost/container/flat_map.hpp>
#include "fboss/agent/Transceiver.h"
#include "fboss/agent/if/gen-cpp2/optic_types.h"

namespace facebook { namespace fboss {

// As per SFF-8436, QSFP+ 10 Gbs 4X PLUGGABLE TRANSCEIVER spec

enum QsfpPages {
  LOWER,
  PAGE0,
  PAGE3,
};

enum class SffField;
class TransceiverImpl;

/*
 * This function takes the QsfpIDPromField name and returns
 * the dataAddress, offset and the length as per the QSFP DOM
 * Document mentioned above.
 */
void getQsfpFieldAddress(SffField field, int &dataAddress,
                        int &offset, int &length);

/*
 * This is the QSFP class which will be storing the QSFP EEPROM
 * data from the address 0xA0 which is static data. The class
 * Also contains the presence status of the QSFP module for the
 * port.
 *
 * Note: The public functions need to take the lock before calling
 * the private functions.
 */
class QsfpModule : public Transceiver {
 public:
  explicit QsfpModule(std::unique_ptr<TransceiverImpl> qsfpImpl);
  /*
   * Returns if the QSFP is present or not
   */
  bool isPresent() const override;
  /*
   * Return a valid type.
   */
  TransceiverType type() const override{
    return TransceiverType::QSFP;
  }
  /*
   * Check if the QSFP is present or not
   */
  void detectTransceiver() override;
  /*
   * Returns if the QSFP supports DOM
   */
  bool isDomSupported() const;
  /*
   * Returns the entire QSFP Dom information
   */
  void getSfpDom(SfpDom &dom) override;
  /*
   * Get the QSFP EEPROM Field
   */
  int getFieldValue(SffField fieldName, uint8_t* fieldValue);
  /*
   * Update the QSFP Fields in the cache
   */
  void updateTransceiverInfoFields() override;
  /*
   * Customize QSPF fields as necessary
   */
  void customizeTransceiver() override;
  /*
   * Returns the entire QSFP information
   */
  void getTransceiverInfo(TransceiverInfo &info) override;

  /*
   * The size of the pages used by QSFP.  See below for an explanation of
   * how they are laid out.  This needs to be publicly accessible for
   * testing.
   */
  enum : unsigned int {
    // Size of page read from QSFP via I2C
    MAX_QSFP_PAGE_SIZE = 128,
    // Number of channels per module
    CHANNEL_COUNT = 4,
  };

 private:
  // no copy or assignment
  QsfpModule(QsfpModule const &) = delete;
  QsfpModule& operator=(QsfpModule const &) = delete;

  enum : unsigned int {
    // Maximum cable length reported
    MAX_CABLE_LEN = 255,
  };
  // QSFP+ requires a bottom 128 byte page describing important monitoring
  // information, and then an upper 128 byte page with less frequently
  // referenced information, including vendor identifiers.  There are
  // three other optional pages;  the third provides a bunch of
  // alarm and warning thresholds which we are interested in.
  uint8_t qsfpIdprom_[MAX_QSFP_PAGE_SIZE];
  uint8_t qsfpPage0_[MAX_QSFP_PAGE_SIZE];
  uint8_t qsfpPage3_[MAX_QSFP_PAGE_SIZE];

  // QSFP Presence status
  bool present_{false};
  // Denotes if the cache value is valid or stale
  bool dirty_{false};
  // Flat memory systems don't support paged access to extra data
  bool flatMem_{false};
  /* Qsfp Internal Implementation */
  std::unique_ptr<TransceiverImpl> qsfpImpl_;

  /*
   * qsfpModuleMutex_ is held around all the read and writes to the qsfpModule
   *
   * This is used so that we get consistent from the QsfpModule and to make
   * sure no other thread tries to write at the time some other is reading
   * the information.
   */
  mutable std::mutex qsfpModuleMutex_;

  /*
   * This function returns a pointer to the value in the static cached
   * data after checking the length fits. The thread needs to have the lock
   * before calling this function.
   */
  const uint8_t* getQsfpValuePtr(int dataAddress, int offset,
                                 int length) const;
  /*
   * This function returns the values on the offset and length
   * from the static cached data. The thread needs to have the lock
   * before calling this function.
   */
  void getQsfpValue(int dataAddress,
                    int offset, int length, uint8_t* data) const;
  /*
   * Sets the IDProm cache data for the port
   * The data should be 256 bytes.
   * The thread needs to have the lock before calling the function.
   */
  void setQsfpIdprom();
  /*
   * This is used by the detection thread to set the QSFP presence
   * status based on the HW read.
   * The thread needs to have the lock before calling the function.
   */
  void setPresent(bool present);
  /*
   * returns individual sensor values after scaling
   */
  double getQsfpSensor(SffField field,
    double (*conversion)(uint16_t value));
  /*
   * returns cable length (negative for "longer than we can represent")
   */
  int getQsfpCableLength(SffField field);
  /*
   * Parse out flag values from bitfields
   */
  static FlagLevels getQsfpFlags(const uint8_t* data, int offset);
  /*
   * Extract sensor flag levels
   */
  FlagLevels getQsfpSensorFlags(SffField fieldName);
  /*
   * This function returns various strings from the QSFP EEPROM
   * caller needs to check if DOM is supported or not
   */
  std::string getQsfpString(SffField flag);
  /*
   * Fills in values for alarm and warning thresholds based on field name
   */
  ThresholdLevels getThresholdValues(SffField field,
                            double (*conversion)(uint16_t value));
  /*
   * Retreives all alarm and warning thresholds
   */
  bool getThresholdInfo(AlarmThreshold& threshold);
  /*
   * Gather the sensor info for thrift queries
   */
  bool getSensorInfo(GlobalSensors &sensor);
  /*
   * Gather per-channel information for thrift queries
   */
  bool getSensorsPerChanInfo(std::vector<Channel>& channels);
  /*
   * Gather the vendor info for thrift queries
   */
  bool getVendorInfo(Vendor &vendor);
  /*
   * Gather the cable info for thrift queries
   */
  bool getCableInfo(Cable &cable);
  /*
   * This function returns true if both the sfp is present and the
   * cache data is not stale. This should be checked before any
   * function that reads cache data is called
   */
  bool cacheIsValid() const;
  /*
   * Update the cached data with the information from the physical QSFP.
   */
  void updateQsfpData();
};

}} //namespace facebook::fboss
