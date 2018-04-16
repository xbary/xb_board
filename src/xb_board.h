#ifndef XB_BOARD_H
#define XB_BOARD_H

#ifndef FSS
#ifdef ESP8266
#include <WString.h>
#define FSS(str) (String(F(str)).c_str())
#endif

#ifdef ARDUINO_ARCH_STM32F1
#include <stdint.h>
#include <WString.h>
#define FSS(str) ((const char *)(str))
#endif


#endif

#include <xb_board_message.h>

#define BOARD_CRITICALFREEHEAP (1024*19)

#include <Arduino.h>

#ifdef ESP8266

extern "C" {
#include "user_interface.h"
}

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#endif


#include <xb_util.h>
#include "cbufSerial.h"



typedef struct
{
	void(*dosetup)(void);
	void(*doloop)(void);
	bool(*domessage)(TMessageBoard *);
	uint8_t Priority : 4;
	uint8_t CounterPriority : 4;
	uint8_t IDTask;
	TIDMessage LastIDMessage;

	int8_t dosetupRC;
	int8_t doloopRC;
	int8_t domessageRC;

} TTaskDef;

typedef TTaskDef * PTaskDef;

class TXB_board
{
public:
	TXB_board(uint8_t ATaskDefCount);
	~TXB_board();


	void cmdparse(String Ars);

	uint32_t LastActiveTelnetClientTick;

	void setString(char *dst, const char *src, int max_size);
	void SysTickCount_init(void);
	void DateTimeSecond_init(void);
	void handle(void);
	void Serial_WriteChar(char Achr);
	void Log(char Achr);
	void Log(const char *Atxt, bool puttime = false);
	void Log(cbufSerial *Acbufserial);
	void Log_TimeStamp();
	uint32_t TXCounter;
	uint8_t NoTxCounter;

	void LED_OKSEND_ON(void);
	void LED_OKSEND_OFF(void);
	void LED_OKSEND_TOGGLE(void);
	void LED_LIFE_ON(void);
	void LED_LIFE_OFF(void);
	void LED_LIFE_TOGGLE(void);
	void RELAY_SET(uint8_t Aindxrelay, bool Astatus);
	bool RELAY_GET(uint8_t Aindxrelay);

	void PrintTimeFromRun(cbufSerial *Astream);
	void PrintTimeFromRun(void);
	void PrintDiag(void);
	void PrintHelp(void);

	PTaskDef *TaskDef;
	uint8_t TaskDefCount;
	int DefTask(TTaskDef *Ataskdef, uint8_t Aid);
	void IterateTask(void);
	bool iteratetask_procedure;
	bool GetTaskString(TMessageBoard *Amb, TTaskDef *ATaskDef, String &APointerString);
	bool GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString);
	bool GetTaskName(TTaskDef *ATaskDef, String &APointerString);
	void SendKeyPress(char Akey);
	void SendKeyPress(char Akey, TTaskDef *Ataskdef);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey);
	void SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Ataskdef, bool Aexcludethistask = false);
	bool SendMessageToTask(TTaskDef *ATaskDef, TMessageBoard *mb, bool Arunagain=false);
	bool SendMessageToTaskByID(uint8_t Aidtask, TMessageBoard *mb, bool Arunagain = false);
	void SendMessageToAllTask(TIDMessage AidMessage, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask=NULL);
	void SendMessageToAllTask(TMessageBoard *mb, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask=NULL);


	uint32_t FreeHeapInLoop;
	uint32_t MinimumFreeHeapInLoop;
	uint32_t MaximumFreeHeapInLoop;


	uint32_t ADRESS_HEAP;
	uint32_t ADRESS_STACK;

	uint32_t GetFreeHeap();

	bool CheckCriticalFreeHeap(void);
};

extern TXB_board board;
extern TTaskDef XB_BOARD_DefTask;

#ifdef ARDUINO_ARCH_STM32F1
#define SysTickCount systick_uptime_millis
#endif

#ifdef ESP8266
extern volatile uint32_t SysTickCount;
extern void TCPClientDestroy(WiFiClient **Awificlient);
#endif
extern bool showasc;


void XB_BOARD_DoLoop(void);
void XB_BOARD_Setup(void);
bool XB_BOARD_DoMessage(TMessageBoard *Am);

#ifdef ESP8266
#include "..\xb_board_def.h"
#else
#include "xb_board_def.h"
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

#ifdef ESP8266
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

#endif /* XB_BOARD */