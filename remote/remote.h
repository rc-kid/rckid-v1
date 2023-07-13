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
            enum class Mode : uint8_t {
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

                static Control Coast() { return Control{Mode::Coast, 0}; }
                static Control Brake() { return Control{Mode::Brake, 0}; }
                static Control CW(uint8_t speed) { return Control{Mode::CW, speed}; }
                static Control CCW(uint8_t speed) { return Control{Mode::CCW, speed}; }

            } __attribute__((packed));

            struct Feedback {
                bool overcurrent;
                uint8_t v;
                uint8_t i;
            } __attribute((__packed__));

            struct Config {
                uint8_t overcurrent;
            } __attribute__((packed));

            Control control;
            Config config;

            bool isSpinning() const {
                return control.mode == Mode::CW || control.mode == Mode::CCW;
            }
        }; // channel::Motor

        class CustomIO {
        public:
            enum class Mode : uint8_t {
                DigitalIn = 0, 
                DigitalOut = 1, 
                AnalogIn = 2, // ADC
                PWM = 3, // PWM
                Servo = 4, // Servo pulse 
            }; 

            struct Control {
                uint16_t value;
            } __attribute__((packed)); 

            struct Feedback {
                uint8_t value;
            } __attribute((__packed__));

            /** The custom IO channel is configured by its mode of operation, and the min and max pulse width for servo control, specified in microseconds. By default, this corresponds to a 270 degree standard servo with neutral position at 1500uS. You may wish to experiment with other values for particular servos.  
            */
            struct Config {
                Mode mode = Mode::DigitalIn;
                uint16_t servoStart = 500; // 0.5ms
                uint16_t servoEnd = 2500; // 2.5ms
                bool pullup = false;

                static Config input() { Config c; return c; }
                static Config inputPullup() { Config c; c.pullup = true; return c; }
                static Config output() { Config c; c.mode = Mode::DigitalOut; return c; }

            } __attribute__((packed));

            Control control;
            Config config;
        }; // channel::CustomIO


        /** 
         
            Hi-low siren = 450/600Hz in 0.5 sec interval
            Wail siren = 600 - 1200Hz sweep, sweep takes ~3 seconds up, 3 seconds down

            wail siren = 600-1200Hz sweep
         */
        class ToneEffect {
        public:
            enum class Effect : uint8_t {
                Tone, 
                Wail, // freq -> freq2 -> freq
                Yelp, // freq -> freq2
                HiLow, // freq | freq2 | freq
                Pulse, // freq | pause ...
            }; // Effect

            struct Control {
                Effect effect;
                uint16_t freq;
                uint16_t freq2;
                uint16_t transition;

                static Control tone(uint16_t freq, uint16_t duration = 0) {
                    return Control{Effect::Tone, freq, 0, duration};
                }

                static Control wail(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::Wail, fStart, fEnd, duration};
                }

                static Control yelp(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::Yelp, fStart, fEnd, duration};
                }

                static Control hiLow(uint16_t fStart, uint16_t fEnd, uint16_t duration) {
                    return Control{Effect::HiLow, fStart, fEnd, duration};
                }

                static Control pulse(uint16_t f, uint16_t dTone, uint16_t dNoTone) {
                    return Control{Effect::Pulse, f, dTone, dNoTone};
                }

            } __attribute__((packed)); 

            struct Feedback {

            } __attribute((__packed__));

            /** Configuration is the channel */
            struct Config {
                uint8_t outputChannel;
            } __attribute__((packed));
            Control control;
            Config config;
        }; // channel::ToneEffect

        class RGBStrip {
        public:
            struct Control {

            } __attribute__((packed)); 
            struct Feedback {

            } __attribute((__packed__));

            struct Config {

            } __attribute__((packed));
            Control control;
            Config config;
        }; // channel::RGBStrip

        class RGBColor {
        public:
            struct Control {

            } __attribute__((packed)); 
            struct Feedback {

            } __attribute__((packed));
            struct Config {

            } __attribute__((packed));
        }; // channel::RGBColor


    } // namespace remote::channel

    /** 

    */
    namespace msg {

        /** When incoming packet starts with 0, the packet is a command packet, in which the second byte is the command itself, followed by up to 30 bytes of data depending on the command itself. If the first byte is non-zero, it is interpreted as a channel control packet message. 
         */
        constexpr uint8_t CommandPacket = 0;

        enum class Kind : uint8_t {
            SetChannelConfig = 1,
            GetChannelConfig,
            SetChannelControl,
            GetChannelControl,
            GetChannelFeedback, 
            Feedback = 0x80,
            FeedbackConsecutive, 

            Error = 0xff,
        }; // msg::Kind

        /** Type of error returned. 
         */
        enum class ErrorKind : uint8_t {
            InvalidCommand, 
            InvalidChannel,

            Unimplemented,

        };

    } // namespace remote::msg



} // namespace remote