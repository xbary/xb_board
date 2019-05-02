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

#ifndef __XB_BOARD_H
#define __XB_BOARD_H

#include <Arduino.h>


#ifndef FSS
#ifdef ESP8266
#include <WString.h>
#define FSS(str) (String(F(str)).c_str())
#endif

#ifdef ESP32
#include <WString.h>
#define FSS(str) (String(F(str)).c_str())
#endif

#ifdef __riscv64
#include <WString.h>
#define FSS(str) (String(F(str)).c_str())
#endif

#ifdef ARDUINO_ARCH_STM32F1
#include <stdint.h>
#include <WString.h>
#define FSS(str) ((const char *)(str))
#endif

#endif

#ifdef __riscv64
#include <utils/xb_util.h>
#include <utils/cbufSerial.h>
#endif

#ifdef ARDUINO_ARCH_STM32
#include <utils/xb_util.h>
#include <utils/cbufSerial.h>
#endif

#ifdef ESP32
#include <utils/xb_util.h>
#include <utils/cbufSerial.h>
#endif

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#include <Ticker.h>
#include <utils/xb_util.h>
#include <utils/cbufSerial.h>
#endif

typedef enum { ftData, ftResponseOK, ftResponseError, ftResponseCRCError, ftBufferIsFull, ftOKWaitForNext, ftUnrecognizedType, ftThereIsNoSuchTask } TFrameType;

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

	uint8_t CountGetStreamAddressAsKeyboard;
	uint8_t CountPutStreamAddressAsLog;
	uint8_t CountPutStreamAddressAsGui;
	uint32_t *GetStreamAddressAsKeyboard;
	uint32_t *PutStreamAddressAsLog;
	uint32_t *PutStreamAddressAsGui;


	uint8_t CounterPriority;
	TUniqueID DeviceID;
	bool ShowLogInfo;
	bool ShowLogWarn;
	bool ShowLogError;
	TIDMessage LastIDMessage;
	THandleDataFrameTransport *HandleDataFrameTransportList;
	int8_t dosetupRC;
	int8_t doloopRC;
	int8_t domessageRC;
	int32_t dointerruptRC;
	uint32_t TickReturn;
	uint32_t TickWaitLoop;
};

#pragma pack(push, 1)
struct TFrameTransport
{
	uint8_t size;
	uint8_t crc8;
	uint32_t FrameID;
	uint32_t SourceAddress;
	TUniqueID SourceDeviceID;
	char SourceTaskName[16];
	uint32_t DestAddress;
	TUniqueID DestDeviceID;
	char DestTaskName[16];
	TFrameType FrameType;
	uint8_t LengthFrame;
	uint8_t Frame[255 - (4+1+1+4+4+8+16+4+8+16+1+1)];
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

#ifndef TIMEOUT_HANDLEDATAFRAMETRANSPORT
#define TIMEOUT_HANDLEDATAFRAMETRANSPORT 10000
#endif

struct THandleDataFrameTransport
{
	THandleDataFrameTransport *Next;
	THandleDataFrameTransport *Prev;
	
	uint32_t TickCreate;
	
	uint32_t FromAddress;
	
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
};

#ifndef WiringPinMode
typedef uint8_t WiringPinMode;
#endif

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
	//-----------------------------------------------------------------------------------------------------------------
private:
	bool iteratetask_procedure;
	bool setup_procedure;
#ifdef ARDUINO_ARCH_STM32
	uint32_t ADRESS_HEAP;
	uint32_t ADRESS_STACK;
#endif

	TUniqueID GetUniqueID();
public:
	//-----------------------------------------------------------------------------------------------------------------
	String DeviceName;
	String DeviceVersion;
	TUniqueID DeviceID;
	uint32_t Tick_ESCKey;
	uint8_t TerminalFunction;
	TXB_board();
	~TXB_board();
		
#if defined(ESP8266) 
	void SysTickCount_init(void);
	void DateTimeSecond_init(void);
#endif
	//-----------------------------------------------------------------------------------------------------------------
	uint8_t crc8(const uint8_t *addr, uint8_t len);
	void FilterString(const char *Asourcestring, String &Adestinationstring);	
	String DeviceIDtoString(TUniqueID Adevid);
    //-----------------------------------------------------------------------------------------------------------------
	TPinInfo *PinInfoTable;	
#ifdef BOARD_LED_TX_PIN
	uint32_t Tick_TX_BLINK;
#endif
#ifdef BOARD_LED_RX_PIN
	uint32_t Tick_RX_BLINK;
#endif
#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
	uint32_t TickEnableBlink;
#endif

