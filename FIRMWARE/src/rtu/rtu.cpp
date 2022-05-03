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
#include "../time/myTime.h"
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
static unsigned long _ms2 = millis();
static bool timeIsOver = true;
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

//--------------------------------------------------------------------------------------------------
// FONCTION    : KEYBOARD_Init
//
// DESCRIPTION : Initialisation de la carte : GPIO, Clocks, Interruptions...
//--------------------------------------------------------------------------------------------------
void _initRxBuffer(void)
{
    _sCmd = "";
}

void _deleteFromChar(String del)
{
    while (_sCmd.startsWith(del) == false)
    {
        _sCmd.remove(0, 1);
    }
    _sCmd.remove(0, 1);
}

void _processCommand(String cmd)
{
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
                if (cmiIdx == 255)
                {
                    _deleteFromChar(":");
                    SETTINGS_SetTotalNumberOfBanks(_sCmd.toInt());
                    _deleteFromChar(":");
                    SETTINGS_SetTotalNumberOfSeriesModules(_sCmd.toInt());
                    _deleteFromChar(":");
                    SETTINGS_SetBypassOverTempShutdown(_sCmd.toInt());
                    _deleteFromChar(":");
                    SETTINGS_SetBypassThresholdmV(_sCmd.toInt());

                    BMS_SendGlobalConfig();
                }
                else
                {
                }
            }
            else if (_sCmd.startsWith("CR") == true)
            {
                //-- controler rules
                _sCmd.remove(0, 2);
                unsigned int cmiIdx = _sCmd.toInt();

                _deleteFromChar(":");
                SETTINGS_SetRuleValue(cmiIdx, _sCmd.toInt());

                _deleteFromChar(":");
                SETTINGS_SetRuleHysteresis(cmiIdx, _sCmd.toInt());

                uint8_t _r[4];
                for (uint8_t i = 0; i < RELAY_TOTAL; i++)
                {
                    _deleteFromChar(":");
                    _r[i] = _sCmd.toInt();
                }

                SETTINGS_SetRelayState(cmiIdx, _r[0], _r[1], _r[2], _r[3]);
            }
            else if (_sCmd.startsWith("RS") == true)
            {
                _sCmd.remove(0, 2);
                uint8_t _r[4];
                for (uint8_t i = 0; i < RELAY_TOTAL; i++)
                {
                    _r[i] = _sCmd.toInt();
                    if (i < (RELAY_TOTAL - 1))
                    {
                        _deleteFromChar(":");
                    }
                }
                SETTINGS_SetRelayDefaultState(_r[0], _r[1], _r[2], _r[3]);
            }
            else if (_sCmd.startsWith("TIME") == true)
            {
                _sCmd.remove(0, 4);
                TIME_setMinutesSinceMidnight(_sCmd.toInt());
            }
        }
        else if (_sCmd.startsWith("R") == true)
        {
            _sCmd.remove(0, 1);
            if (_sCmd.startsWith("CONF") == true)
            {
                _sCmd.remove(0, 4);
                Serial.print("[RCONF");
                Serial.print(settings.totalNumberOfBanks);
                Serial.print(":");
                Serial.print(settings.totalNumberOfSeriesModules);
                Serial.print(":");
                Serial.print(settings.BypassOverTempShutdown);
                Serial.print(":");
                Serial.print(settings.BypassThresholdmV);
                Serial.print("]");
            }
            else if (_sCmd.startsWith("RULES") == true)
            {
                _sCmd.remove(0, 5);

                Serial.print("[RRULES");
                for (int i = 0; i < RELAY_RULES; i++)
                {
                    Serial.print(settings.rulevalue[i]);
                    Serial.print(":");
                    Serial.print(settings.rulehysteresis[i]);
                    Serial.print(":");
                    for (int8_t y = 0; y < RELAY_TOTAL; y++)
                    {
                        Serial.print(settings.rulerelaystate[i][y]);
                        Serial.print(":");
                    }
                }

                Serial.print(settings.rulerelaydefault[0]);
                Serial.print(":");
                Serial.print(settings.rulerelaydefault[1]);
                Serial.print(":");
                Serial.print(settings.rulerelaydefault[2]);

                Serial.print("]");
            }
        }
        else if (_sCmd.startsWith("ID") == true)
        {
            _sCmd.remove(0, 2);
            unsigned int cmiIdx = _sCmd.toInt();
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
    _initRxBuffer();
    _ms2 = millis();
    _ms = millis();
}

void RTU_TaskRun(void)
{
    if (SERIAL_Available())
    {
        _sCmd += (char)SERIAL_Getc();

        if (_sCmd.startsWith("[") == false)
        {
            _initRxBuffer();
        }
    }
    if ((((millis() - _ms2) >= 60000) || (timeIsOver == true)) && _sCmd.length() == 0)
    {
        _ms2 = millis();
        timeIsOver = false;
        Serial.print("[GTIME]");
    }

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

        Serial.print("[RCVS");
        s_CONTROLER *sCtrl = RULES_GetControllerPtr();
        for (int i = 0; i < RELAY_RULES; i++)
        {
            Serial.print(sCtrl->rule_outcome[i]);
            if (i < (RELAY_RULES - 1))
            {
                Serial.print(":");
            }
        }
        Serial.print("]");
        _ms = millis();
    }

    if (_sCmd.endsWith("]"))
    {
        _processCommand(_sCmd);
    }
}
