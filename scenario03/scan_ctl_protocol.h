////////////////////////////////////////////////////////////////////////////////
// Imperx EyeLife Ultrasound scanner control protocol
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// File: scan_ctl_protocol.h
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Declaration of packets for Ultrasound scanner control protocol
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Author:	Nikolay Bitkin (kola)
// Created:	25-aug-2014
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Copyright (C) 2011-2014 Imperx Inc. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef _SCAN_CTL_PROTOCOL_H_
#define _SCAN_CTL_PROTOCOL_H_

// Include ISO C9x  7.18  Integer types header.
// Comment next line, if its not needed
#include "stdint.h"

extern unsigned int g_id_base;

///////////////////////////////////////////////////////////////////////////////
// Win32 section
#ifdef WIN32
#endif //WIN32
///////////////////////////////////////////////////////////////////////////////

#define MAX_COMMAND_LENGTH  512
#define MAX_TRANSACTION_ID  0xFFFFFFFFU

/////////////////////////////////////////
// Uncomment this line to have the re-send support
// #define STREAM_RESENDS_SUPPORT

////////////////////////////////////////////////////////////////////////////////
//  types
#pragma pack(push)
#pragma pack (1)

//
typedef struct _UsCtlHeader
{
    uint32_t id;        // Transaction ID, Increment every Tablet transaction. Very helpful when debugging system.
    uint32_t opcode;    // Command opcode
    uint32_t length;	// Payload length
} UsCtlHeader;

// Acknowledge
typedef struct _UsCtlAck
{
    UsCtlHeader hdr;    // Header
    uint32_t status;	// Transaction Status
} UsCtlAck;

#define USCTL_GET_VERSION           0x00000000U  // Version (for debugging only)
#define USCTL_SET_MOTOR_PARAMS      0x00000001U  // Set parameters controlling Motor
#define USCTL_GET_MOTOR_PARAMS      0x00000002U  // Get parameters controlling Motor.
#define USCTL_SET_ACQU_PARAMS       0x00000003U  // Set parameters controlling Data Acquisition.
#define USCTL_GET_ACQU_PARAMS       0x00000004U  // Get parameters controlling Data Acquisition.
#define USCTL_GET_BATTERY_STATUS    0x00000005U  // Get Battery Status
#define USCTL_SPI_WRITE             0x00000006U  // SPI Write command
#define USCTL_SPI_READ              0x00000007U  // SPI Read command
#define USCTL_START_ACQ             0x00000008U  // Start acquisition
#define USCTL_STOP_ACQ              0x00000009U  // Stop acquisition
#define USCTL_FIRMWARE_UPDATE       0x0000000AU  // Upload new firmware
#define SPI_CMD_LENGTH              10

#define USCTL_STATUS_ERROR              0xFFFFFFFFU // General error state
#define USCTL_STATUS_OK                 0x00000000U // Everything is OK
#define USCTL_STATUS_ALREADY_RUN        0x00000001U // Server is already streaming
#define USCTL_STATUS_NOT_SUPPORTED      0x00000002U // Width or height or bytes per packet are not supported(in USCTL_START_ACQ command) by scaner
#define USCTL_STATUS_NOT_IMPLEMENTED    0x00000003U // Operation is not implemented
#define USCTL_STATUS_TRANSACTION_ERROR  0x00000004U // Transaction error (should be more than last one)


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Version request
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef UsCtlHeader UsCtlGetVersionIn;

// Scanner->Tablet response
typedef struct _UsCtlGetVersionOut
{
    UsCtlAck ack;           // Common ACK header
    uint32_t hw_ver;        // Hardware Version
    uint32_t fw_ver;        // Firmware Version
    uint32_t wifi_mac;      // WiFi MAC address
    uint32_t wifi_mac2;     // WiFi MAC address
    uint32_t wifi_fw_ver;   // WiFi Firmware Version
    uint32_t wifi_phy_ver;  // WiFi Phy Version
    uint32_t wifi_nwp_ver;  // WiFi NWP Version
    uint32_t wifi_rom_ver;  // WiFi ROM version
    uint32_t wifi_chip_id;  // WiFi Chip ID
    uint32_t spare0;
    uint32_t spare1;
} UsCtlGetVersionOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set Motor control parameters
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef struct _UsCtlSetMotorIn
{
    UsCtlHeader hdr;        // Header
    uint32_t cmd;           // Command
    uint32_t pwm_lo;        // PWM_Lo
    uint32_t pwm_hi;        // PWM_Hi
    uint32_t edge_filter;   // Edge Filter
} UsCtlSetMotorIn;

