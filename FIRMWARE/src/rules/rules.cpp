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
void _ClearValues(void)
{
    for (uint8_t r = 0; r < maximum_number_of_banks; r++)
    {
        sCtrl.packvoltage[r] = 0;
        sCtrl.lowestvoltageinpack[r] = 0xFFFF;
        sCtrl.highestvoltageinpack[r] = 0;
    }
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

void _ProcessBank(uint8_t bank){
    //Combine the voltages - work out the highest and lowest pack voltages
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
    _ms = millis();
}

void RULES_TaskRun(void)
{
    if ((millis() - _ms) >= 1000)
    {
        _ms = millis();
        _ClearValues();
        uint8_t cellid = 0;
        for (int8_t bank = 0; bank < maximum_number_of_banks; bank++)
        {
            for (int8_t i = 0; i < maximum_cell_modules_per_packet; i++)
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
    }
}

s_CONTROLER * RULES_GetControllerPtr(void){
    return &sCtrl;
}