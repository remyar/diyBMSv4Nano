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
//#include <AltSoftSerial.h>
#include "./rtu.h"
#include "../bms/bms.h"
#include "../rules/rules.h"
#include "../settings/settings.h"
#include <ModbusSlave.h>

// AltSoftSerial porttwo;
Modbus slave(Serial, 1, 4);

//================================================================================================//
//                                            DEFINES                                             //
//================================================================================================//

//================================================================================================//
//                                          ENUMERATIONS                                          //
//================================================================================================//
// command byte
// WRRR CCCC
// W    = 1 bit indicator packet was processed (controller send (0) module processed (1))
// R    = 3 bits reserved not used
// C    = 4 bits command (16 possible commands)

// commands
//  1000 0000  = set bank identity
//  0000 0001  = read voltage and status
//  0000 0010  = identify module (flash leds)
//  0000 0011  = Read temperature
//  0000 0100  = Report number of bad packets
//  0000 0101  = Report settings/configuration

enum
{
    CONTOLLER_READ_VOLTAGE_AND_STATUS = 0x01,
    CONTROLLER_IDENTIFY = 0x02,
    CONTROLLER_READ_TEMPERATURE = 0x03,
    CONTROLLER_REPORT_NUMBER_OF_BAD_PACKET = 0x04,
    CONTROLLER_REPORT_CONFIGURATION = 0x05
};

//================================================================================================//
//                                      STRUCTURES ET UNIONS                                      //
//================================================================================================//

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 VARIABLES PRIVEES ET PARTAGEES                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------//
//---                                         Privees                                          ---//
//------------------------------------------------------------------------------------------------//

//------------------------------------------------------------------------------------------------//
//---                                        Partagees                                         ---//
//------------------------------------------------------------------------------------------------//

////////////////////////////////////////////////////////////////////////////////////////////////////
//                                 FONCTIONS PRIVEES ET PARTAGEES                                 //
////////////////////////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------------------------//
//---                                         Privees                                          ---//
//------------------------------------------------------------------------------------------------//
// Handel Read Input Registers (FC=04)
uint8_t CbReadRegisters(uint8_t fc, uint16_t address, uint16_t length)
{
    uint8_t result = STATUS_OK;
    switch (fc)
    {
    case FC_READ_HOLDING_REGISTERS:
    {
        switch (address)
        {
        case (CONTOLLER_READ_VOLTAGE_AND_STATUS):
        {
            s_CONTROLER *sctrl = RULES_GetControllerPtr();
            slave.writeRegisterToBuffer(0, sctrl->isStarted);
            for (uint8_t i = 0; i < ((length - 1) / 2); i++)
            {

                UINT32UNION_t val;
                val.number = sctrl->packvoltage[i];
                slave.writeRegisterToBuffer((i * 2) + 1, val.word[0]);
                slave.writeRegisterToBuffer(((i * 2) + 1) + 1, val.word[1]);
            }
            break;
        }
        case (CONTROLLER_IDENTIFY):
        {
            break;
        }
        case (CONTROLLER_READ_TEMPERATURE):
        {
            break;
        }
        case (CONTROLLER_REPORT_NUMBER_OF_BAD_PACKET):
        {
            break;
        }
        case (CONTROLLER_REPORT_CONFIGURATION):
        {
            break;
        }
            /*   case 0:
               {
                   //-- ping request
                   slave.writeRegisterToBuffer(0, 1);
                   break;
               }*/
        default:
        {
            result = STATUS_ILLEGAL_DATA_ADDRESS;
            break;
        }
        }
        break;
    }
    case FC_READ_INPUT_REGISTERS:
    {
        uint8_t cmiIdx = (address & 0xFF00) >> 8;
        address &= 0x00FF;

        switch (address)
        {
        case (CONTOLLER_READ_VOLTAGE_AND_STATUS):
        {
            //-- all data
            uint8_t j = cmiIdx;
            uint8_t i = 0;
            CellModuleInfo *cmi = BMS_GetCMI(cmiIdx);
            slave.writeRegisterToBuffer(i, cmi->valid);
            i++;
            slave.writeRegisterToBuffer(i, cmi->voltagemV);
            i++;
            slave.writeRegisterToBuffer(i, cmi->voltagemVMin);
            i++;
            slave.writeRegisterToBuffer(i, cmi->voltagemVMax);
            i++;
            slave.writeRegisterToBuffer(i, cmi->internalTemp);
            i++;
            slave.writeRegisterToBuffer(i, cmi->externalTemp);
            i++;
            slave.writeRegisterToBuffer(i, cmi->inBypass);
            i++;
            slave.writeRegisterToBuffer(i, cmi->PWMValue);
            i++;
            slave.writeRegisterToBuffer(i, cmi->badPacketCount);
            i++;
            slave.writeRegisterToBuffer(i, cmi->PacketReceivedCount);
            i++;
            slave.writeRegisterToBuffer(i, cmi->BalanceCurrentCount);
            break;
        }
        case (CONTROLLER_IDENTIFY):
        {
            break;
        }
        default:
        {
            result = STATUS_ILLEGAL_DATA_ADDRESS;
            break;
        }
        }
        break;
    }
    default:
    {
        result = STATUS_ILLEGAL_FUNCTION;
        break;
    }
    }
    return result;
}

uint8_t CbWriteRegisters(uint8_t fc, uint16_t address, uint16_t length)
{
    uint8_t result = STATUS_OK;
    switch (fc)
    {
    case (FC_WRITE_MULTIPLE_REGISTERS):
    {
        uint8_t cmiIdx = (address & 0xFF00) >> 8;
        address = address & 0x00FF;
        switch (address)
        {
        case (CONTROLLER_IDENTIFY):
        {
            BMS_SendIdentify(cmiIdx);
            break;
        }
        case (CONTROLLER_REPORT_CONFIGURATION):
        {
            if (cmiIdx == 0xFF)
            {
                // get uint16_t value from the request buffer.
                SETTINGS_SetTotalNumberOfBanks(slave.readRegisterFromBuffer(0));
                SETTINGS_SetTotalNumberOfSeriesModules(slave.readRegisterFromBuffer(1));
                SETTINGS_SetBypassOverTempShutdown(slave.readRegisterFromBuffer(2));
                SETTINGS_SetBypassThresholdmV(slave.readRegisterFromBuffer(3));

                BMS_SendGlobalConfig();
            }
            else
            {
                //-- pass to cmi
            }
            break;
        }
        }
        break;
    }
    }

    return result;
}
//------------------------------------------------------------------------------------------------//
//---                                        Partagees                                         ---//
//------------------------------------------------------------------------------------------------//

//--------------------------------------------------------------------------------------------------
// FONCTION    : KEYBOARD_Init
//
// DESCRIPTION : Initialisation de la carte : GPIO, Clocks, Interruptions...
//--------------------------------------------------------------------------------------------------
void RTU_TaskInit(void)
{
    slave.cbVector[CB_READ_REGISTERS] = CbReadRegisters;
    slave.cbVector[CB_WRITE_MULTIPLE_REGISTERS] = CbWriteRegisters;

    Serial.begin(9600);
    slave.begin(9600);

    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);
}

void RTU_TaskRun(void)
{
    slave.poll();
}
