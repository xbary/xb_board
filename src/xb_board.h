// Szablon projektu .INO z oparty na bibliotece xb_board

/*
 #include <xb_board.h>
 
 void setup()
{
	XB_BOARD_SETUP();
}

void loop()
{
	XB_BOARD_LOOP();
}
 */

// we w³aœciwoœciach projektu dodajemy w 
// Extra flags: -I{build.path}


// Obowi¹zkowa inkluda z parametrami wstêpnymi projektu
/*
#ifndef XB_BOARD_DEF_H
#define XB_BOARD_DEF_H

#define DEVICE_NAME "..."
#define DEVICE_VERSION "?.? (2019.01.1)"

// Standardowe ustawienia dla UART
#define SerialBoard			Serial
#define SerialBoard_BAUD	115200
// Wskazanie GPIO na których ma zostaæ uruchomiony podstawowy UART
//#define SerialBoard_RX_PIN  ?
//#define SerialBoard_TX_PIN  ?

//#define BOARD_LED_TX_PIN ?
//#define BOARD_LED_RX_PIN ?
//#define BOARD_LED_TX_STATUS_OFF LOW
//#define BOARD_LED_RX_STATUS_OFF LOW
//#define TICK_LED_BLINK 250

// Definicje powoduj¹ce wstawienie standardowego GUI z GADGETAMI na terminalach VT100(?)
//#define SCREENTEXT_TYPE_BOARDLOG
//#define XB_GUI

// GPIO do którego pod³¹czony jest np LED informuj¹cy u¿ytkownika czy urz¹dzenie siê nie zawiesi³o
#define BOARD_LED_LIFE_PIN 5



#endif 
 */

#ifndef XB_BOARD_H
#define XB_BOARD_H

#include <Arduino.h>

#ifndef FSS
#ifdef ESP8266
#include <WString.h>
#define FSS(str) (String(F(str)).c_str())
#endif

#if defined(ESP32)

#include <WString.h>

#define FSS(str) (String(F(str)).c_str())
#endif
#ifdef ARDUINO_ARCH_STM32F1
#include <stdint.h>
#include <WString.h>
#define FSS(str) ((const char *)(str))
#endif
#endif

#define _REG register

#if defined(ESP32)

typedef uint8_t WiringPinMode;

#include <utils/xb_util.h>
#include <utils/cbufSerial.h>

#endif

#ifdef ESP8266

typedef uint8_t WiringPinMode;
extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#include <utils/xb_util.h>
#include <utils/cbufSerial.h>

#endif


typedef enum { ftData, ftResponseOK, ftResponseError, ftResponseCRCError, ftBufferIsFull, ftOKWaitForNext, ftUnrecognizedType, ftThereIsNoSuchTask } TFrameType;
typedef enum { sfpLocal, sfpSerial, sfpSerialBT, sfpSerial1, sfpSerial2, sfpSerialTelnet} TSendFrameProt;

typedef union
{
	uint64_t ID64;
	uint8_t ID[8];
	uint8_t MAC[6];
} TUniqueInt64;

typedef struct
{
	TUniqueInt64 ID;
} TUniqueID;

struct TTask;
struct THandleDataFrameTransport;

#include "xb_board_def.h"
#include <utils\xb_board_message.h>

struct TTaskDef
{
	uint8_t Priority;
	void(*dosetup)(void);
	uint32_t(*doloop)(void);
	bool(*domessage)(TMessageBoard *);
	void(*dointerrupt)(void);
	TTask *Task;
};



struct TTask
{
	TTask *Next;
	TTask *Prev;
	TTaskDef *TaskDef;
	TTaskDef *StreamTaskDef;
	TTaskDef *SecStreamTaskDef;
	uint8_t CounterPriority;
	TUniqueID DeviceID;
	bool ShowLogInfo;
	bool ShowLogWarn;
	bool ShowLogError;
	TIDMessage LastIDMessage;
	THandleDataFrameTransport *HandleDataFrameTransport;
	int8_t dosetupRC;
	int8_t doloopRC;
	int8_t domessageRC;
	int32_t dointerruptRC;
	uint32_t TickReturn;
	uint32_t TickWaitLoop;
};

typedef TTaskDef * PTaskDef;

#include "xb_board_def.h"

#pragma pack(push, 1)
struct TFrameTransport
{
	uint8_t size;
	uint8_t crc8;
	uint32_t FrameID;
	TUniqueID DeviceID;
	char TaskName[16];
	TFrameType FrameType;
	uint8_t LengthFrame;
	uint8_t Frame[250 - (1 + 16 + 8 + 4 + 1 + 1 + 4)];
};

#define FRAME_ACK_A 0xf1
#define FRAME_ACK_B 0x2f
#define FRAME_ACK_C 0xf3
#define FRAME_ACK_D 0x4f

struct TFrameTransportACK
{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
};

struct THDFT
{
	struct TFrameTransportACK ACK;
	struct TFrameTransport FT;
};