typedef struct _UsCtlSetMotorIn_v2
{
    UsCtlHeader hdr;        // Header
    uint32_t cmd;           // Command
    uint32_t pwm_lo;        // PWM_Lo
    uint32_t pwm_hi;        // PWM_Hi
    uint32_t edge_filter;   // Edge Filter
    uint32_t set_mode;      // Command Mode
    uint32_t rpm_target;    // RPM Scanner rate
} UsCtlSetMotorIn_v2;

// Scanner->Tablet response
typedef UsCtlAck UsCtlSetMotorOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Motor control parameters
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef UsCtlHeader UsCtlGetMotorIn;

// Scanner->Tablet response
typedef struct _UsCtlGetMotorOut
{
    UsCtlAck ack;           // Common ACK header
    uint32_t cmd;           // Command
    uint32_t pwm_lo;        // PWM_Lo
    uint32_t pwm_hi;        // PWM_Hi
    uint32_t edge_filter;   // Edge Filter
    uint32_t status;        // Status
    uint32_t speed;         // Actual Motor Speed
} UsCtlGetMotorOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Set acquisition parameters
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef struct _UsCtlSetAcqParamIn
{
    UsCtlHeader hdr;
    uint32_t ctl;                       // Control
    uint32_t lines_num;                 // Number of Lines
    uint32_t samples_per_line;          // Number of samples per line
    uint32_t holdoff_after;             // Holdoff after ping
    uint32_t holdoff_between;           // Holdoff between each line
    uint32_t ping_width;                // Ping Width
    uint32_t dvga_zone0;                // DVGA_Zone0
    uint32_t dvga_zone1;                // DVGA_Zone1
    uint32_t dvga_zone2;                // DVGA_Zone2
    uint32_t dvga_zone3;                // DVGA Zone3
    uint32_t dvga_rate0;                // DVGA_Rate0
    uint32_t dvga_rate1;                // DVGA Rate1
    uint32_t dvga_rate2;                // DVGA Rate2
    uint32_t dvga_rate3;                // DVGA Rate3
    uint32_t adc_sample_rate;           // ADC Sample Rate Divider
    uint32_t read_spacing;              // Decimation value for samples in line
    uint32_t holdoff_after_index_pulse; //
    uint32_t holdoff_rx_switch;         //
    uint32_t rx_beam_control;           //
    uint32_t rx_beam_base_addr;         //
    uint32_t derserializer_control;     //
    uint32_t wifi_control;              //
    uint32_t wifi_byte_spacing;         //
} UsCtlSetAcqParamIn;

// Scanner->Tablet response
typedef UsCtlAck UsCtlSetAcqParamOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get acquisition parameters
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef UsCtlHeader UsCtlGetAcqParamIn;

// Scanner->Tablet response
typedef struct _UsCtlGetAcqParamOut
{
    UsCtlAck ack;                       // Common ACK header
    uint32_t ctl;                       // Control
    uint32_t lines_num;                 // Number of Lines
    uint32_t samples_per_line;          // Number of samples per line
    uint32_t holdoff_after;             // Holdoff after ping
    uint32_t holdoff_between;           // Holdoff between each line
    uint32_t ping_width;                // Ping Width
    uint32_t dvga_zone0;                // DVGA_Zone0
    uint32_t dvga_zone1;                // DVGA_Zone1
    uint32_t dvga_zone2;                // DVGA_Zone2
    uint32_t dvga_zone3;                // DVGA Zone3
    uint32_t dvga_rate0;                // DVGA_Rate0
    uint32_t dvga_rate1;                // DVGA Rate1
    uint32_t dvga_rate2;                // DVGA Rate2
    uint32_t dvga_rate3;                // DVGA Rate3
    uint32_t adc_sample_rate;           // ADC Sample Rate Divider
    uint32_t read_spacing;              // Decimation value for samples in line
    uint32_t holdoff_after_index_pulse; //
    uint32_t holdoff_rx_switch;         //
    uint32_t rx_beam_control;           //
    uint32_t rx_beam_base_addr;         //
    uint32_t derserializer_control;     //
    uint32_t wifi_control;              //
    uint32_t wifi_byte_spacing;         //
    uint32_t status;                    // Status
    uint32_t cur_line_index;            // Current line in state machine
    uint32_t cur_sample_index;          // Current Sample in a Line
    uint32_t scan_width;                // Scanning area width in micrometers
    uint32_t scan_height;               // Scanning area height in micrometers
    uint32_t scan_start_angle;          // Scanning area start angle (microradians)
    uint32_t scan_finish_angle;         // Scanning area finish angle (microradians)
    uint32_t scan_start_radius;         // Scanning area start radius in micrometers
    uint32_t scan_finish_radius;        // Scanning area end radius in micrometers
} UsCtlGetAcqParamOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get Battery Status
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef  UsCtlHeader UsCtlGetBatteryStatusIn;

