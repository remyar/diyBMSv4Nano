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
    EEPROM.get(0, settings.totalNumberOfBanks);
    EEPROM.get(1, settings.totalNumberOfSeriesModules);

    settings.totalNumberOfBanks = settings.totalNumberOfBanks < 1 ? 1 : settings.totalNumberOfBanks;
    settings.totalNumberOfSeriesModules = settings.totalNumberOfSeriesModules < 1 ? 1 : settings.totalNumberOfSeriesModules;
}

void SETTINGS_SetTotalNumberOfBanks(uint8_t val)
{
    settings.totalNumberOfBanks = val;
    EEPROM.write(0,val);
}

void SETTINGS_SetTotalNumberOfSeriesModules(uint8_t val)
{
    settings.totalNumberOfSeriesModules = val;
    EEPROM.write(1,val);
}