	bool SetPinInfo(uint16_t Anumpin, uint8_t Afunction, uint8_t Amode, bool Alogwarn = false);
	bool pinMode(uint16_t pin, WiringPinMode mode);
	void digitalWrite(uint16_t pin, uint8_t value);
	uint8_t digitalRead(uint16_t pin);
	uint8_t digitalToggle(uint16_t pin);
	void Blink_RX(int8_t Auserid = -1);
	void Blink_TX(int8_t Auserid = -1);
	void Blink_Life();
	//-----------------------------------------------------------------------------------------------------------------
	
	int8_t doAllInterruptRC;
	TTask *TaskList;
	uint8_t TaskCount;
	TTask *CurrentTask;
	TTask *CurrentIterateTask;
	
	void handle(void);	
	TTask *AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID = 0);
	bool DelTask(TTaskDef *Ataskdef);
	void ResetInAllTaskDefaultStream();		
	TTask *GetTaskByIndex(uint8_t Aindex);
	TTaskDef *GetTaskDefByName(String ATaskName);
	void IterateTask(void);
	void TriggerInterrupt(TTaskDef *Ataskdef);
	void DoInterrupt(TTaskDef *Ataskdef);
	//-----------------------------------------------------------------------------------------------------------------	
	
	bool DoMessage(TMessageBoard *mb, bool Arunagain, TTask *Afromtask, TTaskDef *Atotaskdef);
	bool DoMessageOnAllTask(TMessageBoard *mb, bool Arunagain, TDoMessageDirection ADoMessageDirection, TTask *Afromtask = NULL, TTaskDef *Aexcludetask = NULL);
	bool DoMessageByTaskName(String Ataskname, TMessageBoard *mb, bool Arunagain);
	bool SendMessage_GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString);
	bool SendMessage_GetTaskNameString(TTaskDef *ATaskDef, String &APointerString);
	void SendMessage_OTAUpdateStarted();
	void SendMessage_FunctionKeyPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Aexcludetask=NULL);
	void SendMessage_KeyPress(char Akey, TTaskDef *Aexcludetask = NULL);
	void SendMessage_ConfigSave(void);
	void SendMessage_FreePTR(void *Aptr);
	
	//-----------------------------------------------------------------------------------------------------------------
#ifdef PSRAM_BUG
	void *psram2m;
	uint32_t GETFREEPSRAM_ERROR_COUNTER;
	uint32_t lastfreepsram;
	uint32_t MaximumMallocPSRAM;