// Scanner->Tablet response
typedef struct _UsCtlGetBatteryStatusOut
{
	UsCtlAck ack;        // Common ACK header
    uint32_t  status;    // battery status
    uint32_t  voltage;   // voltage
    uint32_t  spare0;    // spare
} UsCtlGetBatteryStatusOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Write SPI Data
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enumeration of SPI subsystems
#ifdef MAC_DESKTOP_BUILD
typedef enum _SPISubSystemType
#else
typedef enum _SPISubSystemType : uint32_t
#endif
{
    TX_BEAMFORMER       = 0,    // 0 = Tx Beamformer,
    TX_RX_SWITCH        = 1,    // 1 = Tx/Rx Switch,
    RX_AFE_AMPLIFIER    = 2,    // 2 = Rx AFE Amplifier,
    RX_AFE_ADC          = 3,    // 3 = Rx AFE ADC,
    HV1_DAC             = 4,    // 4 = HV1 DAC	Controls HV1, 0 to +50 V
    HV2_DAC             = 5,    // 5 = HV2 DAC	Controls HV2, 0 to -50 V
    BAT_ADC             = 6,    // 6 = BAT ADC	Reading Li-Ion Battery voltage
    SUB_SYS_COUNT       = 7     // Quantity of subsystems
} SPISubSystemType;

// Tablet->Scanner request
typedef struct _UsCtlSPIWriteIn
{
    UsCtlHeader hdr;
    SPISubSystemType SubSystem_ID;  // subsystem where to write the data:
    uint32_t  data_count;           // number of elements in data array to write
    uint32_t  data[SPI_CMD_LENGTH]; // SPI data to write to
} UsCtlSPIWriteIn;

// Scanner->Tablet response
typedef UsCtlAck UsCtlSPIWriteOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Read SPI Data
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef struct _UsCtlSPIReadIn
{
    UsCtlHeader hdr;
    SPISubSystemType SubSystem_ID;  // subsystem where to write the data:
    uint32_t data_count;            // number of elements in data array to read
} UsCtlSPIReadIn;

// Scanner->Tablet response
typedef struct _UsCtlSPIReadOut
{
	UsCtlAck ack;                    // Common ACK header
    uint32_t  data_count;            // number of read out elements in data array
    uint32_t  data[SPI_CMD_LENGTH];  // read out SPI data
} UsCtlSPIReadOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Start scanning
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef struct _UsCtlStartAcqIn
{
    UsCtlHeader hdr;
    uint32_t ipAddress;         // IP Address of recipient (not used for TCP)
    uint32_t port;              // Port of recipient (not used for TCP)
#ifdef STREAM_RESENDS_SUPPORT
    uint32_t ipAddressResend;   // IP Address of resend server (not used for TCP)
    uint32_t portResend;        // Port of resend server (not used for TCP)
#endif // STREAM_RESENDS_SUPPORT
    uint32_t width;             // width of an image
    uint32_t height;            // height of an image
    uint32_t bytesPerPacket;    // quantity of raw data in bytes to be sent as one packet
    uint32_t fps;               // frames per second - only for test
    uint32_t heartbeat_count;   // frames per second - only for test
}UsCtlStartAcqIn;

// Scanner->Tablet response
typedef UsCtlAck UsCtlStartAcqOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Stop scanning
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef UsCtlHeader UsCtlStopAcqIn;

// Scanner->Tablet response
typedef UsCtlAck UsCtlStopAcqOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Firmware update
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tablet->Scanner request
typedef struct _UsCtlFirmwareUpdateIn
{
    UsCtlHeader hdr;
    uint32_t data_count;
    uint32_t data[];
}UsCtlFirmwareUpdateIn;

// Scanner->Tablet response
typedef UsCtlAck UsCtlFirmwareUpdateOut;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#endif //_SCAN_CTL_PROTOCOL_H_