struct THandleDataFrameTransport
{
	union
	{
		uint8_t buf[sizeof(TFrameTransportACK) + sizeof(TFrameTransport)];
		struct THDFT str;
	} Data;

	uint8_t indx_interpret;
	bool isdataframe_interpret;

	bool isdatatoread;
	uint8_t indxdatatoread;
	uint8_t max_indxdatatoread;

	uint8_t *BufDataToLong;
	uint32_t SizeBufDataToLong;
	uint32_t IndxReadBufDataToLong;
};
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
	uint8_t value : 1;
	uint8_t use : 1;
	uint8_t mode : 2;
	uint8_t functionpin : 4;
} TPinInfo;
#pragma pack(pop)

#define FUNCTIONPIN_NOIDENT 0
#define FUNCTIONPIN_GPIO 1
#define FUNCTIONPIN_ANALOGIN 2
#define FUNCTIONPIN_ANALOGOUT 3
#define FUNCTIONPIN_UARTRXTX 4

#define MODEPIN_INPUT 1
#define MODEPIN_OUTPUT 2
#define MODEPIN_ANALOG 3

typedef enum { tlInfo = 0, tlWarn, tlError } TTypeLog;

class TXB_board
{
public:
	TXB_board();
	~TXB_board();

	uint32_t LastActiveTelnetClientTick;

	void setString(char *dst, const char *src, int max_size);
	void SysTickCount_init(void);
	void DateTimeSecond_init(void);
	void HandleKeyPress(char ch);
	void HandleFrame(TFrameTransport *Aft, TTaskDef *ATaskDefStream);
	void HandleFrameLocal(TFrameTransport *Aft);
	void handle(void);
	
	
	bool HandleDataFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport, TTaskDef *ATaskDefStream);
	bool GetFromErrFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport);
	uint32_t GetStream(void *Adata, uint32_t Amaxlength, TTaskDef *AStreamtaskdef);
	uint32_t PutStream(void *Adata, uint32_t Alength, TTaskDef *AStreamtaskdef);
	bool HandleFrameTransportInGetStream;
	
	int print(String Atext);

	TTaskDef *Default_StreamTaskDef;
	TTaskDef *Default_SecStreamTaskDef;


	bool Default_ShowLogInfo;
	bool Default_ShowLogWarn;
	bool Default_ShowLogError;

	void Log(char Achr, TTypeLog Atl = tlInfo);
	void Log(const char *Atxt, bool puttime = false, bool showtaskname = false, TTypeLog Atl = tlInfo);
	void Log(cbufSerial *Acbufserial, TTypeLog Atl = tlInfo);
	void Log_TimeStamp();

	uint32_t TXCounter;
	uint8_t NoTxCounter;

	TPinInfo *PinInfoTable;	

	bool SetPinInfo(uint16_t Anumpin, uint8_t Afunction, uint8_t Amode, bool Alogwarn=false);

	bool pinMode(uint16_t pin, WiringPinMode mode);
	void digitalWrite(uint16_t pin, uint8_t value);
	uint8_t digitalRead(uint16_t pin);
	uint8_t digitalToggle(uint16_t pin);
	void Blink_RX(int8_t Auserid = -1);
	void Blink_TX(int8_t Auserid = -1);

#ifdef BOARD_LED_TX_PIN
	uint32_t Tick_TX_BLINK;
#endif
#ifdef BOARD_LED_RX_PIN
	uint32_t Tick_RX_BLINK;
#endif
#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
	uint32_t TickEnableBlink;
