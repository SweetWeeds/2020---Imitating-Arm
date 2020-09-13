//==============================================================================
//	 RoboBuilder MainController Sample Program	1.0
//										2008.04.14	Robobuilder co., ltd.
// ※ 본 소스코드는 Tab Size = 4를 기준으로 편집되었습니다
//==============================================================================
#include <mega128.h>

// Standard Input/Output functions
#include <stdio.h>
#include <delay.h>

#include "Macro.h"
#include "Main.h"
#include "Comm.h"
#include "Dio.h"
#include "math.h"

// 플래그----------------------------------------------------------------------
bit 	F_PLAYING;				// 모션 실행중 표시
bit 	F_DIRECT_C_EN;			// wCK 직접제어 가능유무(1:가능, 0:불가)

// 버튼 입력 처리용------------------------------------------------------------
WORD    gBtnCnt;				// 버튼 눌림 처리용 카운터

// 시간 측정용-----------------------------------------------------------------
WORD    gMSEC;
BYTE    gSEC;
BYTE    gMIN;
BYTE    gHOUR;

// UART 통신용-----------------------------------------------------------------
char	gTx0Buf[TX0_BUF_SIZE];		// UART0 송신 버퍼
BYTE	gTx0Cnt;					// UART0 송신 대기 바이트 수
BYTE	gRx0Cnt;					// UART0 수신 바이트 수
BYTE	gTx0BufIdx;					// UART0 송신 버퍼 인덱스
char	gRx0Buf[RX0_BUF_SIZE];		// UART0 수신 버퍼
BYTE	gOldRx1Byte;				// UART1 최근 수신 바이트
char	gRx1Buf[RX1_BUF_SIZE];		// UART1 수신 버퍼
BYTE	gRx1Index;					// UART1 수신 버퍼용 인덱스
WORD	gRx1Step;					// UART1 수신 패킷 단계 구분
WORD	gRx1_DStep;					// 직접제어 모드에서 UART1 수신 패킷 단계 구분
WORD	gFieldIdx;					// 필드의 바이트 인덱스
WORD	gFileByteIndex;				// 파일의 바이트 인덱스
BYTE	gFileCheckSum;				// 파일내용 CheckSum
BYTE	gRxData;					// 수신데이터 저장

// 모션 제어용-----------------------------------------------------------------
int		gFrameIdx=0;	    // 모션테이블의 프레임 인덱스
WORD	TxInterval=0;		// 프레임 송신 간격
float	gUnitD[MAX_wCK];	// 단위 증가 변위
BYTE flash	*gpT_Table;		// 모션 토크모드 테이블 포인터
BYTE flash	*gpE_Table;		// 모션 확장포트값 테이블 포인터
BYTE flash	*gpPg_Table;	// 모션 Runtime P이득 테이블 포인터
BYTE flash	*gpDg_Table;	// 모션 Runtime D이득 테이블 포인터
BYTE flash	*gpIg_Table;	// 모션 Runtime I이득 테이블 포인터
WORD flash	*gpFN_Table;	// 씬 프레임 수 테이블 포인터
WORD flash	*gpRT_Table;	// 씬 실행시간 테이블 포인터
BYTE flash	*gpPos_Table;	// 모션 위치값 테이블 포인터

// 액션 파일의 구성 체계
//      - 크기 : wCK < Frame < Scene < Motion < Action
//      - 여러개의 wCK가 모여 Frame을 이루고
//        여러개의 Frame 이 모여 Scene을 이루며
//        여러개의 Scene 이 모여 Motion을 이루고
//        여러개의 Motion 이 모여 Action을 이룬다

struct TwCK_in_Motion{  // 한 개 모션에서 사용하는 wCK 정보
	BYTE	Exist;			// wCK 유무
	BYTE	RPgain;			// Runtime P이득
	BYTE	RDgain;			// Runtime D이득
	BYTE	RIgain;			// Runtime I이득
	BYTE	PortEn;			// 확장포트 사용유무(0:사용안함, 1:사용함)
	BYTE	InitPos;		// 모션파일을 만들 때 사용된 로봇의 영점 위치정보
};

