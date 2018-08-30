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
#define BOARD_NR_GPIO_PINS NUM_DIGITAL_PINS       

#include <utils/xb_util.h>
#include <utils/cbufSerial.h>

#endif

#ifdef ESP8266

typedef uint8_t WiringPinMode;
#define BOARD_NR_GPIO_PINS NUM_DIGITAL_PINS       
extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#endif


typedef enum { ftData, ftResponseOK, ftResponseError, ftResponseCRCError } TFrameType;
typedef enum { sfpLocal, sfpSerial, sfpSerialBT, sfpSerial1, sfpSerial2 } TSendFrameProt;

#include <utils\xb_board_message.h>

typedef struct
{
	void(*dosetup)(void);
	uint32_t(*doloop)(void);
	bool(*domessage)(TMessageBoard *);
	void(*dointerrupt)(void);
	uint8_t Priority;
	uint8_t CounterPriority;
	uint8_t IDTask;
	TIDMessage LastIDMessage;

	int8_t dosetupRC;
	int8_t doloopRC;
	int8_t domessageRC;
	int32_t dointerruptRC;
	uint32_t TickReturn;
	uint32_t TickWaitLoop;
} TTaskDef;

typedef TTaskDef * PTaskDef;

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

#ifdef ESP8266
#include "xb_board_def.h"
#elif defined(ESP32)
#include "xb_board_def.h"
#else
#include "xb_board_def.h"
#endif


#pragma pack(push, 1)
typedef struct
{
	uint8_t size;
	uint8_t crc8;
	uint32_t FrameID;
	TUniqueID DeviceID;
	char TaskName[16];
	TFrameType FrameType;
	uint8_t LengthFrame;
	uint8_t Frame[255 - (1 + 16 + 8 + 4 + 1 + 1+1)];
} TFrameTransport;
#pragma pack(pop)

#define FRAME_ACK_A 0xf1
#define FRAME_ACK_B 0x2f
#define FRAME_ACK_C 0xf3
#define FRAME_ACK_D 0x4f

#pragma pack(push, 1)
typedef struct
{
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
} TFrameTransportACK;
#pragma pack(pop)


class TXB_board
{
public:
	TXB_board(uint8_t ATaskDefCount);
	~TXB_board();

	uint32_t LastActiveTelnetClientTick;

	void setString(char *dst, const char *src, int max_size);
	void SysTickCount_init(void);
	void DateTimeSecond_init(void);
	void HandleKeyPress(char ch);
	void HandleFrame(TFrameTransport *Aft, TSendFrameProt Asfp);
	void HandleSerial(bool ADoKeyPress);
	void HandleTransportFrame(bool ADoKeyPress, TSendFrameProt Asfp, uint16_t Ach=0xffff);
	void handle(void);
	void Serial_WriteChar(char Achr);
	void Log(char Achr);
	void Log(const char *Atxt, bool puttime = false, bool showtaskname = false, TTaskDef *Ataskdef=NULL);
	void Log(cbufSerial *Acbufserial);
	void Log_TimeStamp();
	uint32_t TXCounter;
	uint8_t NoTxCounter;

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
	void PrintDiag(void);

	PTaskDef *TaskDef;
	uint8_t TaskDefCount;
	TTaskDef *CurrentTaskDef;
	int DefTask(TTaskDef *Ataskdef, uint8_t Aid);
	void IterateTask(void);
	void DoInterrupt(TTaskDef *Ataskdef);

	bool GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString);
	bool GetTaskName(TTaskDef *ATaskDef, String &APointerString);
	void SendMessageOTAUpdateStarted();
	void SendKeyPress(char Akey);
	void SendKeyPress(char Akey, TTaskDef *Ataskdef);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Ataskdef, bool Aexcludethistask = false);
	bool SendMessageToTask(TTaskDef *ATaskDef, TMessageBoard *mb, bool Arunagain=false);
	bool SendMessageToTaskByID(uint8_t Aidtask, TMessageBoard *mb, bool Arunagain = false);
	bool SendMessageToTaskByName(String Ataskname, TMessageBoard *mb, bool Arunagain = false);
	bool SendMessageToAllTask(TIDMessage AidMessage, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask=NULL);
	bool SendMessageToAllTask(TMessageBoard *mb, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask=NULL);
	void SendResponseFrameOnProt(uint32_t AFrameID, TSendFrameProt ASendFrameProt, TFrameType AframeType, TUniqueID ADeviceID);
	uint32_t SendFrameToDeviceTask(String Ataskname, TSendFrameProt ASendFrameProt, void *ADataFrame, uint32_t Alength);

#if defined(ESP32)
	uint32_t FreePSRAMInLoop;
	uint32_t MinimumFreePSRAMInLoop;
	uint32_t MaximumFreePSRAMInLoop;

	uint32_t getFreePSRAM();

	void *malloc_psram(size_t size);
#endif
	void free(void *Aptr);
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
extern volatile uint32_t SysTickCount;
extern void TCPClientDestroy(WiFiClient **Awificlient);
#endif

#ifdef ESP32
#define SysTickCount ((uint32_t)millis())
#endif

extern bool showasc;

uint32_t XB_BOARD_DoLoop(void);
void XB_BOARD_Setup(void);
bool XB_BOARD_DoMessage(TMessageBoard *Am);

#define XB_BOARD_SETUP(Aid) board.DefTask(&XB_BOARD_DefTask, Aid)
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

#ifdef Serial1Board

#ifndef Serial1Board
#define Serial1Board Serial1
#endif

#ifndef Serial1Board_BAUD
#define Serial1Board_BAUD 230400
#endif

#ifndef Serial1_print
#define  Serial1_print Serial1Board.print
#endif

#ifndef Serial1_println
#define Serial1_println Serial1Board.println
#endif

#ifndef Serial1_printf
#define Serial1_printf Serial1Board.printf
#endif

#if defined(ESP8266) || defined(ESP32)
#ifndef Serial1_setDebugOutput
#define Serial1_setDebugOutput Serial1Board.setDebugOutput
#endif
#endif

#ifndef Serial1_begin
#define Serial1_begin Serial1Board.begin
#endif

#ifndef Serial1_availableForWrite
#define Serial1_availableForWrite Serial1Board.availableForWrite
#endif

#ifndef Serial1_available
#define Serial1_available Serial1Board.available
#endif

#ifndef Serial1_write
#define Serial1_write Serial1Board.write
#endif

#ifndef Serial1_read
#define Serial1_read Serial1Board.read
#endif

#ifndef Serial1_flush
#define Serial1_flush Serial1Board.flush
#endif

#ifndef Serial1_EmptyTXBufferSize
#ifdef ESP32
#define Serial1_EmptyTXBufferSize 127
#endif
#endif

#endif

#endif /* XB_BOARD */