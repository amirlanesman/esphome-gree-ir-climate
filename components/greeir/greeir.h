#pragma once

#include "esphome/core/component.h"
#include "esphome/components/climate_ir/climate_ir.h"
#include "gree_protocol.h"

namespace esphome
{
  namespace greeir
  {

    // Temperature constants
    const uint8_t GREE_TEMP_MIN = 16; // °C
    const uint8_t GREE_TEMP_MAX = 30; // °C

    // IR timing constants
    const uint32_t GREE_IR_FREQUENCY = 38000;
    const uint32_t GREE_HEADER_MARK = 9000;
    const uint32_t GREE_HEADER_SPACE = 4000;
    const uint32_t GREE_BIT_MARK = 620;
    const uint32_t GREE_ONE_SPACE = 1600;
    const uint32_t GREE_ZERO_SPACE = 540;
    const uint32_t GREE_MESSAGE_SPACE = 19000;

    // YAC1FB9 variant timing (some Gree models)
    const uint32_t GREE_YAC1FB9_HEADER_SPACE = 4500;
    const uint32_t GREE_YAC1FB9_MESSAGE_SPACE = 19800;
    const uint32_t GREE_YAC_HEADER_MARK = 6000;
    const uint32_t GREE_YAC_HEADER_SPACE = 3000;
    const uint32_t GREE_YAC_BIT_MARK = 650;

    // State frame size
    const uint8_t GREE_STATE_FRAME_SIZE = 8;
    const uint8_t GREE_BLOCK_FOOTER_SIZE = 3;

    // Mode constants
    const uint8_t GREE_MODE_AUTO = 0;
    const uint8_t GREE_MODE_COOL = 1;
    const uint8_t GREE_MODE_DRY = 2;
    const uint8_t GREE_MODE_FAN = 3;
    const uint8_t GREE_MODE_HEAT = 4;
    const uint8_t GREE_MODE_ON = 8;
    const uint8_t GREE_MODE_OFF = 0;
    // Power constants
    const uint8_t GREE_POWER_ON = 1;
    const uint8_t GREE_POWER_OFF = 0;

    // Fan speed constants
    const uint8_t GREE_FAN_AUTO = 0;
    const uint8_t GREE_FAN_LOW = 1;
    const uint8_t GREE_FAN_MEDIUM = 2;
    const uint8_t GREE_FAN_HIGH = 3;
    const uint8_t GREE_FAN_TURBO = 0;
    const uint8_t GREE_FAN_TURBO_BIT = 4;

    // Swing constants - Vertical
    const uint8_t GREE_VDIR_AUTO = 0;
    const uint8_t GREE_VDIR_SWING = 1;
    const uint8_t GREE_VDIR_UP = 2;
    const uint8_t GREE_VDIR_MUP = 3;
    const uint8_t GREE_VDIR_MIDDLE = 4;
    const uint8_t GREE_VDIR_MDOWN = 5;
    const uint8_t GREE_VDIR_DOWN = 6;
    const uint8_t GREE_VDIR_MANUAL = 0;

    // Swing constants - Horizontal
    const uint8_t GREE_HDIR_AUTO = 0;
    const uint8_t GREE_HDIR_SWING = 1;
    const uint8_t GREE_HDIR_LEFT = 2;
    const uint8_t GREE_HDIR_MLEFT = 3;
    const uint8_t GREE_HDIR_MIDDLE = 4;
    const uint8_t GREE_HDIR_MRIGHT = 5;
    const uint8_t GREE_HDIR_RIGHT = 6;
    const uint8_t GREE_HDIR_MANUAL = 0;

    // Swing constants - Auto
    const uint8_t GREE_SWING_AUTO = 0;
    const uint8_t GREE_SWING_MANUAL = 1;

    // Preset constants
    const uint8_t GREE_PRESET_NONE = 0;
    const uint8_t GREE_PRESET_SLEEP = 1;
    const uint8_t GREE_PRESET_SLEEP_BIT = 0;

    // Gree model variants
    enum class GreeIRModel
    {
      GENERIC,
      YAW1F,
      YBOFB,
      YAC1FB9,
      YT1F
    };

    class GreeIRClimate : public climate_ir::ClimateIR
    {
    public:
      GreeIRClimate()
          : climate_ir::ClimateIR(GREE_TEMP_MIN, GREE_TEMP_MAX, 1.0f, true, true,
                                  {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW,
                                   climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                                  {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL,
                                   climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH}) {}

      /// Override control to handle all changes in a single call.
      void control(const climate::ClimateCall &call) override;

      /// Set the model for this climate device
      void set_model(GreeIRModel model) { this->model_ = model; }

      /// Get the model
      GreeIRModel get_model() const { return this->model_; }

      /// Enable WiFi function bits (some models)
      void set_wifi_function(bool enable) { this->wifi_function_ = enable; }
      void set_check_checksum(bool enable) { this->check_checksum_ = enable; }
      void set_set_modes(bool enable) { this->set_modes_ = enable; }
      void set_repeat(int8_t repeat) { this->repeat_ = repeat; }

    protected:
      climate::ClimateTraits traits() override;

      /// Transmit via IR the state of this climate controller.
      void transmit_state() override;
      /// Handle received IR Buffer
      bool on_receive(remote_base::RemoteReceiveData data) override;

      /// Calculate checksum for IR data
      uint8_t checksum_();

      void get_state_to_send(uint8_t remote_state[]);

      /// Get operation mode for current state
      uint8_t operation_mode_();

      /// Get fan speed for current state
      uint8_t fan_speed_();

      /// Get vertical swing setting
      uint8_t vertical_swing_();

      /// Get horizontal swing setting
      uint8_t horizontal_swing_();

      /// Get swing setting
      uint8_t swing_auto_();

      /// Parse received IR data into climate state
      bool parse_state_frame_(const uint8_t frame[]);

      GreeIRModel model_{GreeIRModel::GENERIC};
      bool wifi_function_{false};
      bool check_checksum_{false};
      bool set_modes_{false};
      int8_t repeat_{1};
      int32_t last_transmit_time_{};
    };

  } // namespace gree
} // namespace esphome