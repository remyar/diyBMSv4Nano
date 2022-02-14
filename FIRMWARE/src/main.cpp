#include <Arduino.h>
#include "./Low-Level/board.h"
#include "./bms/bms.h"
#include <ModbusSlave.h>
#include <SoftwareSerial.h>

static SoftwareSerial portOne2(12, 13);

Modbus slave(portOne2, 1, 14);

// Handel Read Input Registers (FC=04)
uint8_t ReadAnalogIn(uint8_t fc, uint16_t address, uint16_t length)
{
    // we only answer to function code 4
    if (fc != FC_READ_INPUT_REGISTERS)
        return;

    // write registers into the answer buffer
    for (int i = 0; i < length; i++)
    {
        slave.writeRegisterToBuffer(i, analogRead(address + i));
    }
    return STATUS_OK;
}

void setup()
{
    BOARD_Init();

    BMS_TaskInit();

    // register handler functions
    slave.cbVector[CB_READ_REGISTERS] = ReadAnalogIn;
    slave.begin(9600);
}

void loop()
{
    BMS_TaskRun();
    slave.poll();
}