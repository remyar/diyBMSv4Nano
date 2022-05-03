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
#include "../typedefs.h"
#include "./settings.h"

//================================================================================================//
//                                            DEFINES                                             //
//================================================================================================//

//================================================================================================//
//                                          ENUMERATIONS                                          //
//================================================================================================//

//================================================================================================//
//                                      STRUCTURES ET UNIONS                                      //
//================================================================================================//
diybms_eeprom_settings settings;

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

//--------------------------------------------------------------------------------------------------
// FONCTION    : KEYBOARD_Init
//
// DESCRIPTION : Initialisation de la carte : GPIO, Clocks, Interruptions...
//--------------------------------------------------------------------------------------------------
void SETTINGS_Load(void)
{
    settings.totalNumberOfBanks = EEPROM.read(0);
    settings.totalNumberOfSeriesModules = EEPROM.read(1);

    settings.totalNumberOfBanks = settings.totalNumberOfBanks < 1 ? 1 : settings.totalNumberOfBanks;
    settings.totalNumberOfBanks = settings.totalNumberOfBanks == 0xFF ? 1 : settings.totalNumberOfBanks;

    settings.totalNumberOfSeriesModules = settings.totalNumberOfSeriesModules < 1 ? 1 : settings.totalNumberOfSeriesModules;
    settings.totalNumberOfSeriesModules = settings.totalNumberOfSeriesModules == 0xFF ? 1 : settings.totalNumberOfSeriesModules;

    settings.BypassOverTempShutdown = EEPROM.read(2);

    UINT16UNION_t _v;
    _v.bytes[0] = EEPROM.read(3);
    _v.bytes[1] = EEPROM.read(4);
    settings.BypassThresholdmV = _v.number;

    settings.BypassOverTempShutdown = settings.BypassOverTempShutdown < 1 ? 6 : settings.BypassOverTempShutdown;
    settings.BypassOverTempShutdown = settings.BypassOverTempShutdown == 0xFF ? 6 : settings.BypassOverTempShutdown;

    settings.BypassThresholdmV = settings.BypassThresholdmV < 1 ? 3000 : settings.BypassThresholdmV;
    settings.BypassThresholdmV = settings.BypassThresholdmV == 0xFFFF ? 2500 : settings.BypassThresholdmV;

    for (int i = 0; i < RELAY_RULES; i++)
    {
        UINT32UNION_t v;

        v.bytes[0] = EEPROM.read(64 + (4 * i));
        v.bytes[1] = EEPROM.read(64 + (4 * i) + 1);
        v.bytes[2] = EEPROM.read(64 + (4 * i) + 2);
        v.bytes[3] = EEPROM.read(64 + (4 * i) + 3);
        settings.rulehysteresis[i] = v.number;

        v.bytes[0] = EEPROM.read(128 + (4 * i));
        v.bytes[1] = EEPROM.read(128 + (4 * i) + 1);
        v.bytes[2] = EEPROM.read(128 + (4 * i) + 2);
        v.bytes[3] = EEPROM.read(128 + (4 * i) + 3);
        settings.rulevalue[i] = v.number;

        settings.rulerelaystate[i][0] = (RelayState)EEPROM.read(192 + (4 * i));
        settings.rulerelaystate[i][1] = (RelayState)EEPROM.read(192 + (4 * i) + 1);
        settings.rulerelaystate[i][2] = (RelayState)EEPROM.read(192 + (4 * i) + 2);
       // settings.rulerelaystate[i][3] = (RelayState)EEPROM.read(192 + (4 * i) + 3);
    }

    settings.rulerelaydefault[0] = (RelayState)EEPROM.read(16);
    settings.rulerelaydefault[1] = (RelayState)EEPROM.read(16 + 1);
    settings.rulerelaydefault[2] = (RelayState)EEPROM.read(16 + 2);
    //settings.rulerelaydefault[3] = (RelayState)EEPROM.read(16 + 3);

    settings.rulevalue[ModuleOverVoltage] = settings.rulevalue[ModuleOverVoltage] < 0 ? 4150 : settings.rulevalue[ModuleOverVoltage];
    settings.rulevalue[ModuleOverVoltage] = settings.rulevalue[ModuleOverVoltage] == 0xFFFFFFFF ? 4150 : settings.rulevalue[ModuleOverVoltage];

    settings.rulevalue[ModuleUnderVoltage] = settings.rulevalue[ModuleUnderVoltage] < 0 ? 3000 : settings.rulevalue[ModuleUnderVoltage];
    settings.rulevalue[ModuleUnderVoltage] = settings.rulevalue[ModuleUnderVoltage] == 0xFFFFFFFF ? 3000 : settings.rulevalue[ModuleUnderVoltage];

    settings.rulevalue[ModuleOverTemperatureInternal] = settings.rulevalue[ModuleOverTemperatureInternal] < 0 ? 60 : settings.rulevalue[ModuleOverTemperatureInternal];
    settings.rulevalue[ModuleOverTemperatureInternal] = settings.rulevalue[ModuleOverTemperatureInternal] == 0xFFFFFFFF ? 60 : settings.rulevalue[ModuleOverTemperatureInternal];

    settings.rulevalue[ModuleUnderTemperatureInternal] = settings.rulevalue[ModuleUnderTemperatureInternal] < 0 ? 50 : settings.rulevalue[ModuleUnderTemperatureInternal];
    settings.rulevalue[ModuleUnderTemperatureInternal] = settings.rulevalue[ModuleUnderTemperatureInternal] == 0xFFFFFFFF ? 50 : settings.rulevalue[ModuleUnderTemperatureInternal];

    settings.rulevalue[ModuleOverTemperatureExternal] = settings.rulevalue[ModuleOverTemperatureExternal] < 0 ? 55 : settings.rulevalue[ModuleOverTemperatureExternal];
    settings.rulevalue[ModuleOverTemperatureExternal] = settings.rulevalue[ModuleOverTemperatureExternal] == 0xFFFFFFFF ? 55 : settings.rulevalue[ModuleOverTemperatureExternal];

    settings.rulevalue[ModuleUnderTemperatureExternal] = settings.rulevalue[ModuleUnderTemperatureExternal] < 0 ? 5 : settings.rulevalue[ModuleUnderTemperatureExternal];
    settings.rulevalue[ModuleUnderTemperatureExternal] = settings.rulevalue[ModuleUnderTemperatureExternal] == 0xFFFFFFFF ? 5 : settings.rulevalue[ModuleUnderTemperatureExternal];

    settings.rulevalue[BankOverVoltage] = settings.rulevalue[BankOverVoltage] < 0 ? 4200 : settings.rulevalue[BankOverVoltage];
    settings.rulevalue[BankOverVoltage] = settings.rulevalue[BankOverVoltage] == 0xFFFFFFFF ? 4200  : settings.rulevalue[BankOverVoltage];

    settings.rulevalue[BankUnderVoltage] = settings.rulevalue[BankUnderVoltage] < 0 ? 3000 : settings.rulevalue[BankUnderVoltage];
    settings.rulevalue[BankUnderVoltage] = settings.rulevalue[BankUnderVoltage] == 0xFFFFFFFF ? 3000  : settings.rulevalue[BankUnderVoltage];

    settings.rulevalue[Timer2] = settings.rulevalue[Timer2] < 0 ? 0 : settings.rulevalue[Timer2];
    settings.rulevalue[Timer2] = settings.rulevalue[Timer2] == 0xFFFFFFFF ? 0  : settings.rulevalue[Timer2];

    settings.rulevalue[Timer1] = settings.rulevalue[Timer1] < 0 ? 0 : settings.rulevalue[Timer1];
    settings.rulevalue[Timer1] = settings.rulevalue[Timer1] == 0xFFFFFFFF ? 0  : settings.rulevalue[Timer1];

    for (int i = 0; i < RELAY_RULES; i++){
        settings.rulehysteresis[i] = settings.rulehysteresis[i] == 0xFFFFFFFF ? settings.rulevalue[i] : settings.rulehysteresis[i];
    }

}

