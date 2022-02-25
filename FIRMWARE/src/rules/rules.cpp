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
#include "./rules.h"
#include "../settings/settings.h"

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
static unsigned long _ms = millis();
s_CONTROLER sCtrl;
static int8_t numberOfBalancingModules;
// True if at least 1 module has an external temp sensor fitted
static bool moduleHasExternalTempSensor;
static uint8_t zeroVoltageModuleCount = 0;
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
void _RunRules(uint32_t *value, uint32_t *hysteresisvalue, bool emergencyStop, uint16_t mins)
{
    // At least 1 module is zero volt - not a problem whilst we are in stabilizing start up mode
    if (zeroVoltageModuleCount > 0)
    {
        sCtrl.rule_outcome[Rule::ModuleOverVoltage] = false;
        sCtrl.rule_outcome[Rule::ModuleUnderVoltage] = false;
        sCtrl.rule_outcome[Rule::ModuleOverTemperatureInternal] = false;
        sCtrl.rule_outcome[Rule::ModuleUnderTemperatureInternal] = false;
        sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] = false;
        sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] = false;

        // Abort processing any more rules until controller is stable/running state
        return;
    }

    if (sCtrl.highestCellVoltage > value[Rule::ModuleOverVoltage] && sCtrl.rule_outcome[Rule::ModuleOverVoltage] == false)
    {
        // Rule Individual cell over voltage - TRIGGERED
        sCtrl.rule_outcome[Rule::ModuleOverVoltage] = true;
    }
    else if (sCtrl.highestCellVoltage < hysteresisvalue[Rule::ModuleOverVoltage] && sCtrl.rule_outcome[Rule::ModuleOverVoltage] == true)
    {
        // Rule Individual cell over voltage - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::ModuleOverVoltage] = false;
    }

    if (sCtrl.lowestCellVoltage < value[Rule::ModuleUnderVoltage] && sCtrl.rule_outcome[Rule::ModuleUnderVoltage] == false)
    {
        // Rule Individual cell under voltage (mV) - TRIGGERED
        sCtrl.rule_outcome[Rule::ModuleUnderVoltage] = true;
    }
    else if (sCtrl.lowestCellVoltage > hysteresisvalue[Rule::ModuleUnderVoltage] && sCtrl.rule_outcome[Rule::ModuleUnderVoltage] == true)
    {
        // Rule Individual cell under voltage (mV) - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::ModuleUnderVoltage] = false;
    }

    // These rules only fire if external temp sensor actually exists
    if (moduleHasExternalTempSensor)
    {
        // Doesn't cater for negative temperatures on rule (int8 vs uint32)
        if (((uint8_t)sCtrl.highestExternalTemp > value[Rule::ModuleOverTemperatureExternal]) && sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] == false)
        {
            // Rule Individual cell over temperature (external probe)
            sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] = true;
        }
        else if (((uint8_t)sCtrl.highestExternalTemp < hysteresisvalue[Rule::ModuleOverTemperatureExternal]) && sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] == true)
        {
            // Rule Individual cell over temperature (external probe) - HYSTERESIS RESET
            sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] = false;
        }

        // Doesn't cater for negative temperatures on rule (int8 vs uint32)
        if (((uint8_t)sCtrl.lowestExternalTemp < value[Rule::ModuleUnderTemperatureExternal]) && sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] == false)
        {
            // Rule Individual cell UNDER temperature (external probe)
            sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] = true;
        }
        else if (((uint8_t)sCtrl.lowestExternalTemp > hysteresisvalue[Rule::ModuleUnderTemperatureExternal]) && sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] == true)
        {
            // Rule Individual cell UNDER temperature (external probe) - HYSTERESIS RESET
            sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] = false;
        }
    }
    else
    {
        sCtrl.rule_outcome[Rule::ModuleOverTemperatureExternal] = false;
        sCtrl.rule_outcome[Rule::ModuleUnderTemperatureExternal] = false;
    }

    // Internal temperature monitoring and rules
    // Does not cope with negative temperatures on rule (int8 vs uint32)
    if (((uint8_t)sCtrl.highestInternalTemp > value[Rule::ModuleOverTemperatureInternal]) && sCtrl.rule_outcome[Rule::ModuleOverTemperatureInternal] == false)
    {
        // Rule Individual cell over temperature (Internal probe)
        sCtrl.rule_outcome[Rule::ModuleOverTemperatureInternal] = true;
    }
    else if (((uint8_t)sCtrl.highestInternalTemp < hysteresisvalue[Rule::ModuleOverTemperatureInternal]) && sCtrl.rule_outcome[Rule::ModuleOverTemperatureInternal] == true)
    {
        // Rule Individual cell over temperature (Internal probe) - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::ModuleOverTemperatureInternal] = false;
    }

    // Doesn't cater for negative temperatures on rule (int8 vs uint32)
    if (((uint8_t)sCtrl.lowestInternalTemp < value[Rule::ModuleUnderTemperatureInternal]) && sCtrl.rule_outcome[Rule::ModuleUnderTemperatureInternal] == false)
    {
        // Rule Individual cell UNDER temperature (Internal probe)
        sCtrl.rule_outcome[Rule::ModuleUnderTemperatureInternal] = true;
    }
    else if (((uint8_t)sCtrl.lowestInternalTemp > hysteresisvalue[Rule::ModuleUnderTemperatureInternal]) && sCtrl.rule_outcome[Rule::ModuleUnderTemperatureInternal] == true)
    {
        // Rule Individual cell UNDER temperature (Internal probe) - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::ModuleUnderTemperatureInternal] = false;
    }

    // While Pack voltages
    if (sCtrl.highestPackVoltage > value[Rule::BankOverVoltage] && sCtrl.rule_outcome[Rule::BankOverVoltage] == false)
    {
        // Rule - Pack over voltage (mV)
        sCtrl.rule_outcome[Rule::BankOverVoltage] = true;
    }
    else if (sCtrl.highestPackVoltage < hysteresisvalue[Rule::BankOverVoltage] && sCtrl.rule_outcome[Rule::BankOverVoltage] == true)
    {
        // Rule - Pack over voltage (mV) - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::BankOverVoltage] = false;
    }

    if (sCtrl.lowestPackVoltage < value[Rule::BankUnderVoltage] && sCtrl.rule_outcome[Rule::BankUnderVoltage] == false)
    {
        // Rule - Pack under voltage (mV)
        sCtrl.rule_outcome[Rule::BankUnderVoltage] = true;
    }
    else if (sCtrl.lowestPackVoltage > hysteresisvalue[Rule::BankUnderVoltage] && sCtrl.rule_outcome[Rule::BankUnderVoltage] == true)
    {
        // Rule - Pack under voltage (mV) - HYSTERESIS RESET
        sCtrl.rule_outcome[Rule::BankUnderVoltage] = false;
    }
}

