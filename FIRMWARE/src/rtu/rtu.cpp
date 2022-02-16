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
#include <ModbusSlave.h>

//AltSoftSerial porttwo;
Modbus slave(Serial, 1, 4);

//================================================================================================//
//                                            DEFINES                                             //
//================================================================================================//

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
        case 0:
        {
            //-- ping request
            slave.writeRegisterToBuffer(0, 1);
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
    case FC_READ_INPUT_REGISTERS:
    {
        uint8_t a = ((address & 0xFF00) >> 8);
        uint8_t d = address & 0x00FF;
        if (a >= 0 && a < maximum_controller_cell_modules)
        {

            CellModuleInfo *mod = &cmi[a];
            switch (d)
            {
            case (0):
            {
                slave.writeRegisterToBuffer(0, mod->badPacketCount);
                break;
            }
            case (1):
            {
                slave.writeRegisterToBuffer(0, mod->BalanceCurrentCount);
                break;
            }
            case (2):
            {
                slave.writeRegisterToBuffer(0, mod->BoardVersionNumber);
                break;
            }
            case (3):
            {
                slave.writeRegisterToBuffer(0, mod->bypassOverTemp);
                break;
            }
            case (4):
            {
                slave.writeRegisterToBuffer(0, mod->BypassOverTempShutdown);
                break;
            }
            case (5):
            {
                slave.writeRegisterToBuffer(0, mod->BypassThresholdmV);
                break;
            }
            case (6):
            {
                slave.writeRegisterToBuffer(0, mod->Calibration);
                break;
            }
            case (7):
            {
                slave.writeRegisterToBuffer(0, (mod->CodeVersionNumber & 0xFFFF0000) >> 16);
                slave.writeRegisterToBuffer(2, mod->CodeVersionNumber & 0xFFFF);
                break;
            }
            case (8):
            {
                slave.writeRegisterToBuffer(0, mod->External_BCoefficient);
                break;
            }
            case (9):
            {
                slave.writeRegisterToBuffer(0, mod->externalTemp);
                break;
            }
            case (10):
            {
                slave.writeRegisterToBuffer(0, mod->inBypass);
                break;
            }
            case (11):
            {
                slave.writeRegisterToBuffer(0, mod->Internal_BCoefficient);
                break;
            }
            case (12):
            {
                slave.writeRegisterToBuffer(0, mod->internalTemp);
                break;
            }
            case (13):
            {
                slave.writeRegisterToBuffer(0, mod->LoadResistance);
                break;
            }
            case (14):
            {
                slave.writeRegisterToBuffer(0, mod->mVPerADC);
                break;
            }
            case (15):
            {
                slave.writeRegisterToBuffer(0, mod->settingsCached);
                break;
            }
            case (16):
            {
                slave.writeRegisterToBuffer(0, mod->valid);
                break;
            }
            case (17):
            {
                slave.writeRegisterToBuffer(0, mod->voltagemV);
                break;
            }
            case (18):
            {
                slave.writeRegisterToBuffer(0, mod->voltagemVMax);
                break;
            }
            case (19):
            {
                slave.writeRegisterToBuffer(0, mod->voltagemVMin);
                break;
            }
            case (0xFF):
            {

                slave.writeRegisterToBuffer(0, mod->badPacketCount);
                slave.writeRegisterToBuffer(1, mod->BalanceCurrentCount);
                slave.writeRegisterToBuffer(2, mod->BoardVersionNumber);
                slave.writeRegisterToBuffer(3, mod->bypassOverTemp);
                slave.writeRegisterToBuffer(4, mod->BypassOverTempShutdown);
                slave.writeRegisterToBuffer(5, mod->BypassThresholdmV);
                slave.writeRegisterToBuffer(6, mod->Calibration);
                slave.writeRegisterToBuffer(7, (mod->CodeVersionNumber & 0xFFFF0000) >> 16);
                slave.writeRegisterToBuffer(8, mod->CodeVersionNumber & 0xFFFF);
                slave.writeRegisterToBuffer(9, mod->External_BCoefficient);
                slave.writeRegisterToBuffer(10, mod->externalTemp);
                slave.writeRegisterToBuffer(11, mod->inBypass);
                slave.writeRegisterToBuffer(12, mod->Internal_BCoefficient);
                slave.writeRegisterToBuffer(13, mod->internalTemp);
                slave.writeRegisterToBuffer(14, mod->LoadResistance);
                slave.writeRegisterToBuffer(15, mod->mVPerADC);
                slave.writeRegisterToBuffer(16, mod->PacketReceivedCount);
                slave.writeRegisterToBuffer(17, mod->PWMValue);
                slave.writeRegisterToBuffer(18, mod->settingsCached);
                slave.writeRegisterToBuffer(19, mod->valid);
                slave.writeRegisterToBuffer(20, mod->voltagemV);
                slave.writeRegisterToBuffer(21, mod->voltagemVMax);
                slave.writeRegisterToBuffer(22, mod->voltagemVMin);
                break;
            }
            default:
            {
                result = STATUS_ILLEGAL_DATA_VALUE;
                break;
            }
            }
        }
        else
        {
            result = STATUS_ILLEGAL_DATA_ADDRESS;
            break;
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

    Serial.begin(115200);
    slave.begin(115200);

    pinMode(4,OUTPUT);
    digitalWrite(4,LOW);
}

void RTU_TaskRun(void)
{
    slave.poll();
}