struct TwCK_in_Scene{	// 한 개 씬에서 사용하는 wCK 정보
	BYTE	Exist;			// wCK 유무
	BYTE	SPos;			// 첫 프레임의 wCK 위치
	BYTE	DPos;			// 끝 프레임의 wCK 위치
	BYTE	Torq;			// 토크
	BYTE	ExPortD;		// 확장포트 출력 데이터(1~3)
};

struct TMotion{			// 한 개 모션에서 사용하는 정보들
	BYTE	PF;				// 모션에 맞는 플랫폼
	BYTE	RIdx;			// 모션의 상대 인덱스
	DWORD	AIdx;			// 모션의 절대 인덱스
	WORD	NumOfScene;		// 씬 수
	WORD	NumOfwCK;		// wCK 수
	struct	TwCK_in_Motion  wCK[MAX_wCK];	// wCK 파라미터
	WORD	FileSize;		// 파일 크기
}Motion;

struct TScene{			// 한 개 씬에서 사용하는 정보들
	WORD	Idx;			// 씬 인덱스(0~65535)
	WORD	NumOfFrame;		// 프레임 수
	WORD	RTime;			// 씬 수행 시간[msec]
	struct	TwCK_in_Scene   wCK[MAX_wCK];	// wCK 데이터
}Scene;

WORD	gSIdx;			// 씬 인덱스(0~65535)

//------------------------------------------------------------------------------
// UART0 송신 인터럽트(패킷 송신용)
//------------------------------------------------------------------------------
interrupt [USART0_TXC] void usart0_tx_isr(void) {
	if(gTx0BufIdx<gTx0Cnt){			// 보낼 데이터가 남아있으면
    	while(!(UCSR0A&(1<<UDRE))); 	// 이전 바이트 전송이 완료될때까지 대기
		UDR0=gTx0Buf[gTx0BufIdx];		// 1바이트 송신
    	gTx0BufIdx++;      				// 버퍼 인덱스 증가
	}
	else if(gTx0BufIdx==gTx0Cnt){	// 송신 완료
		gTx0BufIdx = 0;					// 버퍼 인덱스 초기화
		gTx0Cnt = 0;					// 송신 대기 바이트수 초기화
	}
}


//------------------------------------------------------------------------------
// UART0 수신 인터럽트(wCK, 사운드모듈에서 받은 신호)
//------------------------------------------------------------------------------
interrupt [USART0_RXC] void usart0_rx_isr(void)
{
	int		i;
    char	data;
	data=UDR0;
	gRx0Cnt++;
    // 수신데이터를 FIFO에 저장
   	for(i=1; i<RX0_BUF_SIZE; i++) gRx0Buf[i-1] = gRx0Buf[i];
   	gRx0Buf[RX0_BUF_SIZE-1] = data;
}


//------------------------------------------------------------------------------
// 타이머0 오버플로 인터럽트 (시간 측정용 0.998ms 간격)
//------------------------------------------------------------------------------
interrupt [TIM0_OVF] void timer0_ovf_isr(void) {
	TCNT0 = 25;
	// 1ms 마다 실행
    if(++gMSEC>999){
		// 1s 마다 실행
        gMSEC=0;
        if(++gSEC>59){
			// 1m 마다 실행
            gSEC=0;
            if(++gMIN>59){
				// 1h 마다 실행
                gMIN=0;
                if(++gHOUR>23)
                    gHOUR=0;
            }
        }
    }
}


//------------------------------------------------------------------------------
// 타이머1 오버플로 인터럽트 (프레임 송신)
//------------------------------------------------------------------------------
interrupt [TIM1_OVF] void timer1_ovf_isr(void) {
	if( gFrameIdx == Scene.NumOfFrame ) {   // 마지막 프레임이었으면
   	    gFrameIdx = 0;
    	RUN_LED1_OFF;
		F_PLAYING=0;		// 모션 실행중 표시해제
		TIMSK &= 0xfb;  	// Timer1 Overflow Interrupt 해제
		TCCR1B=0x00;
		return;
	}
	TCNT1=TxInterval;
	TIFR |= 0x04;		// 타이머 인터럽트 플래그 초기화
	TIMSK |= 0x04;		// Timer1 Overflow Interrupt 활성화(140쪽)
	MakeFrame();		// 한 프레임 준비
	SendFrame();		// 한 프레임 송신
}


