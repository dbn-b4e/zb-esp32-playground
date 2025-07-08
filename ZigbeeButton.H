/* Class of Zigbee On/Off Button endpoint inherited from common EP class */

#pragma once

#include "soc/soc_caps.h"
#include "sdkconfig.h"
#if SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED

#include "ZigbeeEP.h"
#include "ha/esp_zigbee_ha_standard.h"

/**
 * @brief Zigbee HA standard button device clusters.
 *        Added here as not supported by ESP Zigbee library.
 *
 *
 */
typedef struct zigbee_button_cfg_s {
  esp_zb_basic_cluster_cfg_t basic_cfg;       /*!<  Basic cluster configuration, @ref esp_zb_basic_cluster_cfg_s */
  esp_zb_identify_cluster_cfg_t identify_cfg; /*!<  Identify cluster configuration, @ref esp_zb_identify_cluster_cfg_s */
  esp_zb_groups_cluster_cfg_t groups_cfg;     /*!<  Groups cluster configuration, @ref esp_zb_groups_cluster_cfg_s */
  esp_zb_scenes_cluster_cfg_t scenes_cfg;     /*!<  Scenes cluster configuration, @ref esp_zb_scenes_cluster_cfg_s */
  esp_zb_on_off_cluster_cfg_t on_off_cfg;     /*!<  On off cluster configuration, @ref esp_zb_on_off_cluster_cfg_s */
} zigbee_button_cfg_t;

/**
 * @brief Zigbee HA standard button device default config value.
 *        Added here as not supported by ESP Zigbee library.
 *
 */
// clang-format off
#define ZIGBEE_DEFAULT_BUTTON_CONFIG()                                  \
  {                                                                             \
    .basic_cfg =                                                                \
      {                                                                         \
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,              \
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,            \
      },                                                                        \
    .identify_cfg =                                                             \
      {                                                                         \
        .identify_time = ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE,       \
      },                                                                        \                                                                      \
    .on_off_cfg =                                                               \
      {                                                                         \
        .on_off = ESP_ZB_ZCL_ON_OFF_ON_OFF_DEFAULT_VALUE,                       \
      },                                                                        \
  }
// clang-format on

class ZigbeeButton : public ZigbeeEP {
public:
  ZigbeeButton(uint8_t endpoint);
  ~ZigbeeButton() {}

  void onButtonChange(void (*callback)(bool)) {
    _on_button_change = callback;
  }
  void restoreButton() {
    buttonChanged();
  }

  void setButtonState(bool state);
  void toggleButton();

  void setReporting(uint16_t min_interval, uint16_t max_interval, float delta);
  void reportButton();

  bool getButtonState() {
    return _current_state;
  }

private:
  void zbAttributeSet(const esp_zb_zcl_set_attr_value_message_t *message) override;

  void buttonChanged();
  // callback function to be called on button change (State)
  void (*_on_button_change)(bool);

  /**
   * @brief  Create a standard HA button cluster list.
   *        Added here as not supported by ESP Zigbee library.
   *
   * @note This contains basic, identify, groups, scenes, on-off, as server side.
   * @param[in] button_cfg  Configuration parameters for this cluster lists defined by @ref zigbee_button_cfg_t
   *
   * @return Pointer to cluster list  @ref esp_zb_cluster_list_s
   *
   */
  esp_zb_cluster_list_t *zigbee_button_clusters_create(zigbee_button_cfg_t *button_cfg);

  bool _current_state;
};

#endif  // SOC_IEEE802154_SUPPORTED