#endif
	uint32_t FreePSRAMInLoop;
	uint32_t MinimumFreePSRAMInLoop;
	uint32_t MaximumFreePSRAMInLoop;
	uint32_t FreeHeapInLoop;
	uint32_t MinimumFreeHeapInLoop;
	uint32_t MaximumFreeHeapInLoop;
	int OurReservedBlock;
	
	uint32_t getFreePSRAM();
	uint32_t getFreeHeap();
	void *_malloc_psram(size_t size);
	void *_malloc(size_t size);
	void free(void *Aptr);
	void freeandnull(void **Aptr);
	//-----------------------------------------------------------------------------------------------------------------
	
	bool HandleFrameTransportInGetStream;

	uint32_t GetStream(void *Adata, uint32_t Amaxlength, TTaskDef *AStreamtaskdef, uint32_t Afromaddress = 0);
	uint32_t PutStream(void *Adata, uint32_t Alength, TTaskDef *AStreamtaskdef, uint32_t AToAddress = 0);
	void BeginUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress);
	void EndUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress);
	bool HandleDataFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport, TTaskDef *ATaskDefStream);
	bool GetFromErrFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport);
	THandleDataFrameTransport *AddToTask_HandleDataFrameTransport(TTaskDef *AStreamtaskdef, uint32_t Afromaddress);
	void CheckOld_HandleDataFrameTransport(TTask *ATask = NULL);
	bool SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, String AOnStreamTaskName, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	bool SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, TTaskDef *ATaskDefStream, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	void SendResponseFrameOnProt(uint32_t AFrameID, TTaskDef *ATaskDefStream, uint32_t Afromaddress, uint32_t Atoaddress, TFrameType AframeType, TUniqueID ADeviceID);
	void HandleFrame(TFrameTransport *Aft, TTaskDef *ATaskDefStream);
	void HandleFrameLocal(TFrameTransport *Aft);
	void AddStreamAddressAsKeyboard(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	void SubStreamAddressAsKeyboard(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	void AddStreamAddressAsLog(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	void SubStreamAddressAsLog(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	void AddStreamAddressAsGui(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	void SubStreamAddressAsGui(TTaskDef *AStreamDefTask, uint32_t Aaddress);
	//-----------------------------------------------------------------------------------------------------------------
	
	bool Default_ShowLogInfo;
	bool Default_ShowLogWarn;
	bool Default_ShowLogError;
	uint32_t TXCounter;
	uint8_t NoTxCounter;

	void AllPutStreamGui(void *Adata, uint32_t Alength);
	void AllPutStreamLog(void *Adata, uint32_t Alength);
	int print(String Atext);
	void Log(char Achr, TTypeLog Atl = tlInfo);
	void Log(const char *Atxt, bool puttime = false, bool showtaskname = false, TTypeLog Atl = tlInfo);
	void Log(cbufSerial *Acbufserial, TTypeLog Atl = tlInfo);
	void Log_TimeStamp();
	void PrintTimeFromRun(cbufSerial *Astream);
	void PrintTimeFromRun(void);
	//-----------------------------------------------------------------------------------------------------------------
#ifdef XB_PREFERENCES
	bool PREFERENCES_BeginSection(String ASectionname);
	void PREFERENCES_EndSection();
	size_t PREFERENCES_PutArrayBytes(const char* key, const void* array, size_t sizearray);
	size_t PREFERENCES_GetArrayBytes(const char* key, void* array, size_t maxsizearray);
	size_t PREFERENCES_PutBool(const char* key, const bool value);
	bool PREFERENCES_GetBool(const char* key, const bool defaultvalue);
	size_t PREFERENCES_GetString(const char* key, char* value, const size_t maxlen);
	String PREFERENCES_GetString(const char* key, String defaultvalue);
	uint32_t PREFERENCES_GetUINT32(const char* key, uint32_t defaultvalue);
	uint8_t PREFERENCES_GetUINT8(const char* key, uint8_t defaultvalue);
	size_t PREFERENCES_PutString(const char* key, const char* value);
	size_t PREFERENCES_PutString(const char* key, String value);
	size_t PREFERENCES_PutUINT32(const char* key, uint32_t value);
	size_t PREFERENCES_PutUINT8(const char* key, uint8_t value);
#endif
	//-----------------------------------------------------------------------------------------------------------------

};

extern TXB_board board;
extern TTaskDef XB_BOARD_DefTask;
extern volatile uint32_t DateTimeUnix;
extern volatile uint32_t DateTimeStart;

#ifdef ARDUINO_ARCH_STM32
#define SysTickCount (uint32_t)(millis())
#endif

#ifdef ESP8266
extern volatile uint32_t __SysTickCount;
#define SysTickCount (__SysTickCount)
extern void TCPClientDestroy(WiFiClient **Awificlient);
#endif

#ifdef ESP32
#define SysTickCount (uint32_t)(millis())
#endif

#ifdef __riscv64
#define SysTickCount (uint32_t)(millis())
#endif


#define XB_BOARD_SETUP() board.AddTask(&XB_BOARD_DefTask)
#define XB_BOARD_LOOP() board.IterateTask()

#ifndef BOARD_CRITICALFREEHEAP
#define BOARD_CRITICALFREEHEAP (1024*19)
#endif

#ifndef NUM_DIGITAL_PINS
#define NUM_DIGITAL_PINS 61
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

#endif
