#pragma once

#include "platform/platform.h"

/** \page Remote Control Protocol

    

 */
namespace remote {

    namespace channel {
        /** Channel types supported by the remote protocol. 
         */
        enum class Kind {
            Motor = 1, 
            CustomIO = 2, 
            ToneEffect = 3, 
            RGBStrip = 4, 
            RGBColor = 5,
        }; // channel::Kind

        class Motor {
        public:
            enum class Mode {
                Coast, 
                Brake, 
                CW, 
                CCW
            }; // MotorChannel::Mode

            struct Control {
                Mode mode;
                uint8_t speed;

                bool operator == (Control const & other) const {
                    return mode == other.mode && speed == other.speed;
                }

                bool operator != (Control const & other) const {
                    return mode != other.mode || speed != other.speed;
                }
            };

            struct Feedback {
                bool overcurrent;
                uint8_t v;
                uint8_t i;
            };

            struct Config {
                uint8_t overcurrent;
            };

            Control control;
            Feedback feedback;
            Config config;

            bool isSpinning() const {
                return control.mode == Mode::CW || control.mode == Mode::CCW;
            }
        }; // channel::Motor

        class CustomIO {
        public:
            enum class Mode {
                DigitalIn = 0, 
                DigitalOut = 1, 
                AnalogIn = 2, // ADC
                PWM = 3, // PWM
                Servo = 4, // Servo pulse 
            }; 

            struct Control {
                uint16_t value;
            }; 

            struct Feedback {
                uint8_t value;
            };

            struct Config {
                Mode mode;
                uint16_t servoStart;
                uint16_t servoEnd;
            };

            Control control;
            Feedback feedback;
            Config config;

        }; // channel::CustomIO

        class ToneEffect {
        public:
            struct Control {

            }; 
            struct Feedback {

            };
            struct Config {

            };
        }; // channel::ToneEffect

        class RGBStrip {
        public:
            struct Control {

            }; 
            struct Feedback {

            };
            struct Config {

            };
        }; // channel::RGBStrip

        class RGBColor {
        public:
            struct Control {

            }; 
            struct Feedback {

            };
            struct Config {

            };
        }; // channel::RGBColor


    } // namespace remote::channel

    /** 

    */
    namespace msg {

        /** When incoming packet starts with 0, the packet is a command packet, in which the second byte is the command itself, followed by up to 30 bytes of data depending on the command itself. If the first byte is non-zero, it is interpreted as a channel control packet message. 
         */
        constexpr uint8_t CommandPacket = 0;

        enum class Kind : uint8_t {
            ResponseOK, 
            ResponseFail,
            SetChannelConfig,
            GetChannelConfig,
            SetChannelControl,
            GetChannelControl,
            GetChannelFeedback, 
        }; // msg::Kind

    } // namespace remote::msg



} // namespace remote