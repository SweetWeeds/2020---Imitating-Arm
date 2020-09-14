#include "sam160.h"
#include <time.h>

int main() {
    char val;
    MyMotor motor;
    u8 SamId = (u8) 0x0;
    u8 Torq = (u8) 0x0;
    
    u8 dat = motor.Standard_FirmwareVersionRead_CMD(SamId);
    printf("Version:%d\n", dat);

    motor.Close();
    return 0;
}