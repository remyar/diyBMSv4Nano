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

cppQueue requestQueue(sizeof(PacketStruct), 4, FIFO);
cppQueue replyQueue(sizeof(PacketStruct), 4, FIFO);

PacketRequestGenerator prg = PacketRequestGenerator(&requestQueue);
PacketReceiveProcessor receiveProc = PacketReceiveProcessor();

// Memory to hold in and out serial buffer
uint8_t SerialPacketReceiveBuffer[2 * sizeof(PacketStruct)];
static unsigned long _ms = millis();
static unsigned long _ms2 = millis();
// This large array holds all the information about the modules
CellModuleInfo cmi[maximum_controller_cell_modules];
uint16_t sequence = 0;

static e_STATE_BMS bmsState = BMS_START;
static uint8_t NbCellules = 0;

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
    // Receive is IO2 which means the RX1 plug must be disconnected for programming to work!
    SERIAL_DATA.begin(9600); // Serial for comms to modules

    myPacketSerial.begin(&SERIAL_DATA, &onPacketReceived, sizeof(PacketStruct), SerialPacketReceiveBuffer, sizeof(SerialPacketReceiveBuffer));
    _ms = millis();
    _ms2 = millis();

    bmsState = BMS_START;
    NbCellules = 0;
}



void BMS_TaskRun(void)
{
    switch (bmsState)
    {
    case (BMS_START):
    {
        if ((millis() - _ms) >= 1000)
        {
            _ms = millis();
            prg.sendGetSettingsRequest(NbCellules);
            NbCellules++;
            if (NbCellules >= maximum_cell_modules_per_packet)
            {
                bmsState = BMS_GET_NB_CELLS;
                _ms = millis();
            }
        }
        break;
    }
    case (BMS_GET_NB_CELLS):
    {
        if ((millis() - _ms) >= 3000)
        {
            NbCellules = 0;
            _ms = millis() - 5000;
            for (int j = 0; j < maximum_cell_modules_per_packet; j++)
            {
                CellModuleInfo *mod = &cmi[j];
                if (mod->BoardVersionNumber >= 440)
                {
                    NbCellules++;
                }
            }
            bmsState = BMS_RUN;
        }
        break;
    }
    case (BMS_RUN):
    {
        if ((millis() - _ms) >= 3000)
        {
            _ms = millis();
            uint16_t max = NbCellules;
            uint8_t startmodule = 0;
            uint16_t endmodule = (startmodule + maximum_cell_modules_per_packet) - 1;

            // Limit to number of modules we have configured
            if (endmodule > max)
            {
                endmodule = max - 1;
            }

            prg.sendCellVoltageRequest(startmodule, endmodule);
            prg.sendCellTemperatureRequest(startmodule, endmodule);

            // If any module is in bypass then request PWM reading for whole bank
            for (uint8_t m = startmodule; m <= endmodule; m++)
            {
                if (cmi[m].inBypass)
                {
                    prg.sendReadBalancePowerRequest(startmodule, endmodule);
                    // We only need 1 reading for whole bank
                    break;
                }
            }
        }
        break;
    }
    }

    if ((millis() - _ms2) >= 1000)
    {
        _ms2 = millis();
        GPIO_BUILTIN_LED_ON();
        if (requestQueue.isEmpty() == false)
        {
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
        }
        GPIO_BUILTIN_LED_OFF();
    }

    if (replyQueue.isEmpty() == false)
    {
        PacketStruct ps;
        replyQueue.pop(&ps);
        if (receiveProc.ProcessReply(&ps))
        {
 /* 
            for (int j = 0; j < 2; j++)
            {
                CellModuleInfo *mod = &cmi[j];

                Serial.println("Module " + String(j) + " ****************");
                 Serial.println("badPacketCount : " + String(mod->badPacketCount));
                    Serial.println("BalanceCurrentCount : " + String(mod->BalanceCurrentCount));
                Serial.println("BoardVersionNumber : " + String(mod->BoardVersionNumber));
                  Serial.println("bypassOverTemp : " + String(mod->bypassOverTemp));
                  Serial.println("BypassOverTempShutdown : " + String(mod->BypassOverTempShutdown));
                  Serial.println("BypassThresholdmV : " + String(mod->BypassThresholdmV));
                  Serial.println("CodeVersionNumber : " + String(mod->CodeVersionNumber));
                  Serial.println("External_BCoefficient : " + String(mod->External_BCoefficient));
                  Serial.println("externalTemp : " + String(mod->externalTemp));
                  Serial.println("inBypass : " + String(mod->inBypass));
                  Serial.println("Internal_BCoefficient : " + String(mod->Internal_BCoefficient));
                  Serial.println("internalTemp : " + String(mod->internalTemp));
                  Serial.println("LoadResistance : " + String(mod->LoadResistance));
                  Serial.println("mVPerADC : " + String(mod->mVPerADC));
                  Serial.println("PacketReceivedCount : " + String(mod->PacketReceivedCount));
                  Serial.println("PWMValue : " + String(mod->PWMValue));
                  Serial.println("settingsCached : " + String(mod->settingsCached));
                  Serial.println("valid : " + String(mod->valid));
                    Serial.println("voltagemV : " + String(mod->voltagemV));
               Serial.println("voltagemVMax : " + String(mod->voltagemVMax));
                 Serial.println("voltagemVMin : " + String(mod->voltagemVMin));
                Serial.println("");
            }*/
            
        }
    }

    // Call update to receive, decode and process incoming packets
    myPacketSerial.checkInputStream();
}

uint8_t BMS_GetNbCells(void){
    return NbCellules;
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