void SETTINGS_SetTotalNumberOfBanks(uint8_t val)
{
    settings.totalNumberOfBanks = val;
    EEPROM.write(0, val);
}

void SETTINGS_SetTotalNumberOfSeriesModules(uint8_t val)
{
    settings.totalNumberOfSeriesModules = val;
    EEPROM.write(1, val);
}

void SETTINGS_SetBypassOverTempShutdown(uint8_t val)
{
    settings.BypassOverTempShutdown = val;
    EEPROM.write(2, val);
}

void SETTINGS_SetBypassThresholdmV(uint16_t val)
{
    settings.BypassThresholdmV = val;
    UINT16UNION_t v = {.number = val};
    EEPROM.write(3, v.bytes[0]);
    EEPROM.write(4, v.bytes[1]);
}

void SETTINGS_SetRuleHysteresis(uint8_t idx, uint32_t val)
{
    settings.rulehysteresis[idx] = val;
    UINT32UNION_t v;
    v.number = val;
    EEPROM.write(64 + (4 * idx), v.bytes[0]);
    EEPROM.write(64 + (4 * idx) + 1, v.bytes[1]);
    EEPROM.write(64 + (4 * idx) + 2, v.bytes[2]);
    EEPROM.write(64 + (4 * idx) + 3, v.bytes[3]);
}

void SETTINGS_SetRuleValue(uint8_t idx, uint32_t val)
{
    settings.rulevalue[idx] = val;
    UINT32UNION_t v;
    v.number = val;
    EEPROM.write(128 + (4 * idx), v.bytes[0]);
    EEPROM.write(128 + (4 * idx) + 1, v.bytes[1]);
    EEPROM.write(128 + (4 * idx) + 2, v.bytes[2]);
    EEPROM.write(128 + (4 * idx) + 3, v.bytes[3]);
}

void SETTINGS_SetRelayState(uint8_t idx, uint8_t relay1, uint8_t relay2, uint8_t relay3, uint8_t relay4)
{
    settings.rulerelaystate[idx][0] = (RelayState)relay1;
    settings.rulerelaystate[idx][1] = (RelayState)relay2;
    settings.rulerelaystate[idx][2] = (RelayState)relay3;
    //settings.rulerelaystate[idx][3] = (RelayState)relay4;

    EEPROM.write(192 + (4 * idx), relay1);
    EEPROM.write(192 + (4 * idx) + 1, relay2);
    EEPROM.write(192 + (4 * idx) + 2, relay3);
    //EEPROM.write(192 + (4 * idx) + 3, relay4);
}

void SETTINGS_SetRelayDefaultState(uint8_t relay1, uint8_t relay2, uint8_t relay3, uint8_t relay4)
{
    settings.rulerelaydefault[0] = (RelayState)relay1;
    settings.rulerelaydefault[1] = (RelayState)relay2;
    settings.rulerelaydefault[2] = (RelayState)relay3;
    //settings.rulerelaydefault[3] = (RelayState)relay4;

    EEPROM.write(16, relay1);
    EEPROM.write(16 + 1, relay2);
    EEPROM.write(16 + 2, relay3);
    //EEPROM.write(16 + 3, relay4);
}