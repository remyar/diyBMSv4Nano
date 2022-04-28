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
#include "../Low-Level/Serial.h"
#include "./rtu.h"
#include "../bms/bms.h"
#include "../rules/rules.h"
#include "../settings/settings.h"

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

static unsigned long _ms = millis();

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

//------------------------------------------------------------------------------------------------//
//---                                        Partagees                                         ---//
//------------------------------------------------------------------------------------------------//
String _sCmd;
String _cmdToSend = "";

//--------------------------------------------------------------------------------------------------
// FONCTION    : KEYBOARD_Init
//
// DESCRIPTION : Initialisation de la carte : GPIO, Clocks, Interruptions...
//--------------------------------------------------------------------------------------------------
void _initRxBuffer(void)
{
    _sCmd = "";
}

void _initTxBuffer(void)
{
    _cmdToSend = "";
}

String _deleteFromChar(String cmd, String del)
{
    String result = cmd;
    while (result.startsWith(del) == false)
    {
        result.remove(0, 1);
    }
    result.remove(0, 1);
    return result;
}

void _processCommand(String cmd)
{
    _initTxBuffer();

    while (_sCmd.startsWith("[") == false)
    {
        _sCmd.remove(0, 1);
    }

    if (_sCmd.startsWith("[") == true)
    {
        _sCmd.remove(0, 1);
        if (_sCmd.startsWith("W") == true)
        {
            _sCmd.remove(0, 1);
            if (_sCmd.startsWith("CS") == true)
            {
                _sCmd.remove(0, 2);
                unsigned int cmiIdx = _sCmd.toInt();
                Serial.println(_sCmd.toInt());
                if (cmiIdx == 255)
                {
                    _sCmd = _deleteFromChar(_sCmd, ":");
                    SETTINGS_SetTotalNumberOfBanks(_sCmd.toInt());
                    Serial.println(_sCmd.toInt());
                    _sCmd = _deleteFromChar(_sCmd, ":");
                    SETTINGS_SetTotalNumberOfSeriesModules(_sCmd.toInt());
                    Serial.println(_sCmd.toInt());
                    _sCmd = _deleteFromChar(_sCmd, ":");
                    SETTINGS_SetBypassOverTempShutdown(_sCmd.toInt());
                    Serial.println(_sCmd.toInt());
                    _sCmd = _deleteFromChar(_sCmd, ":");
                    SETTINGS_SetBypassThresholdmV(_sCmd.toInt());
                    Serial.println(_sCmd.toInt());
                    BMS_SendGlobalConfig();
                }
                else
                {
                }
            }
        }
        else if (_sCmd.startsWith("R") == true)
        {
            _sCmd.remove(0, 1);
            if (_sCmd.startsWith("VS") == true)
            {
                _sCmd.remove(0, 2);
                unsigned int cmiIdx = _sCmd.toInt();
                CellModuleInfo *cmi = BMS_GetCMI(cmiIdx);
                Serial.print("[RVS");
                Serial.print(cmiIdx);
                Serial.print(":");
                Serial.print(cmi->valid);
                Serial.print(":");
                Serial.print(cmi->voltagemV);
                Serial.print(":");
                Serial.print(cmi->voltagemVMin);
                Serial.print(":");
                Serial.print(cmi->voltagemVMax);
                Serial.print(":");
                Serial.print(cmi->internalTemp);
                Serial.print(":");
                Serial.print(cmi->externalTemp);
                Serial.print(":");
                Serial.print(cmi->inBypass);
                Serial.print(":");
                Serial.print(cmi->PWMValue);
                Serial.print(":");
                Serial.print(cmi->badPacketCount);
                Serial.print(":");
                Serial.print(cmi->PacketReceivedCount);
                Serial.print(":");
                Serial.print(cmi->BalanceCurrentCount);
                Serial.print("]");
            }
        }
        else if (_sCmd.startsWith("ID") == true)
        {
            SERIAL_PrintString(_sCmd);

            _sCmd.remove(0, 2);
            unsigned int cmiIdx = _sCmd.toInt();

            SERIAL_PrintString(_sCmd);

            SERIAL_PrintString(String(cmiIdx));
            
            BMS_SendIdentify(cmiIdx);
        }

        while (_sCmd.startsWith("]") == false)
        {
            _sCmd.remove(0, 1);
        }

        //-- remove ']'
        _sCmd.remove(0, 1);
    }
}

void RTU_TaskInit(void)
{
    /* Serial.begin(9600);

     pinMode(4, OUTPUT);
     digitalWrite(4, LOW);*/

    _initRxBuffer();
    _initTxBuffer();
}

void RTU_TaskRun(void)
{
    if ((millis() - _ms) >= 1000 && _sCmd.length() == 0)
    {
        for (int i = 0; i < (settings.totalNumberOfSeriesModules * settings.totalNumberOfBanks); i++)
        {
            CellModuleInfo *cmi = BMS_GetCMI(i);
            Serial.print("[RVS");
            Serial.print(i);
            Serial.print(":");
            Serial.print(cmi->valid);
            Serial.print(":");
            Serial.print(cmi->voltagemV);
            Serial.print(":");
            Serial.print(cmi->voltagemVMin);
            Serial.print(":");
            Serial.print(cmi->voltagemVMax);
            Serial.print(":");
            Serial.print(cmi->internalTemp);
            Serial.print(":");
            Serial.print(cmi->externalTemp);
            Serial.print(":");
            Serial.print(cmi->inBypass);
            Serial.print(":");
            Serial.print(cmi->PWMValue);
            Serial.print(":");
            Serial.print(cmi->badPacketCount);
            Serial.print(":");
            Serial.print(cmi->PacketReceivedCount);
            Serial.print(":");
            Serial.print(cmi->BalanceCurrentCount);
            Serial.print("]");
        }
        _ms = millis();
    }

    if (SERIAL_Available())
    {
        _sCmd += (char)SERIAL_Getc();

        if (_sCmd.startsWith("[") == false)
        {
            _initRxBuffer();
        }
    }

    if (_sCmd.endsWith("]"))
    {
        _processCommand(_sCmd);
        // _initRxBuffer();
        if (_cmdToSend.length() > 0)
        {
            SERIAL_PrintString(_cmdToSend);
            _initTxBuffer();
        }
    }
}
