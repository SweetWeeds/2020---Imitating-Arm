// C library headers
#include <stdio.h>
#include <string.h>

// Linux headers
#include <fcntl.h> // Contains file controls like O_RDWR
#include <errno.h> // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

/*--------------------data definition ------------------------------------------------------------------------------------------------*/
typedef signed long s32; // 4 byte
typedef signed short s16; // 2 byte
typedef signed char s8; // 1 byte
typedef unsigned long u32; // 4 byte
typedef unsigned short u16; // 2 byte
typedef unsigned char u8; // 1 byte

class RS485 {
    private:
        struct termios tty;
        int serial_port = open("/dev/ttyUSB0", O_RDWR);
        char read_buf[1];
        void init();
    public:
        RS485();
        void Write(u8 data);
        void Read(u8 *buf);
        void Close();
};