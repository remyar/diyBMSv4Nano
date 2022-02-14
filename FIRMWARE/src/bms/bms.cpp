//================================================================================================//
//                                                                                                //
// PROJET       : DongleWifi GoodRace                                                             //
// MODULE       : Board                                                                           //
// DESCRIPTION  :                                                                                 //
// CREATION     : 27/01/2020                                                                      //
// HISTORIQUE   :                                                                                 //
//                                                                                                //
//================================================================================================//

//================================================================================================//
//                                        FICHIERS INCLUS                                         //
//================================================================================================//
#include "../Low-Level/board.h"
#include "./bms.h"
//#include "../settings/settings.h"
#include <SerialEncoder.h>
#include <SoftwareSerial.h>
#include "crc16.h"

SoftwareSerial portOne(10,11);

//================================================================================================//
//                                            DEFINES                                             //
//================================================================================================//
#define SERIAL_DATA portOne

//================================================================================================//
//                                          ENUMERATIONS                                          //
//================================================================================================//

//================================================================================================//
//                                      STRUCTURES ET UNIONS                                      //
//================================================================================================//

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 VARIABLES PRIVEES ET PARTAGEES                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------//
//---                                         Privees                                          ---//
//------------------------------------------------------------------------------------------------//
SerialEncoder myPacketSerial;

cppQueue requestQueue(sizeof(PacketStruct), 4, FIFO);
cppQueue replyQueue(sizeof(PacketStruct), 4, FIFO);

PacketRequestGenerator prg = PacketRequestGenerator(&requestQueue);
PacketReceiveProcessor receiveProc = PacketReceiveProcessor();

// Memory to hold in and out serial buffer
uint8_t SerialPacketReceiveBuffer[2 * sizeof(PacketStruct)];
static unsigned long _ms = millis();
static unsigned long _ms2 = millis();
//This large array holds all the information about the modules
CellModuleInfo cmi[maximum_controller_cell_modules];
uint16_t sequence = 0;

//------------------------------------------------------------------------------------------------//
//---                                        Partagees                                         ---//
//------------------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 FONCTIONS PRIVEES ET PARTAGEES                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------//
//---                                         Privees                                          ---//
//------------------------------------------------------------------------------------------------//

void onPacketReceived()
{
    PacketStruct ps;
    memcpy(&ps, SerialPacketReceiveBuffer, sizeof(PacketStruct));

    if ((ps.command & 0x0F) == COMMAND::Timing)
    {
        //Timestamp at the earliest possible moment
        uint32_t t = millis();
        ps.moduledata[2] = (t & 0xFFFF0000) >> 16;
        ps.moduledata[3] = t & 0x0000FFFF;
        //Ensure CRC is correct
        ps.crc = CRC16::CalculateArray((uint8_t *)&ps, sizeof(PacketStruct) - 2);
    }

    if (!replyQueue.push(&ps))
    {
        Serial.println("*Failed to queue reply*");
    }
}

//------------------------------------------------------------------------------------------------//
//---                                        Partagees                                         ---//
//------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------------------
// FONCTION    : KEYBOARD_Init
//
// DESCRIPTION : Initialisation de la carte : GPIO, Clocks, Interruptions...
//--------------------------------------------------------------------------------------------------
void BMS_TaskInit(void)
{
    //Receive is IO2 which means the RX1 plug must be disconnected for programming to work!
    SERIAL_DATA.begin(9600); // Serial for comms to modules

    myPacketSerial.begin(&SERIAL_DATA, &onPacketReceived, sizeof(PacketStruct), SerialPacketReceiveBuffer, sizeof(SerialPacketReceiveBuffer));
    _ms = millis() - 5000;
    _ms2 = millis() - 5000;
}

void BMS_TaskRun(void)
{
    if ((millis() - _ms) > 3000)
    {
        uint16_t i = 0;
        uint16_t max = 3;
        uint8_t startmodule = 0;

        while (i < max)
        {
            uint16_t endmodule = (startmodule + maximum_cell_modules_per_packet) - 1;

            //Limit to number of modules we have configured
            if (endmodule > max)
            {
                endmodule = max - 1;
            }

            prg.sendCellVoltageRequest(startmodule, endmodule);
            prg.sendCellTemperatureRequest(startmodule, endmodule);

            //If any module is in bypass then request PWM reading for whole bank
            for (uint8_t m = startmodule; m <= endmodule; m++)
            {
                if (cmi[m].inBypass)
                {
                    prg.sendReadBalancePowerRequest(startmodule, endmodule);
                    //We only need 1 reading for whole bank
                    break;
                }
            }

            //Move to the next bank
            startmodule = endmodule + 1;
            i += maximum_cell_modules_per_packet;
        }

        _ms = millis();
    }

    if ((millis() - _ms2) > 1000)
    {
        GPIO_BUILTIN_LED_ON();
        if (requestQueue.isEmpty() == false)
        {
            PacketStruct transmitBuffer;

            requestQueue.pop(&transmitBuffer);
            sequence++;
            transmitBuffer.sequence = sequence;

            if (transmitBuffer.command == COMMAND::Timing)
            {
                //Timestamp at the last possible moment
                uint32_t t = millis();
                transmitBuffer.moduledata[0] = (t & 0xFFFF0000) >> 16;
                transmitBuffer.moduledata[1] = t & 0x0000FFFF;
            }

            transmitBuffer.crc = CRC16::CalculateArray((uint8_t *)&transmitBuffer, sizeof(PacketStruct) - 2);
            myPacketSerial.sendBuffer((byte *)&transmitBuffer);
        }
        GPIO_BUILTIN_LED_OFF();
        _ms2 = millis();
    }

    if (replyQueue.isEmpty() == false)
    {
        PacketStruct ps;
        replyQueue.pop(&ps);
        if (!receiveProc.ProcessReply(&ps))
        {
        }
    }

    // Call update to receive, decode and process incoming packets
    myPacketSerial.checkInputStream();
}

CellModuleInfo *BMS_GetCMI(uint16_t idx)
{
    return &cmi[idx];
}

PacketRequestGenerator * BMS_GetPrg(void){
    return &prg;
}

PacketReceiveProcessor * BMS_GetReceiveProc(void){
    return &receiveProc;
}