#endif


	void PrintTimeFromRun(cbufSerial *Astream);
	void PrintTimeFromRun(void);
	//void PrintDiag(void);

	TTask *TaskList;
	uint8_t TaskCount;

	TTask *CurrentTask;
	TTask *CurrentIterateTask;

	TTask *AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID = 0);
	bool DelTask(TTaskDef *Ataskdef);

	void ResetInAllTaskDefaultStream();		
		
	TTask *GetTaskByIndex(uint8_t Aindex);
	TTaskDef *GetTaskDefByName(String ATaskName);
	void IterateTask(void);
	int8_t doAllInterruptRC;
	void TriggerInterrupt(TTaskDef *Ataskdef);
	void DoInterrupt(TTaskDef *Ataskdef);
	
	bool GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString);
	bool GetTaskName(TTaskDef *ATaskDef, String &APointerString);
	void SendMessageOTAUpdateStarted();
	void SendKeyPress(char Akey);
	void SendKeyPress(char Akey, TTaskDef *Ataskdef);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Ataskdef, bool Aexcludethistask = false);
	bool DoMessage(TMessageBoard *mb, bool Arunagain, TTask *Afromtask, TTaskDef *Atotaskdef);
	bool SendMessageToTask(TTaskDef *ATaskDef, TMessageBoard *mb, bool Arunagain=false);
	bool SendMessageToTask(TTaskDef *ATaskDef, TIDMessage AIDmessage, bool Arunagain = false);
	bool SendMessageToTaskByName(String Ataskname, TMessageBoard *mb, bool Arunagain = false);
	bool SendMessageToAllTask(TIDMessage AidMessage, TDoMessageDirection ADoMessageDirection = doFORWARD, TTaskDef *Aexcludetask=NULL);
	bool SendMessageToAllTask(TMessageBoard *mb, TDoMessageDirection ADoMessageDirection = doFORWARD, TTaskDef *Aexcludetask=NULL);
	void SendResponseFrameOnProt(uint32_t AFrameID,  TTaskDef *ATaskDefStream, TFrameType AframeType, TUniqueID ADeviceID);
	bool SendFrameToDeviceTask(String Ataskname, String AONStreamTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	bool SendFrameToDeviceTask(String Ataskname, TTaskDef *ATaskDefStream, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	void SendSaveConfigToAllTask(void);

#if defined(ESP32)
	uint32_t FreePSRAMInLoop;
	uint32_t MinimumFreePSRAMInLoop;
	uint32_t MaximumFreePSRAMInLoop;

	uint32_t getFreePSRAM();

	void *_malloc_psram(size_t size);
	void *_malloc(size_t size);
#endif
#if defined(ESP8266)
	void *_malloc(size_t size);
#endif
	void free(void *Aptr);
	void freeandnull(void **Aptr);
	void SendMessageToAllTask_FreePTR(void *Aptr);
	uint32_t FreeHeapInLoop;
	uint32_t MinimumFreeHeapInLoop;
	uint32_t MaximumFreeHeapInLoop;

	uint32_t getFreeHeap();
	bool CheckCriticalFreeHeap(void);

	TUniqueID GetUniqueID();
	uint8_t crc8(const uint8_t *addr, uint8_t len);
private:
	bool GetTaskString(TMessageBoard *Amb, TTaskDef *ATaskDef, String &APointerString);
	bool iteratetask_procedure;
	bool setup_procedure;
#ifdef ARDUINO_ARCH_STM32F1
	uint32_t ADRESS_HEAP;
	uint32_t ADRESS_STACK;
#endif
};

extern TXB_board board;
extern TTaskDef XB_BOARD_DefTask;
extern volatile uint32_t DateTimeUnix;
extern volatile uint32_t DateTimeStart;

#ifdef ARDUINO_ARCH_STM32F1
#define SysTickCount systick_uptime_millis
#endif

#ifdef ESP8266
extern volatile uint32_t __SysTickCount;
#define SysTickCount (__SysTickCount)
extern void TCPClientDestroy(WiFiClient **Awificlient);
#endif

#ifdef ESP32
#define SysTickCount (uint32_t)(millis())
#endif

extern bool showasc;

uint32_t XB_BOARD_DoLoop(void);
void XB_BOARD_Setup(void);
bool XB_BOARD_DoMessage(TMessageBoard *Am);

#define XB_BOARD_SETUP() board.AddTask(&XB_BOARD_DefTask)
#define XB_BOARD_LOOP() board.IterateTask()

#ifndef BOARD_CRITICALFREEHEAP
#define BOARD_CRITICALFREEHEAP (1024*19)
#endif

#ifndef SerialBoard
#define SerialBoard Serial
#endif

#ifndef SerialBoard_BAUD
#define SerialBoard_BAUD 230400
#endif

#ifndef Serial_print
#define  Serial_print SerialBoard.print
#endif

#ifndef Serial_println
#define Serial_println SerialBoard.println
#endif

#ifndef Serial_printf
#define Serial_printf SerialBoard.printf
#endif

#if defined(ESP8266) || defined(ESP32)
#ifndef Serial_setDebugOutput
#define Serial_setDebugOutput SerialBoard.setDebugOutput
#endif
#endif

#ifndef Serial_begin
#define Serial_begin SerialBoard.begin
#endif

#ifndef Serial_availableForWrite
#define Serial_availableForWrite SerialBoard.availableForWrite
#endif

#ifndef Serial_available
#define Serial_available SerialBoard.available
#endif

#ifndef Serial_write
#define Serial_write SerialBoard.write
#endif

#ifndef Serial_read
#define Serial_read SerialBoard.read
#endif

#ifndef Serial_flush
#define Serial_flush SerialBoard.flush
#endif

#ifndef Serial_EmptyTXBufferSize
#ifdef ESP32
#define Serial_EmptyTXBufferSize 127
#endif
#endif

#ifndef BOARD_NR_GPIO_PINS 
#define BOARD_NR_GPIO_PINS NUM_DIGITAL_PINS       
#endif

#ifndef pin_Mode
#define pin_Mode pinMode
#endif 

#ifndef digital_Read
#define digital_Read digitalRead
#endif 

#ifndef digital_Write
#define digital_Write digitalWrite
#endif 

#endif /* XB_BOARD */