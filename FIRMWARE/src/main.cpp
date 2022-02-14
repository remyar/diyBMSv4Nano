#include <Arduino.h>
#include "./Low-Level/board.h"
#include "./bms/bms.h"

void setup()
{
    BOARD_Init();

    BMS_TaskInit();
}

void loop()
{
    BMS_TaskRun();
}