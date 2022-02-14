#include <Arduino.h>

//#include <driver/uart.h>

//#include "EmbeddedFiles_Defines.h"

//#include "EmbeddedFiles_Integrity.h"

//#define TOUCH_SCREEN

#ifndef DIYBMS_DEFINES_H_
#define DIYBMS_DEFINES_H_

//Total number of cells a single controler can handle (memory limitation)
#define maximum_controller_cell_modules 16

//Maximum of 16 cell modules (don't change this!) number of cells to process in a single packet of data
#define maximum_cell_modules_per_packet 16

//Maximum number of banks allowed
//This also needs changing in default.htm (MAXIMUM_NUMBER_OF_BANKS)
#define maximum_number_of_banks 1

typedef union
{
    float number;
    uint8_t bytes[4];
    uint16_t word[2];
} FLOATUNION_t;

// Only the lowest 4 bits can be used!
enum COMMAND : uint8_t
{
    ResetBadPacketCounter = 0,
    ReadVoltageAndStatus = 1,
    Identify = 2,
    ReadTemperature = 3,
    ReadBadPacketCounter = 4,
    ReadSettings = 5,
    WriteSettings = 6,
    ReadBalancePowerPWM = 7,
    Timing = 8,
    ReadBalanceCurrentCounter = 9,
    ReadPacketReceivedCounter = 10,
    ResetBalanceCurrentCounter = 11
};

//NOTE THIS MUST BE EVEN IN SIZE (BYTES) ESP8266 IS 32 BIT AND WILL ALIGN AS SUCH!
struct PacketStruct
{
    uint8_t start_address;
    uint8_t end_address;
    uint8_t command;
    uint8_t hops;
    uint16_t sequence;
    uint16_t moduledata[maximum_cell_modules_per_packet];
    uint16_t crc;
} __attribute__((packed));

struct CellModuleInfo
{
    //Used as part of the enquiry functions
    bool settingsCached : 1;
    //Set to true once the module has replied with data
    bool valid : 1;
    //Bypass is active
    bool inBypass : 1;
    //Bypass active and temperature over set point
    bool bypassOverTemp : 1;

    uint16_t voltagemV;
    uint16_t voltagemVMin;
    uint16_t voltagemVMax;
    //Signed integer byte (negative temperatures)
    int8_t internalTemp;
    int8_t externalTemp;

    uint8_t BypassOverTempShutdown;
    uint16_t BypassThresholdmV;
    uint16_t badPacketCount;

    // Resistance of bypass load
    float LoadResistance;
    //Voltage Calibration
    float Calibration;
    //Reference voltage (millivolt) normally 2.00mV
    float mVPerADC;
    //Internal Thermistor settings
    uint16_t Internal_BCoefficient;
    //External Thermistor settings
    uint16_t External_BCoefficient;
    //Version number returned by code of module
    uint16_t BoardVersionNumber;
    //Last 4 bytes of GITHUB version
    uint32_t CodeVersionNumber;
    //Value of PWM timer for load shedding
    uint16_t PWMValue;

    uint16_t BalanceCurrentCount;
    uint16_t PacketReceivedCount;
};

// This enum holds the states the controller goes through whilst
// it stabilizes and moves into running state.
enum ControllerState : uint8_t
{
    Unknown = 0,
    PowerUp = 1,
    Stabilizing = 2,
    ConfigurationSoftAP = 3,
    Running = 255,
};


//This holds all the cell information in a large array array
extern CellModuleInfo cmi[maximum_controller_cell_modules];

#endif