//------------------------------------------------------------------------------
// 하드웨어 초기화
//------------------------------------------------------------------------------
void HW_init(void) {
	// Input/Output Ports initialization
	// Port A initialization
	// Func7=Out Func6=Out Func5=Out Func4=Out Func3=Out Func2=Out Func1=In Func0=In 
	// State7=0 State6=0 State5=0 State4=0 State3=0 State2=0 State1=P State0=P 
	PORTA=0x03;
	DDRA=0xFC;

	// Port B initialization
	// Func7=In Func6=Out Func5=Out Func4=Out Func3=In Func2=Out Func1=In Func0=In 
	// State7=T State6=0 State5=0 State4=0 State3=T State2=0 State1=T State0=T 
	PORTB=0x00;
	DDRB=0x74;

	// Port C initialization
	// Func7=Out Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=0 State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTC=0x00;
	DDRC=0x80;

	// Port D initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=P State0=P 
	PORTD=0x83;
	DDRD=0x80;

	// Port E initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=In Func1=In Func0=In 
	// State7=T State6=P State5=P State4=P State3=0 State2=T State1=T State0=T 
	PORTE=0x70;
	DDRE=0x08;

	// Port F initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTF=0x00;
	DDRF=0x00;

	// Port G initialization
	// Func4=In Func3=In Func2=Out Func1=In Func0=In 
	// State4=T State3=T State2=0 State1=T State0=T 
	PORTG=0x00;
	DDRG=0x04;

	// 타이머 0---------------------------------------------------------------
	// : 범용 시간 측정용으로 사용(ms 단위)
	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: 230.400 kHz
	// Clock 증가 주기 = 1/230400 = 4.34us
	// Overflow 시간 = 255*1/230400 = 1.107ms
	// 1ms 주기 overflow를 위한 카운트 시작값 =  255-230 = 25
	// Mode: Normal top=FFh
	// OC0 output: Disconnected
	ASSR=0x00;
	TCCR0=0x04;
	TCNT0=0x00;
	OCR0=0x00;

	// 타이머 1---------------------------------------------------------------
	// : 모션 플레이시 프레임 송신 간격 조절용으로 사용
	// Timer/Counter 1 initialization
	// Clock source: System Clock
	// Clock value: 14.400 kHz
	// Clock 증가 주기 = 1/14400 = 69.4us
	// Mode: Normal top=FFFFh
	// OC1A output: Discon.
	// OC1B output: Discon.
	// OC1C output: Discon.
	// Noise Canceler: Off
	// Input Capture on Falling Edge
	// Timer 1 Overflow Interrupt: On
	// Input Capture Interrupt: Off
	// Compare A Match Interrupt: Off
	// Compare B Match Interrupt: Off
	// Compare C Match Interrupt: Off
	TCCR1A=0x00;
	TCCR1B=0x05;
	TCNT1H=0x00;
	TCNT1L=0x00;
	ICR1H=0x00;
	ICR1L=0x00;
	OCR1AH=0x00;
	OCR1AL=0x00;
	OCR1BH=0x00;
	OCR1BL=0x00;
	OCR1CH=0x00;
	OCR1CL=0x00;

	// 타이머 2---------------------------------------------------------------
	// Timer/Counter 2 initialization
	// Clock source: System Clock
	// Clock value: 14.400 kHz
	// Clock 증가 주기 = 1/14400 = 69.4us
	// Mode: Normal top=FFh
	// OC2 output: Disconnected
	TCCR2=0x05;
	TCNT2=0x00;
	OCR2=0x00;

	// 타이머 3---------------------------------------------------------------
	// : 가속도 센서 신호 분석용으로 사용
	TCCR3A=0x00;
	TCCR3B=0x03;
	TCNT3H=0x00;
	TCNT3L=0x00;
	ICR3H=0x00;
	ICR3L=0x00;
	OCR3AH=0x00;
	OCR3AL=0x00;
	OCR3BH=0x00;
	OCR3BL=0x00;
	OCR3CH=0x00;
	OCR3CL=0x00;

	// External Interrupt(s) initialization
	// INT0: Off
	// INT1: Off
	// INT2: Off
	// INT3: Off
	// INT4: Off
	// INT5: Off
	// INT6: Off
	// INT7: Off
	EICRA=0x00;
	EICRB=0x00;
	EIMSK=0x00;

	// Timer(s)/Counter(s) Interrupt(s) initialization
	TIMSK=0x00;
	ETIMSK=0x00;

	// USART0 initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART0 Receiver: Off
	// USART0 Transmitter: On
	// USART0 Mode: Asynchronous
	// USART0 Baud rate: 115200
	UCSR0A=0x00;
	UCSR0B=0x48;
	UCSR0C=0x06;
	UBRR0H=0x00;
	UBRR0L=0x07;

	// USART1 initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART1 Receiver: On
	// USART1 Transmitter: On
	// USART1 Mode: Asynchronous
	// USART1 Baud rate: 115200
	UCSR1A=0x00;
	UCSR1B=0x18;		// 수신인터럽트 사용안함
	UCSR1C=0x06;
	UBRR1H=0x00;
	UBRR1L=BR115200;	// UART1 의 BAUD RATE를 115200로 설정

	TWCR = 0;
}


