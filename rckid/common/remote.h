#pragma once

/** Remote communication protocol over NRF. 
 
    Should work on both RPI and AVR side. Describes the channels, commands and stuff. 
*/

namespace remote {

    namespace channel {

        /** Motor control channel. 
         
            Supports 64 levels of speed, direction, coasting and braking. 
        */
        class Motor {
        public:
            enum class Mode : uint8_t {
                Coast = 0b10000000, 
                Brake = 0b11000000, 
                CV =    0b00000000,
                CCV =   0b01000000,
            }; // Motor::Mode

            Mode mode() const {
                return static_cast<Mode>(raw_ & 0b11000000);
            }

            unsigned speed() const {
                return raw_ & 0x3f;
            }

            static Motor coast() { return Motor{Kind::Coast, 0}; }

            static Motor brake() { return Motor{Kind::Brake, 0}; }

            static Motor cv(unsigned speed) { return Motor{Kind::CV, speed}; }

            static Motor ccv(unsigned speed) { return Motor{Kind::CCV, speed}; }

        private:

            Motor(Kind kind, unsigned speed):
                raw_{static_cast<uint8_t>(kind) | (speed > 0x3f ? 0x3f : speed)} {}

            // X D s s s s s s
            uint8_t raw_;
        }; // channel::Motor

        

    } // namespace remote::channel


} // namespace remote