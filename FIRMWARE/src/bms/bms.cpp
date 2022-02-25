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
#include "../rules/rules.h"
#include "../settings/settings.h"
#include <SerialEncoder.h>
#include <SoftwareSerial.h>
#include "crc16.h"

SoftwareSerial portOne(12, 11);

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

static cppQueue requestQueue(sizeof(PacketStruct), 6, FIFO);    //-- 40 * 6     = 240
static cppQueue replyQueue(sizeof(PacketStruct), 2, FIFO);      //-- 40 * 2     = 80

static PacketRequestGenerator prg = PacketRequestGenerator(&requestQueue);
static PacketReceiveProcessor receiveProc = PacketReceiveProcessor();

// Memory to hold in and out serial buffer
static uint8_t SerialPacketReceiveBuffer[2 * sizeof(PacketStruct)];
static unsigned long _ms = millis();
static unsigned long _cpt = 0;
// This large array holds all the information about the modules
CellModuleInfo cmi[maximum_controller_cell_modules];
static uint16_t sequence = 0;

static e_STATE_BMS bmsState = READ_VOLTAGE_AND_STATUS;

static uint16_t _dummyVar = 0;
// static uint8_t _sendIdentityTo = 0;
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
        // Timestamp at the earliest possible moment
        uint32_t t = millis();
        ps.moduledata[2] = (t & 0xFFFF0000) >> 16;
        ps.moduledata[3] = t & 0x0000FFFF;
        // Ensure CRC is correct
        ps.crc = CRC16::CalculateArray((uint8_t *)&ps, sizeof(PacketStruct) - 2);
    }

    if (!replyQueue.push(&ps))
    {
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
    // Receive is IO2 which means the RX1 plug must be disconnected for programming to work!
    SERIAL_DATA.begin(9600); // Serial for comms to modules

    myPacketSerial.begin(&SERIAL_DATA, &onPacketReceived, sizeof(PacketStruct), SerialPacketReceiveBuffer, sizeof(SerialPacketReceiveBuffer));
    _ms = millis();
    bmsState = SEND_TIMING_REQUEST;
    _cpt = 0;
}