//------------------------------------------------------------------------------
// 플래그 초기화
//------------------------------------------------------------------------------
void SW_init(void) {
	PF1_LED1_OFF;
	PF1_LED2_OFF;
	PF2_LED_OFF;
	PWR_LED1_OFF;
	PWR_LED2_OFF;
	RUN_LED1_OFF;
	RUN_LED2_OFF;
	ERR_LED_OFF;
	F_PLAYING = 0;          // 동작중 아님

	gTx0Cnt = 0;			// UART0 송신 대기 바이트 수
	gTx0BufIdx = 0;			// TX0 버퍼 인덱스 초기화
	PSD_OFF;                // PSD 거리센서 전원 OFF
	gMSEC=0;
	gSEC=0;
	gMIN=0;
	gHOUR=0;
}


void SpecialMode(void)
{
	int i;

	// PF2 버튼이 눌러졌으면 wCK 직접제어 모드로 진입
	if(PINA.0==1 && PINA.1==0){
		delay_ms(10);
		if(PINA.0==1){
		    TIMSK &= 0xFE;     // Timer0 Overflow Interrupt 미사용
			EIMSK &= 0xBF;		// EXT6(리모컨 수신) 인터럽트 미사용
			UCSR0B |= 0x80;   	// UART0 Rx인터럽트 사용
			UCSR0B &= 0xBF;   	// UART0 Tx인터럽트 미사용
		}
	}
}

//------------------------------------------------------------------------------
//  Main Function �
//------------------------------------------------------------------------------
void main(void) {
	WORD    i, lMSEC;

	HW_init();			// Initialize Hardware�
	SW_init();			// Initialize Variables�

	#asm("sei");
	TIMSK |= 0x01;		// Timer0 Overflow Interrupt activate

	SpecialMode();
    
	SendSetCmd(1, ID_SET, 10, 10);
	
	while(1){
		/*
		lMSEC = gMSEC;
		ReadButton();	    //  Read Button �
		IoUpdate();		    // IO Status UPDATE
		while(lMSEC==gMSEC);
		*/
		PositionMove(1,1,200);
		IOwrite(1,0);
		PWR_LED2_ON;    delay_ms(500);
		PWR_LED2_OFF;    delay_ms(500);
		
		PositionMove(1,1,50);
		IOwrite(1,3);
		PWR_LED1_ON;    delay_ms(500);
		PWR_LED1_OFF;    delay_ms(500);
    }
}
