#include <Arduino.h>
#include "./Low-Level/board.h"
#include "./bms/bms.h"
#include "./rules/rules.h"
#include "./rtu/rtu.h"
#include "./settings/settings.h"

void setup()
{
    BOARD_Init();
    SETTINGS_Load();
    
    RULES_TaskInit();
    BMS_TaskInit();
    RTU_TaskInit();
}

void loop()
{
    RTU_TaskRun();
    BMS_TaskRun();
   // RULES_TaskRun();
}