void BMS_TaskRun(void)
{
    // Call update to receive, decode and process incoming packets
    myPacketSerial.checkInputStream();

    if (replyQueue.isEmpty() == false)
    {
        PacketStruct ps;
        replyQueue.pop(&ps);
        if (receiveProc.ProcessReply(&ps))
        {
        }
    }

    if ((millis() - _ms) >= 1000)
    {
        _ms = millis();
        _cpt++;
        if (((_cpt % 30) == 0) && (bmsState != SEND_IDENTIFY_REQUEST) && (bmsState != SEND_GLOBAL_CONFIG))
        {
            bmsState = SEND_TIMING_REQUEST;
        }
    }
    else
    {
        return;
    }

    switch (bmsState)
    {
    case (SEND_IDENTIFY_REQUEST):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }

        bmsState = READ_VOLTAGE_AND_STATUS;
        break;
    }
    case (SEND_TIMING_REQUEST):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }
        prg.sendTimingRequest();
        _dummyVar = 0;
        bmsState = SEND_SETTINGS_REQUEST;
        break;
    }
    case (SEND_SETTINGS_REQUEST):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }

        for (uint8_t module = _dummyVar; module < maximum_controller_cell_modules; module++)
        {

            prg.sendGetSettingsRequest(module);
            _dummyVar++;
            if (_dummyVar >= (settings.totalNumberOfBanks * settings.totalNumberOfSeriesModules) - 1)
            {
                bmsState = SEND_BALANCE_CURRENT_COUNT_REQUEST;
                break;
            }
            if ((_dummyVar % 4) == 0)
            {
                break;
            }
        }
        break;
    }
    case (SEND_BALANCE_CURRENT_COUNT_REQUEST):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }
        uint8_t startmodule = 0;
        uint16_t endmodule = (settings.totalNumberOfBanks * settings.totalNumberOfSeriesModules) - 1; //-- max module configured - 1
        prg.sendReadBalanceCurrentCountRequest(startmodule, endmodule);
        bmsState = SEND_PACKET_RECEIVED_REQUEST;
        break;
    }
    case (SEND_PACKET_RECEIVED_REQUEST):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }
        uint8_t startmodule = 0;
        uint16_t endmodule = (settings.totalNumberOfBanks * settings.totalNumberOfSeriesModules) - 1; //-- max module configured - 1
        prg.sendReadPacketsReceivedRequest(startmodule, endmodule);
        bmsState = SEND_BAD_PACKET_COUNTER;
        break;
    }
    case (SEND_BAD_PACKET_COUNTER):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }
        uint8_t startmodule = 0;
        uint16_t endmodule = (settings.totalNumberOfBanks * settings.totalNumberOfSeriesModules) - 1; //-- max module configured - 1
        prg.sendReadBadPacketCounter(startmodule, endmodule);
        bmsState = READ_VOLTAGE_AND_STATUS;
        s_CONTROLER *ctrl = RULES_GetControllerPtr();
        ctrl->isStarted = true;
        break;
    }
    case (READ_VOLTAGE_AND_STATUS):
    {
        if ((_cpt % 3) != 0)
        {
            break;
        }
        uint8_t startmodule = 0;
        uint16_t endmodule = (settings.totalNumberOfBanks * settings.totalNumberOfSeriesModules) - 1; //-- max module configured - 1

        prg.sendCellVoltageRequest(startmodule, endmodule);
        prg.sendCellTemperatureRequest(startmodule, endmodule);
        for (uint8_t m = startmodule; m <= endmodule; m++)
        {
            if (cmi[m].inBypass)
            {
                prg.sendReadBalancePowerRequest(startmodule, endmodule);
                // We only need 1 reading for whole bank
                break;
            }
        }
        break;
    }
    case (SEND_GLOBAL_CONFIG):
    {
        if (requestQueue.isEmpty() == false)
        {
            break;
        }

        bmsState = READ_VOLTAGE_AND_STATUS;
        break;
    }
    }

    if (requestQueue.isEmpty() == false)
    {
        GPIO_BUILTIN_LED_ON();
        PacketStruct transmitBuffer;

        requestQueue.pop(&transmitBuffer);
        sequence++;
        transmitBuffer.sequence = sequence;

        if (transmitBuffer.command == COMMAND::Timing)
        {
            // Timestamp at the last possible moment
            uint32_t t = millis();
            transmitBuffer.moduledata[0] = (t & 0xFFFF0000) >> 16;
            transmitBuffer.moduledata[1] = t & 0x0000FFFF;
        }

        transmitBuffer.crc = CRC16::CalculateArray((uint8_t *)&transmitBuffer, sizeof(PacketStruct) - 2);
        myPacketSerial.sendBuffer((byte *)&transmitBuffer);
        GPIO_BUILTIN_LED_OFF();
    }
}

uint8_t BMS_GetNbCells(void)
{
    return 0; // NbCellules;
}

CellModuleInfo *BMS_GetCMI(uint16_t idx)
{
    return &cmi[idx];
}

PacketRequestGenerator *BMS_GetPrg(void)
{
    return &prg;
}

PacketReceiveProcessor *BMS_GetReceiveProc(void)
{
    return &receiveProc;
}

void BMS_SendIdentify(uint8_t cmiIdx)
{
    prg.clearQueue();
    prg.sendIdentifyModuleRequest(cmiIdx);
    bmsState = SEND_IDENTIFY_REQUEST;
}

void BMS_SendGlobalConfig(void)
{
    prg.clearQueue();
    prg.sendSaveGlobalSetting(settings.BypassThresholdmV, settings.BypassOverTempShutdown);
    bmsState = SEND_GLOBAL_CONFIG;
}