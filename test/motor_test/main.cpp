#include "sam160.h"
#include <time.h>
#include <iostream>

int main() {
    u8 val;
    MyMotor motor;
    u8 SamId = (u8) 0x0;
    u8 Torq = (u8) 0x4;
    //u8 TargetPos = (u8) 0x7F;
    //motor.Quick_PosControl_CMD(SamId, Torq, (u8)150);
    u8 ver = motor.Standard_FirmwareVersionRead_CMD(0x00);
    printf("%x\n", SamId);
    while(true) {
        printf("Input: ");
        scanf(&val);
        printf("val:%x\n", val);
        motor.Quick_PosControl_CMD(SamId, Torq, val);
    }
    //sleep(1);

    //for(int i=0; i<255; i++) {
    //    u16 dat = motor.Quick_PosControl_CMD(SamId, Torq, (u8)i);
    //    //u16 dat = motor.Quick_StatusRead_CMD(SamId);
    //    //printf("%d:%x\n", i, dat);
    //    usleep(10);
    //}
    //sleep(1);
    //for(int i=254; i>0; i--) {
    //    u16 dat = motor.Quick_PosControl_CMD(SamId, Torq, (u8)i);
    //    //u16 dat = motor.Quick_StatusRead_CMD(SamId);
    //    //printf("%d:%x\n", i, dat);
    //    usleep(10);
    //}
    motor.Close();
    return 0;
}