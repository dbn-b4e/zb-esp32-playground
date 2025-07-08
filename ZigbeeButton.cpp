#include "ZigbeeButton.h"
#if SOC_IEEE802154_SUPPORTED && CONFIG_ZB_ENABLED

#include "esp_zigbee_cluster.h"

ZigbeeButton::ZigbeeButton(uint8_t endpoint)
  : ZigbeeEP(endpoint) {
  _device_id = ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID;

  zigbee_button_cfg_t button_cfg = ESP_ZB_DEFAULT_ON_OFF_SWITCH_CONFIG();
  _cluster_list = zigbee_button_clusters_create(&button_cfg);

  _ep_config = { .endpoint = _endpoint, .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID, .app_device_id = ESP_ZB_HA_ON_OFF_SWITCH_DEVICE_ID, .app_device_version = 0 };

  _current_state = false;
}

// set attribute method -> method overridden in child class
void ZigbeeButton::zbAttributeSet(const esp_zb_zcl_set_attr_value_message_t *message) {
  // check the data and call right method
  log_v("ZigbeeButton::zbAttributeSet");
  if (message->info.cluster == ESP_ZB_ZCL_CLUSTER_ID_ON_OFF) {
    if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL) {
      if (_current_state != *(bool *)message->attribute.data.value) {
        _current_state = *(bool *)message->attribute.data.value;
        buttonChanged();
      }
      return;
    } else {
      log_w("Received message ignored. Attribute ID: %d not supported for On/Off Button", message->attribute.id);
    }
  } else {
    log_w("Received message ignored. Cluster ID: %d not supported for Button", message->info.cluster);
  }
}

void ZigbeeButton::buttonChanged() {
  if (_on_button_change) {
    _on_button_change(_current_state);
  }
}

void ZigbeeButton::setButtonState(bool state) {
  _current_state = state;
  buttonChanged();

  log_v("Updating on/off state to %d", state);
  /* Update clusters */
  esp_zb_lock_acquire(portMAX_DELAY);
  // set on/off state
  esp_zb_zcl_set_attribute_val(
    _endpoint, ESP_ZB_ZCL_CLUSTER_ID_ON_OFF, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID, &_current_state, false);
  esp_zb_lock_release();
}

void ZigbeeButton::toggleButton() {
  _current_state = !_current_state;
  ZigbeeButton::setButtonState(_current_state);
}

void ZigbeeButton::reportButton() {
  /* Send report attributes command */

  esp_zb_zcl_report_attr_cmd_t report_attr_cmd;
  report_attr_cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;  // ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT; // ESP_ZB_APS_ADDR_MODE_DST_ADDR_ENDP_NOT_PRESENT;
  report_attr_cmd.attributeID = ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
  report_attr_cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
  report_attr_cmd.clusterID = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
  report_attr_cmd.zcl_basic_cmd.dst_addr_u.addr_short = 0;
  report_attr_cmd.zcl_basic_cmd.dst_endpoint = 1;
  report_attr_cmd.zcl_basic_cmd.src_endpoint = _endpoint;

  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_report_attr_cmd_req(&report_attr_cmd);
  esp_zb_lock_release();
  log_v("Button state report sent");
}

/* NOT CALLED IN MY .ino*/
void ZigbeeButton::setReporting(uint16_t min_interval, uint16_t max_interval, float delta) {
  esp_zb_zcl_reporting_info_t reporting_info;
  memset(&reporting_info, 0, sizeof(esp_zb_zcl_reporting_info_t));
  reporting_info.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_SRV;
  reporting_info.ep = _endpoint;
  reporting_info.cluster_id = ESP_ZB_ZCL_CLUSTER_ID_ON_OFF;
  reporting_info.cluster_role = ESP_ZB_ZCL_CLUSTER_SERVER_ROLE;
  reporting_info.attr_id = ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID;
  reporting_info.u.send_info.min_interval = min_interval;
  reporting_info.u.send_info.max_interval = max_interval;
  reporting_info.u.send_info.def_min_interval = min_interval;
  reporting_info.u.send_info.def_max_interval = max_interval;
  reporting_info.u.send_info.delta.u16 = (uint16_t)(delta * 100);  // Convert delta to ZCL uint16_t
  reporting_info.dst.profile_id = ESP_ZB_AF_HA_PROFILE_ID;
  reporting_info.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;
  esp_zb_lock_acquire(portMAX_DELAY);
  esp_zb_zcl_update_reporting_info(&reporting_info);
  esp_zb_lock_release();
}

esp_zb_cluster_list_t *ZigbeeButton::zigbee_button_clusters_create(zigbee_button_cfg_t *button_cfg) {
  esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_basic_cluster_create(&button_cfg->basic_cfg);
  esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_identify_cluster_create(&button_cfg->identify_cfg);
  esp_zb_attribute_list_t *esp_zb_on_off_cluster = esp_zb_on_off_cluster_create(&button_cfg->on_off_cfg);

  // ------------------------------ Create cluster list ------------------------------
  esp_zb_cluster_list_t *esp_zb_cluster_list = esp_zb_zcl_cluster_list_create();
  esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
  esp_zb_cluster_list_add_on_off_cluster(esp_zb_cluster_list, esp_zb_on_off_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

  return esp_zb_cluster_list;
}

#endif  // SOC_IEEE802154_SUPPORTED