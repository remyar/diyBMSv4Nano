#include <Arduino.h>
#include "./Low-Level/board.h"
#include "./bms/bms.h"
#include "./rtu/rtu.h"

void setup()
{
    BOARD_Init();

    BMS_TaskInit();
    RTU_TaskInit();
}

void loop()
{
    BMS_TaskRun();
    RTU_TaskRun();
}