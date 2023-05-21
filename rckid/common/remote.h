#pragma once


/** Remote communication protocol over NRF. 

    Each channel is static.  

    Control message followed by:
        channel, control

    Example for 5 bytes for motor
                3 bytes for pwm, servo, digital in/out
                4 bytes for tone


Commands

	- GetDeviceInfo (device name, sw version, etc.) 
	- GetRadioInfo (channel, name) 
	- SetRadioInfo (channel, name)
	- Connect()
	- Disconnect()
	- GetChannelInfo(num)
	- SetChannelInfo(num, data)




    # Commands

    All packets are always 32bytes long so that we can support the proper NRF chips as well as their clones. The controller sends commands to the device and the device sends feedback, or answers to the commands to the controller. 

    ### Control

    The default command from controller to device, i.e. a command with non-zero first byte is always interpreted as Control. The command data consists of tuples of channel number and channel control data for that channel. At least one channel must be specified for the message to valid. End of message is either the end of buffer, or 0 in the position of channel (channel 0 is reserved). 

    ### ControlEx [0x00 0x01 N S]

    Similar to control, but first specifies how many consecutive channels will be set by the message, followed by the index of the first channel. Then the control data for the channels follow w/o further channel specification

    ### GetChannelInfo [0x00 0x02 N]

    Requests channel information for channel N. Channel info is simply the channel's type. 

    ### GetChannelSetup [0x00 0x03 N]

    Requests the channel setup for given channel. 

    ### SetChannelSetup [0x00 0x04 N]

    Provides setup for given channel. 

    ### GetDeviceInfo [0x00 0x05]

    Requires device informantion. 



 
    Should work on both RPI and AVR side. Describes the channels, commands and stuff. 

    Each channel can be read, written and controlled

*/

namespace remote {

    namespace channel {

        enum class Type : uint8_t {
            Reserved = 0, 
            Motor = 1,
            CustomIO = 2, 
            RGB = 3, 
            RGBColor = 4,
        }; // channel::Type

        /** Brushed DC motor control. 
         
            Control:
                - mode (CV, CCV, Brake, Coast)
                - speed (0..255)
            Feedback:
                - Stall?
                - V
                - I
            Setup:
                - stall limit
                - stall alert
        */
        class Motor {
        public:
            enum class Mode : uint8_t {
                Coast, 
                CW, 
                CCW,
                Brake
            }; // Motor::Mode

            struct Control {
                Mode mode;
                uint8_t speed;
            } __attribute__((packed)); // Motor::Control

            struct Feedback {
                uint8_t overload;
                uint16_t voltage;
                uint16_t current;
            } __attribute__((packed)); // Motor::Feedback

            struct Setup {
                uint16_t stallLimit;
                uint8_t stallAlert;
            } __attribute__((packed)); // Motor::Setup

            Control control;
            Feedback feedback;
            Setup setup;
        }; // channel::Motor

        /** Customizable I/O channel. 
         
            Can operate in different modes. 
        */
        class CustomIO {
            enum class Mode : uint8_t {
                DigitalIn, 
                DigitalOut,
                AnalogIn, 
                AnalogOut,
                PWM, 
                Servo, 
                Tone 
            }; // CustomIO::Mode

            struct Control {
                uint16_t value;
            } __attribute__((packed)); // CustomIO::Control

            struct Feedback {
                uint16_t value;
            } __attribute__((packed)); 

            struct Setup {
                Mode mode;
                // TODO 
            } __attribute__((packed)); 

            uint16_t value;
            Setup setup;
        }; 


        class RGB {

        }; 

        class RGBColor {

        }; 


    } // namespace remote::channel


} // namespace remote