void _ClearValues(void)
{
    for (uint8_t r = 0; r < maximum_number_of_banks; r++)
    {
        sCtrl.packvoltage[r] = 0;
        sCtrl.lowestvoltageinpack[r] = 0xFFFF;
        sCtrl.highestvoltageinpack[r] = 0;
    }

    sCtrl.highestPackVoltage = 0;
    sCtrl.lowestPackVoltage = 0xFFFFFFFF;
    sCtrl.highestCellVoltage = 0;
    sCtrl.lowestCellVoltage = 0xFFFF;
    sCtrl.highestExternalTemp = -127;
    sCtrl.lowestExternalTemp = 127;
    sCtrl.highestInternalTemp = -127;
    sCtrl.lowestInternalTemp = 127;
    moduleHasExternalTempSensor = false;
    zeroVoltageModuleCount = 0;
}

void _ProcessCell(uint8_t bank, uint8_t cellNumber, CellModuleInfo *c)
{
    if (c->valid == false)
    {
        return;
    }

    sCtrl.packvoltage[bank] += c->voltagemV;
    
    // If the voltage of the module is zero, we probably haven't requested it yet (which happens during power up)
    // so keep count so we don't accidentally trigger rules.
    if (c->voltagemV == 0)
    {
        zeroVoltageModuleCount++;
    }

    if (c->voltagemV > sCtrl.highestvoltageinpack[bank])
    {
        sCtrl.highestvoltageinpack[bank] = c->voltagemV;
    }
    if (c->voltagemV < sCtrl.lowestvoltageinpack[bank])
    {
        sCtrl.lowestvoltageinpack[bank] = c->voltagemV;
    }

    if (c->voltagemV > sCtrl.highestCellVoltage)
    {
        sCtrl.highestCellVoltage = c->voltagemV;
        sCtrl.address_HighestCellVoltage = cellNumber;
    }

    if (c->voltagemV < sCtrl.lowestCellVoltage)
    {
        sCtrl.lowestCellVoltage = c->voltagemV;
        sCtrl.address_LowestCellVoltage = cellNumber;
    }

    if (c->externalTemp != -40)
    {
        moduleHasExternalTempSensor = true;

        if (c->externalTemp > sCtrl.highestExternalTemp)
        {
            sCtrl.highestExternalTemp = c->externalTemp;
            sCtrl.address_highestExternalTemp = cellNumber;
        }

        if (c->externalTemp < sCtrl.lowestExternalTemp)
        {
            sCtrl.lowestExternalTemp = c->externalTemp;
            sCtrl.address_lowestExternalTemp = cellNumber;
        }
    }

    if (c->internalTemp > sCtrl.highestInternalTemp)
    {
        sCtrl.highestInternalTemp = c->internalTemp;
    }

    if (c->externalTemp < sCtrl.lowestInternalTemp)
    {
        sCtrl.lowestInternalTemp = c->internalTemp;
    }
}

