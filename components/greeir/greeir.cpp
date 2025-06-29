#include "greeir.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace greeir
  {

    static const char *const TAG = "greeir.climate";

    void GreeIRClimate::control(const climate::ClimateCall &call)
    {
      if (call.get_mode().has_value())
        this->mode = *call.get_mode();
      if (call.get_target_temperature().has_value())
        this->target_temperature = *call.get_target_temperature();
      if (call.get_fan_mode().has_value())
        this->fan_mode = *call.get_fan_mode();
      if (call.get_swing_mode().has_value())
        this->swing_mode = *call.get_swing_mode();
      if (call.get_preset().has_value())
        this->preset = *call.get_preset();

      this->transmit_state();
      this->publish_state();
    }

    const uint8_t kKelvinatorChecksumStart = 10;

    /// Calculate the checksum for a given block of state.
    /// @param[in] block A pointer to a block to calc the checksum of.
    /// @param[in] length Length of the block array to checksum.
    /// @return The calculated checksum value.
    /// @note Many Bothans died to bring us this information.
    static uint8_t calcBlockChecksum(const uint8_t *block, const uint16_t length)
    {
      uint8_t sum = kKelvinatorChecksumStart;
      // Sum the lower half of the first 4 bytes of this block.
      for (uint8_t i = 0; i < 4 && i < length - 1; i++, block++)
        sum += (*block & 0b1111);
      // then sum the upper half of the next 3 bytes.
      for (uint8_t i = 4; i < length - 1; i++, block++)
        sum += (*block >> 4);
      // Trim it down to fit into the 4 bits allowed. i.e. Mod 16.
      return sum & 0b1111;
    }

    void set_bits(remote_base::RemoteTransmitData &data, uint8_t byte, uint32_t bit_mark, uint32_t one_space, uint32_t zero_space, uint8_t length)
    {
      // Set data bits
      for (uint8_t j = 0; j < length; j++)
      {
        data.mark(bit_mark);
        bool bit = byte & (1 << j);
        data.space(bit ? one_space : zero_space);
      }
    }

    void set_bytes(remote_base::RemoteTransmitData &data, uint8_t remote_state[], uint32_t bit_mark, uint32_t one_space, uint32_t zero_space, uint8_t length, uint8_t offset)
    {
      // Set data bits
      for (uint8_t i = offset; i < length + offset; i++)
      {
        set_bits(data, remote_state[i], bit_mark, one_space, zero_space, 8);
      }
    }

    void GreeIRClimate::get_state_to_send(uint8_t remote_state[])
    {
      // Placeholder implementation: returns a static dummy array
      // uint8_t remote_state[GREE_STATE_FRAME_SIZE] = {0};
      GreeProtocol &gree_state = *reinterpret_cast<GreeProtocol *>(remote_state);

      if (this->mode != climate::CLIMATE_MODE_OFF)
      {
        gree_state.Power = GREE_POWER_ON;
        gree_state.Mode = this->operation_mode_();
      }
      else
      {
        gree_state.Power = GREE_POWER_OFF;
      }

      gree_state.Fan = this->fan_speed_();
      gree_state.Temp = this->target_temperature - GREE_TEMP_MIN;
      gree_state.SwingAuto = this->swing_auto_();
      gree_state.SwingV = this->vertical_swing_();
      gree_state.SwingH = this->horizontal_swing_();
      gree_state.Sleep = (this->preset == climate::CLIMATE_PRESET_SLEEP) ? 1 : 0;
      gree_state.ModelA = (this->get_model() == GreeIRModel::YAW1F || this->get_model() == GreeIRModel::YAC1FB9) ? 1 : 0;
      gree_state.WiFi = this->wifi_function_ ? 1 : 0;
      gree_state.unknown1 = 0b0101; // Don't know why
      gree_state.unknown2 = 0b100;  // Don't know why
      gree_state.Sum = calcBlockChecksum(gree_state.remote_state, GREE_STATE_FRAME_SIZE);
      gree_state.Light = 1; // Light on

      ESP_LOGD(TAG, "Sending Gree frame: %02X %02X %02X %02X %02X %02X %02X %02X",
               remote_state[0], remote_state[1], remote_state[2], remote_state[3],
               remote_state[4], remote_state[5], remote_state[6], remote_state[7]);
    }

    void GreeIRClimate::transmit_state()
    {

      // Determine timing based on model
      uint32_t header_mark = GREE_HEADER_MARK;
      uint32_t header_space = GREE_HEADER_SPACE;
      uint32_t bit_mark = GREE_BIT_MARK;
      uint32_t message_space = GREE_MESSAGE_SPACE;

      switch (this->model_)
      {
      case GreeIRModel::YAC1FB9:
        header_space = GREE_YAC1FB9_HEADER_SPACE;
        message_space = GREE_YAC1FB9_MESSAGE_SPACE;
        break;
      case GreeIRModel::YAW1F:
      case GreeIRModel::YBOFB:
        header_mark = GREE_YAC_HEADER_MARK;
        header_space = GREE_YAC_HEADER_SPACE;
        bit_mark = GREE_YAC_BIT_MARK;
        break;
      default:
        break;
      }

      uint8_t remote_state[GREE_STATE_FRAME_SIZE] = {0};
      this->get_state_to_send(remote_state);

      // Build IR data
      auto transmit = this->transmitter_->transmit();
      auto data = transmit.get_data();

      data->set_carrier_frequency(GREE_IR_FREQUENCY);

      for (int i = 0; i < this->repeat_; i++)
      {
        // Header
        data->mark(header_mark);
        data->space(header_space);

        set_bytes(*data, remote_state, bit_mark, GREE_ONE_SPACE, GREE_ZERO_SPACE, 4, 0); // block 1
        set_bits(*data, 0b010, bit_mark, GREE_ONE_SPACE, GREE_ZERO_SPACE, 3);            // block footer
        set_bits(*data, 0b1, bit_mark, message_space, message_space, 1);                 // message space
        set_bytes(*data, remote_state, bit_mark, GREE_ONE_SPACE, GREE_ZERO_SPACE, 4, 4); // block 2
        set_bits(*data, 0b1, bit_mark, message_space, message_space, 1);                 // message space
      }

      this->last_transmit_time_ = millis();
      transmit.perform();
    }

    bool get_bits(remote_base::RemoteReceiveData &data, uint8_t &byte, uint32_t bit_mark, uint32_t one_space, uint32_t zero_space, uint8_t length)
    {
      byte = 0;
      for (uint8_t j = 0; j < length; j++)
      {
        if (data.expect_item(bit_mark, one_space))
        {
          byte |= (1 << j);
        }
        else if (data.expect_item(bit_mark, zero_space))
        {
          byte |= (0 << j);
        }
        else
        {
          ESP_LOGVV(TAG, "Bit %d:%d failed. stream index=%d", i, j, data.get_index());
          return false;
        }
      }
      return true;
    }

    bool get_bytes(remote_base::RemoteReceiveData &data, uint8_t remote_state[], uint32_t bit_mark, uint32_t one_space, uint32_t zero_space, uint8_t length, uint8_t offset)
    { // Read data bits
      for (uint8_t i = offset; i < length + offset; i++)
      {
        uint8_t data_byte = 0;
        get_bits(data, data_byte, bit_mark, one_space, zero_space, 8);
        remote_state[i] = data_byte;
      }
      return true;
    }

    bool GreeIRClimate::on_receive(remote_base::RemoteReceiveData data)
    {
      if (millis() - this->last_transmit_time_ < 500)
      {
        ESP_LOGV(TAG, "Blocked receive because of recent transmission");
        return false;
      }

      auto raw = data.get_raw_data();
      ESP_LOGV(TAG, "Raw data has %zu items.", raw.size());
      for (size_t i = 0; i < raw.size(); i++)
      {
        ESP_LOGVV(TAG, "[%03d] %d", i, raw[i]);
      }

      if (raw.size() < 130)
      {
        ESP_LOGV(TAG, "Received data too short: %zu items", raw.size());
        return false;
      }
      if (raw.size() > 150)
      {
        ESP_LOGV(TAG, "Received data too long: %zu items", raw.size());
        return false;
      }

      uint32_t ir_frequency = GREE_IR_FREQUENCY;
      uint32_t header_mark = GREE_HEADER_MARK;
      uint32_t header_space = GREE_HEADER_SPACE;
      uint32_t bit_mark = GREE_BIT_MARK;
      uint32_t one_space = GREE_ONE_SPACE;
      uint32_t zero_space = GREE_ZERO_SPACE;
      uint32_t message_space = GREE_MESSAGE_SPACE;

      if (this->model_ == GreeIRModel::YAC1FB9)
      {
        header_space = GREE_YAC1FB9_HEADER_SPACE;
        message_space = GREE_YAC1FB9_MESSAGE_SPACE;
      }
      else if (this->model_ == GreeIRModel::YAW1F || this->model_ == GreeIRModel::YBOFB)
      {
        header_mark = GREE_YAC_HEADER_MARK;
        header_space = GREE_YAC_HEADER_SPACE;
        bit_mark = GREE_YAC_BIT_MARK;
      }

      // Check if this looks like a Gree IR signal
      if (!data.expect_item(header_mark, header_space))
      {
        ESP_LOGD(TAG, "Header fail");
        return false;
      }

      uint8_t remote_state[GREE_STATE_FRAME_SIZE];
      if (!get_bytes(data, remote_state, bit_mark, one_space, zero_space, 4, 0))
      {
        ESP_LOGD(TAG, "Block 1 parsing failed");
        return false;
      }

      uint8_t footer_data_byte = 0;
      get_bits(data, footer_data_byte, bit_mark, one_space, zero_space, 3);
      if (footer_data_byte != 0b010)
      {
        ESP_LOGD(TAG, "Block Footer failed at data index: %d", data.get_index());
        ESP_LOGD(TAG, "Expected 0b010, got %d", footer_data_byte);
        return false;
      }

      if (!data.expect_item(bit_mark, message_space))
      {
        ESP_LOGD(TAG, "Message space failed at data index: %d", data.get_index());
        return false;
      }

      if (!get_bytes(data, remote_state, bit_mark, one_space, zero_space, 4, 4))
      {
        ESP_LOGD(TAG, "Block 2 parsing failed");
        return false;
      }

      ESP_LOGV(TAG, "Received Gree frame: %02X %02X %02X %02X %02X %02X %02X %02X",
               remote_state[0], remote_state[1], remote_state[2], remote_state[3],
               remote_state[4], remote_state[5], remote_state[6], remote_state[7]);

      // Parse the received data
      return this->parse_state_frame_(remote_state);
    }

    bool GreeIRClimate::parse_state_frame_(const uint8_t frame[])
    {

      const GreeProtocol &parsed_frame = *reinterpret_cast<const GreeProtocol *>(frame);
      uint8_t checksum = calcBlockChecksum(frame, GREE_STATE_FRAME_SIZE);
      ESP_LOGV(TAG, "Calculated checksum: %02X", checksum);
      ESP_LOGV(TAG, "Received checksum: %02X", parsed_frame.Sum);

      if (checksum != parsed_frame.Sum)
      {
        if (this->check_checksum_)
        {
          ESP_LOGW(TAG, "Checksum mismatch: expected %02X, got %02X", checksum, parsed_frame.Sum);
          return false;
        }
        else
        {
          ESP_LOGD(TAG, "Checksum mismatch: expected %02X, got %02X. Ignoring checksum...", checksum, parsed_frame.Sum);
        }
      }

      // Parse power state
      if (parsed_frame.Power == GREE_POWER_ON)
      {
        // Parse mode
        switch (parsed_frame.Mode)
        {
        case GREE_MODE_COOL:
          this->mode = climate::CLIMATE_MODE_COOL;
          break;
        case GREE_MODE_HEAT:
          this->mode = climate::CLIMATE_MODE_HEAT;
          break;
        case GREE_MODE_AUTO:
          this->mode = climate::CLIMATE_MODE_HEAT_COOL;
          break;
        case GREE_MODE_DRY:
          this->mode = climate::CLIMATE_MODE_DRY;
          break;
        case GREE_MODE_FAN:
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
        default:
          ESP_LOGW(TAG, "Unknown mode: %d", mode);
          return false;
        }
        ESP_LOGV(TAG, "Parsed mode: %d", this->mode);

        // Parse temperature
        this->target_temperature = parsed_frame.Temp + GREE_TEMP_MIN;
        ESP_LOGV(TAG, "Parsed target temperature: %d", this->target_temperature);

        // Parse fan speed
        // uint8_t fan = (frame[0] & 0x30) >> 4;
        switch (parsed_frame.Fan)
        {
        case GREE_FAN_AUTO:
          this->fan_mode = climate::CLIMATE_FAN_AUTO;
          break;
        case GREE_FAN_LOW:
          this->fan_mode = climate::CLIMATE_FAN_LOW;
          break;
        case GREE_FAN_MEDIUM:
          this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
          break;
        case GREE_FAN_HIGH:
          this->fan_mode = climate::CLIMATE_FAN_HIGH;
          break;
        default:
          ESP_LOGW(TAG, "Unknown fan speed: %d", parsed_frame.Fan);
          break;
        }
        ESP_LOGV(TAG, "Parsed fan mode: %d", this->fan_mode);

        // Parse swing modes
        uint8_t vswing = parsed_frame.SwingV;
        uint8_t hswing = parsed_frame.SwingH;
        // uint8_t vswing = frame[4] & 0x0F;
        // uint8_t hswing = (frame[4] & 0xF0) >> 4;
        ESP_LOGVV(TAG, "unparsed vswing: %d", vswing);
        ESP_LOGVV(TAG, "unparsed hswing: %d", hswing);

        if (vswing == GREE_VDIR_SWING && hswing == GREE_HDIR_SWING)
        {
          this->swing_mode = climate::CLIMATE_SWING_BOTH;
        }
        else if (vswing == GREE_VDIR_SWING)
        {
          this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
        }
        else if (hswing == GREE_HDIR_SWING)
        {
          this->swing_mode = climate::CLIMATE_SWING_HORIZONTAL;
        }
        else
        {
          this->swing_mode = climate::CLIMATE_SWING_OFF;
        }

        ESP_LOGV(TAG, "parsed swing: %d", this->swing_mode);

        // Parse presets
        // if (frame[0] & (1 << 7))
        if (parsed_frame.Sleep)
        {
          this->preset = climate::CLIMATE_PRESET_SLEEP;
        }
        else
        {
          this->preset = climate::CLIMATE_PRESET_NONE;
        }
        ESP_LOGV(TAG, "parsed preset: %d", this->preset);
      }
      else
      {
        this->mode = climate::CLIMATE_MODE_OFF;
      }

      this->publish_state();
      return true;
    }

    uint8_t GreeIRClimate::operation_mode_()
    {
      switch (this->mode)
      {
      case climate::CLIMATE_MODE_COOL:
        return GREE_MODE_COOL;
      case climate::CLIMATE_MODE_HEAT:
        return GREE_MODE_HEAT;
      case climate::CLIMATE_MODE_HEAT_COOL:
        return GREE_MODE_AUTO;
      case climate::CLIMATE_MODE_DRY:
        return GREE_MODE_DRY;
      case climate::CLIMATE_MODE_FAN_ONLY:
        return GREE_MODE_FAN;
      case climate::CLIMATE_MODE_OFF:
      default:
        return GREE_MODE_OFF;
      }
    }

    uint8_t GreeIRClimate::fan_speed_()
    {
      switch (this->fan_mode.value_or(climate::CLIMATE_FAN_AUTO))
      {
      case climate::CLIMATE_FAN_LOW:
        return GREE_FAN_LOW;
      case climate::CLIMATE_FAN_MEDIUM:
        return GREE_FAN_MEDIUM;
      case climate::CLIMATE_FAN_HIGH:
        return GREE_FAN_HIGH;
      case climate::CLIMATE_FAN_AUTO:
      default:
        return GREE_FAN_AUTO;
      }
    }

    uint8_t GreeIRClimate::vertical_swing_()
    {
      switch (this->swing_mode)
      {
      case climate::CLIMATE_SWING_VERTICAL:
      case climate::CLIMATE_SWING_BOTH:
        return GREE_VDIR_SWING;
      case climate::CLIMATE_SWING_OFF:
      case climate::CLIMATE_SWING_HORIZONTAL:
      default:
        return GREE_VDIR_MANUAL;
      }
    }

    uint8_t GreeIRClimate::horizontal_swing_()
    {
      switch (this->swing_mode)
      {
      case climate::CLIMATE_SWING_HORIZONTAL:
      case climate::CLIMATE_SWING_BOTH:
        return GREE_HDIR_SWING;
      case climate::CLIMATE_SWING_OFF:
      case climate::CLIMATE_SWING_VERTICAL:
      default:
        return GREE_HDIR_AUTO;
      }
    }

    uint8_t GreeIRClimate::swing_auto_()
    {
      switch (this->swing_mode)
      {
      case climate::CLIMATE_SWING_BOTH:
      case climate::CLIMATE_SWING_HORIZONTAL:
      case climate::CLIMATE_SWING_VERTICAL:
        return GREE_SWING_AUTO;
      case climate::CLIMATE_SWING_OFF:
      default:
        return GREE_SWING_MANUAL;
      }
    }

    climate::ClimateTraits GreeIRClimate::traits()
    {
      // auto traits = climate::ClimateTraits();
      auto traits = ClimateIR::traits();
      if (this->set_modes_)
      {
        traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                    climate::CLIMATE_MODE_COOL,
                                    climate::CLIMATE_MODE_HEAT,
                                    climate::CLIMATE_MODE_DRY,
                                    climate::CLIMATE_MODE_FAN_ONLY,
                                    climate::CLIMATE_MODE_HEAT_COOL});
      }
      return traits;
    }

  } // namespace greeir
} // namespace esphome