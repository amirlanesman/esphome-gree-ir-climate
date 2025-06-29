#pragma once

// Copied from IRremoteESP8266

namespace esphome
{
    namespace greeir
    {
        union GreeProtocol
        {
            uint8_t remote_state[8]; ///< The state in native IR code form
            struct
            {
                // Byte 0
                uint8_t Mode : 3;
                uint8_t Power : 1;
                uint8_t Fan : 2;
                uint8_t SwingAuto : 1;
                uint8_t Sleep : 1;
                // Byte 1
                uint8_t Temp : 4;
                uint8_t TimerHalfHr : 1;
                uint8_t TimerTensHr : 2;
                uint8_t TimerEnabled : 1;
                // Byte 2
                uint8_t TimerHours : 4;
                uint8_t Turbo : 1;
                uint8_t Light : 1;
                uint8_t ModelA : 1; // model==YAW1F
                uint8_t Xfan : 1;
                // Byte 3
                uint8_t : 2;
                uint8_t TempExtraDegreeF : 1;
                uint8_t UseFahrenheit : 1;
                uint8_t unknown1 : 4; // value=0b0101
                // Byte 4
                uint8_t SwingV : 4;
                uint8_t SwingH : 3;
                uint8_t : 1;
                // Byte 5
                uint8_t DisplayTemp : 2;
                uint8_t IFeel : 1;
                uint8_t unknown2 : 3; // value = 0b100
                uint8_t WiFi : 1;
                uint8_t : 1;
                // Byte 6
                uint8_t : 8;
                // Byte 7
                uint8_t : 2;
                uint8_t Econo : 1;
                uint8_t : 1;
                uint8_t Sum : 4;
            };
        };
    }
}