void _ProcessBank(uint8_t bank)
{
    // Combine the voltages - work out the highest and lowest pack voltages
    if (sCtrl.packvoltage[bank] > sCtrl.highestPackVoltage)
    {
        sCtrl.highestPackVoltage = sCtrl.packvoltage[bank];
    }
    if (sCtrl.packvoltage[bank] < sCtrl.lowestPackVoltage)
    {
        sCtrl.lowestPackVoltage = sCtrl.packvoltage[bank];
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
void RULES_TaskInit(void)
{
    _ClearValues();
    sCtrl.isStarted = false;
    for (int i = 0; i < RELAY_TOTAL; i++)
    {
        sCtrl.previousRelayState[i] = RELAY_OFF;
    }
    _ms = millis();
}

void RULES_TaskRun(void)
{
    if ((millis() - _ms) >= 1000)
    {
        _ms = millis();
        _ClearValues();
        uint8_t cellid = 0;
        for (int8_t bank = 0; bank < settings.totalNumberOfBanks ; bank++)
        {
            for (int8_t i = 0; i < settings.totalNumberOfSeriesModules; i++)
            {
                _ProcessCell(bank, cellid, &cmi[cellid]);
                if (cmi[cellid].valid && cmi[cellid].settingsCached)
                {
                    if (cmi[cellid].inBypass)
                    {
                        numberOfBalancingModules++;
                    }
                }
                cellid++;
            }

            _ProcessBank(bank);
        }

        _RunRules(settings.rulevalue, settings.rulehysteresis, false, 0);

        RelayState relay[RELAY_TOTAL];

        // Set defaults based on configuration
        for (int8_t y = 0; y < RELAY_TOTAL; y++)
        {
            relay[y] = settings.rulerelaydefault[y] == RELAY_ON ? RELAY_ON : RELAY_OFF;
        }

        // Test the rules (in reverse order)
        for (int8_t n = RELAY_RULES - 1; n >= 0; n--)
        {
            if (sCtrl.rule_outcome[n] == true)
            {
                for (int8_t y = 0; y < RELAY_TOTAL; y++)
                {
                    // Dont change relay if its set to ignore/X
                    if (settings.rulerelaystate[n][y] != RELAY_X)
                    {
                        if (settings.rulerelaystate[n][y] == RELAY_ON)
                        {
                            relay[y] = RELAY_ON;
                        }
                        else
                        {
                            relay[y] = RELAY_OFF;
                        }
                    }
                }
            }
        }

        uint8_t changes = 0;
        for (int8_t n = 0; n < RELAY_TOTAL; n++)
        {
            if (sCtrl.previousRelayState[n] != relay[n])
            {
                changes++;
                switch (n)
                {
                case 0:
                    GPIO_WRITE(GPIO_PIN_RELAY_1, relay[n] == RELAY_ON ? true : false);
                    break;
                case 1:
                    GPIO_WRITE(GPIO_PIN_RELAY_2, relay[n] == RELAY_ON ? true : false);
                    break;
                case 2:
                    GPIO_WRITE(GPIO_PIN_RELAY_3, relay[n] == RELAY_ON ? true : false);
                    break;
                case 3:
                    GPIO_WRITE(GPIO_PIN_RELAY_4, relay[n] == RELAY_ON ? true : false);
                    break;
                default:
                    break;
                }

                sCtrl.previousRelayState[n] = relay[n];
            }
        }
    }
}

s_CONTROLER *RULES_GetControllerPtr(void)
{
    return &sCtrl;
}