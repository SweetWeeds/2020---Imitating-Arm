#include "sam160.h"
#include <time.h>

int main() {
    char val;
    MyMotor motor;
    u8 SamId = (u8) 0x0;
    u8 Torq = (u8) 0x0;
    
    u8 dat = motor.Standard_FirmwareVersionRead_CMD(SamId);
    printf("Version:%d\n", dat);
    motor.Quick_PosControl_CMD(SamId, Torq, (u8)0);
    
    for(int cnt = 0; cnt < 255; cnt+=5)
        motor.Quick_PosControl_CMD(SamId, Torq, (u8)cnt);
        //usleep(1000000);
        sleep(1);
    
    for(int cnt = 254; cnt >= 0; cnt-=5)
        motor.Quick_PosControl_CMD(SamId, Torq, (u8)cnt);
        sleep(1);
    
    motor.Close();

    return 0;
}