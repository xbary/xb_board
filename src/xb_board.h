/*

*** Uruchamianie projektu .INO opartego na bibliotece XB_BOARD ***

 -------------------------------------
 KROK 1. Dodajemy bibliotekê XB_BOARD.

Tak powinien wygl¹daæ g³ówny plik projektu, nale¿y dopisa dwa makra których kod odwo³uje siê do funkcji biblioteki.

#include <xb_board.h>
 
 void setup()
{
	XB_BOARD_SETUP();
}

void loop()
{
	XB_BOARD_LOOP();
}
- eof -

-----------------------------------------------------------------------------
KROK 2. Dodanie flagi dla kompilatora aby szuka³ inkludy w katalogu projektu.

We w³aœciwoœciach projektu dodajemy w pozycji Extra flags: taki wpis -I{build.path}

-----------------------------------------------------
KROK 3. Dodanie definicji we w³aœciwoœciach projektu z nazw¹ projektu.

We w³aœciwoœciach projektu dodajemy w pozycji Defines: PROJECT_NAME="{build.project_name}"

-----------------------------------------------------
KROK 4. Dodanie pliku inkludy o nazwie xb_board_def.h

#ifndef XB_BOARD_DEF_H
#define XB_BOARD_DEF_H

#endif


------------------------------------------------------------------------------------------------
KROK 5. Kompilacja i uruchomienie. Sprawdzono dzia³anie bilioteki na p³ytkach: ESP32 Dev module, 
        ESP32 wRover module.



*** MAKRA KONFIGURACJI ORAZ URUCHAMIAJ¥CE FUNKCJE BIBLIOTEKI ***


#define BOARD_DEVICE_NAME "..."
// Nazwa urzadzenia


#define BOARD_DEVICE_VERSION "?.? (2019.01.1)"
// Tekst opisuj¹cy wersje urz¹dzenia


#define BOARD_LED_LIFE_PIN 5
// GPIO (np 5 dla WEMOS LOLIN D32 PRO) do którego pod³¹czony jest np LED blikuj¹cy co sekundê.
// Równoczeœnie co sekundê jest wysy³any message IM_LIVE_BLINK do wszystkich zadañ bez wzglêdu 
// czy zdefiniowany pin BOARD_LED_LIFE_PIN.


#define BOARD_LED_TX_PIN ?
#define BOARD_LED_RX_PIN ?
// GPIO do którego pod³¹czone s¹ ledy informuj¹ce ¿e coœ jest odbierane oraz wysy³ane z urz¹dzenia.
// Mo¿na pomin¹æ lub te¿ zdefiniowaæ tylko  BOARD_LED_TX_PIN  lub  BOARD_LED_RX_PIN.
// Za wyzwolenie tej funkcjonalnoœci odpowiadaj¹ funkcje Blink_RX() oraz Blink_TX(), funkcje oprócz
// zapalaniu LEDów wczeœniej  zdefiniowanych wysy³aj¹ messaga IM_RX_BLINK, IM_TX_BLINK do wszystkich 
// zadañ. Mo¿na podaæ w argumencie ID w celu rozpoznania w messagu o które RX TX nam chodzi.

#define BOARD_LED_TX_STATUS_OFF LOW
#define BOARD_LED_RX_STATUS_OFF LOW
// Definiowanie czy wygaszenie LEDów jest przy stanie LOW czy HIGH pinów wczeœniej zdefiniowanych.

#define TICK_LED_BLINK 250
// Ustalenie czasu jak d³ugo ma LED_TX LED_RX œwieciæ po wyzwoleniu funkcjami Blink_RX() oraz Blink_TX()

#define XB_GUI
// Jeœ³i zdefiniowano to ca³y system zacznie korzystaæ z biblioteki xb_gui.h

#define XB_PREFERENCES
// Jeœli zdefiniowano to biblioteka zacznie korzystaæ do obs³ugi konfiguracji w pamiêci flash z 
// biblioteki Preferences.h (ESP32) .


// Jeœli chcemy aby funkcja Log() biblioteki wysy³a³a na UART jakieœ komunikaty oraz ¿eby 
// biblioteka pobiera³a kody z RX jako wciskanie klawiszy, nale¿y najpierw dodaæ do projektu
// biblioteke XB_SERIAL. Podstawowa konfiguracja XB_SERIAL polega na ustawieniu takich makr:

#define Serial0Board_BAUD 115200
// Okreœla szybkoœæ pierwszego UARTa w urz¹dzeniu, oraz równoczeœnie go uruchamia

#define SERIAL0_SizeRXBuffer 1024
// Definicja wielkoœci bufora odbiorczego

#define Serial0Board_UseKeyboard
// Jeœli zdefiniujemy to zostanie dodany stream z którego korzystaæ bêdzie klawiatura jako kody 
// klawiszy nadchodz¹ce w RX

#define Serial0Board_UseLog
// Jeœli zdefiniowane to biblioteka XB_BOARD a konkretnie funkcja Log() zacznie korzystaæ poprzez
// stream do wysy³ania komunikatów na TX uarta pierwszego

#define Serial0Board_UseGui
// Jeœli zdefiniowane to na pierwszy uart bêdzie rysowane GUI

#define Serial0Board_RX_PIN ?
#define Serial0Board_TX_PIN ?
// Dla ESP32 mo¿emy zdefiniowaæ na których pinach zostanie uruchomiony pierwszy UART.
// Jeœli nie podamy tych definicji to u¿yte zostan¹ standardowe przyporz¹dkowania pinów.

#define Serial0Board_UseDebugOutPut
// Jeœli zdefiniujemy to na pierwszy UART zostan¹ skierowane komunikaty diagnostyczne frameworku 
// arduino dla ESP32.

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
#endif

#ifdef ARDUINO_ARCH_STM32
#include <utils/xb_util.h>
#endif

#ifdef ESP32
#include <utils/xb_util.h>
#endif

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#include <Ticker.h>
#include <utils/xb_util.h>
#endif

typedef enum { 
	ftData, 
	ftResponseOK, 
	ftResponseError, 
	ftResponseCRCError, 
	ftBufferIsFull, 
	ftOKWaitForNext, 
	ftUnrecognizedType, 
	ftThereIsNoSuchTask 
} TFrameType;

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

#ifndef Serial0BoardBuf_BAUD
#define board_log(str) Serial.print(str)
#else
#define board_log(str) board.Log(String(str).c_str())
#endif

#ifdef XB_PREFERENCES
#include <Preferences.h>
#endif

#ifndef BOARD_TASKNAME_MAXLENGTH
#define BOARD_TASKNAME_MAXLENGTH 16
#endif

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
	DEFLIST_VAR(THandleDataFrameTransport,HandleDataFrameTransportList)
	int8_t dosetupRC;
	int8_t doloopRC;
	int8_t domessageRC;
	int32_t dointerruptRC;
	uint32_t TickReturn;
	uint32_t TickWaitLoop;

#ifdef XB_GUI
	uint32_t LastSumTaskStatusText;
	bool LastBoldTaskStatus;
#endif 

	
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

struct THDFT_ResponseItem
{
	THDFT_ResponseItem *Next;
	THDFT_ResponseItem *Prev;
	THDFT hdft;
	uint32_t ltsize;
	TTaskDef *TaskDefStream;
	uint32_t DestAddress;
};

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

struct TGPIODrive
{
	TGPIODrive* Next;
	TGPIODrive* Prev;
	uint16_t FromPin;
	uint16_t ToPin;
	TTaskDef* OwnerTask;
	String Name;
	void* UserData;
};


struct TFarDeviceID
{
	TFarDeviceID* Next;
	TFarDeviceID* Prev;
	TUniqueID FarDeviceID;
	TTaskDef* RequestTaskStream;
	uint32_t FarAddress;
	uint32_t TickLastRequest;
	uint32_t DateTimeLastRequest;

};

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
#ifndef CONSOLE_WIDTH_DEFAULT
#define CONSOLE_WIDTH_DEFAULT 80
#endif
#ifndef CONSOLE_HEIGHT_DEFAULT
#define CONSOLE_HEIGHT_DEFAULT 25
#endif

typedef struct {
	uint8_t Color0 : 2;
	uint8_t Color1 : 2;
	uint8_t Color2 : 2;
	uint8_t Color3 : 2;
} TConsoleColor;

class TConsoleScreen 
{
public:
	TConsoleScreen();
	~TConsoleScreen();

	void Set_ColorBuf(uint32_t Aindx, uint8_t Acolor);
	uint8_t Get_ColorBuf(uint32_t Aindx);

	void CreateConsoleInWindow();
	void DestroyConsoleInWindow();
	void ScrollUPConsole();
	void PutCharConsole(uint8_t Ach);
	void PutConsole(uint8_t* Adata, uint32_t Alength);

	uint8_t Color;
	uint8_t *Buf;
	uint32_t Size_Buf;
	TConsoleColor *ColorBuf;
	uint32_t Size_ColorBuf;
	uint8_t Currsor_X;
	uint8_t Currsor_Y;
	uint8_t Width;
	uint8_t Height;
	uint8_t Repaint_All;
};

#ifdef XB_BOARD_MEMDEBUG
struct TXBMemDebug
{
	TXBMemDebug* Next;
	TXBMemDebug* Prev;
	uint32_t Size;
	TTask* OwnerTask;
};
#endif

struct TBuf {
	uint32_t SectorSize;
	uint32_t AlarmMaxLength;
	uint8_t* Buf;
	uint32_t Length;
	uint32_t IndxW;
	uint32_t IndxR;
	uint32_t LastTickUse;
	uint32_t MaxLength;
};


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
	String ProjectName;
	TUniqueID DeviceID;
	uint32_t Tick_ESCKey;
	uint8_t TerminalFunction;
	bool CTRLKey;
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
	DEFLIST_VAR(TGPIODrive,GPIODriveList)
	uint16_t Digital_Pins_Count;

#ifdef BOARD_LED_TX_PIN
	uint32_t Tick_TX_BLINK;
#endif
#ifdef BOARD_LED_RX_PIN
	uint32_t Tick_RX_BLINK;
#endif
#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
	uint32_t TickEnableBlink;
#endif
	TGPIODrive* GetGPIODriveByPin(uint16_t Apin);
	TGPIODrive* AddGPIODrive(uint16_t Acountpin, String Aname);
	TGPIODrive* AddGPIODrive(uint16_t Acountpin, TTaskDef* Ataskdef, String Aname);
	TGPIODrive* AddGPIODrive(uint16_t Afrompin, uint16_t Atopin, String Aname);
	TGPIODrive* AddGPIODrive(uint16_t Afrompin, uint16_t Atopin, TTaskDef* Ataskdef, String Aname);
	TGPIODrive* GetGPIODriveByIndex(uint8_t Aindex);
	uint8_t GetGPIODriveCount();

	bool SetDigitalPinCount(uint16_t Acount);
	bool SetPinInfo(uint16_t Anumpin, uint8_t Afunction, uint8_t Amode, bool Alogwarn = false);

	bool pinMode(uint16_t pin, WiringPinMode mode);
	void digitalWrite(uint16_t pin, uint8_t value);
	uint8_t digitalRead(uint16_t pin);
	uint8_t digitalToggle(uint16_t pin);

	void SoftResetMCU(bool Asendmessage = true);

	void Blink_RX(int8_t Auserid = -1);
	void Blink_TX(int8_t Auserid = -1);
	void Blink_Life();
	//-----------------------------------------------------------------------------------------------------------------
	
	int8_t doAllInterruptRC;
	DEFLIST_VAR(TTask,TaskList)
	TTask *CurrentTask;
	
	void handle();	
	TTask *AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID = 0);
	bool DelTask(TTaskDef *Ataskdef);
	TTask *GetTaskByIndex(uint8_t Aindex);
	TTaskDef *GetTaskDefByName(String ATaskName);
	TTask* GetTaskByName(String ATaskName);
	void IterateTask();
	void TriggerInterrupt(TTaskDef *Ataskdef);
	void DoInterrupt(TTaskDef *Ataskdef);
	void CancelWaitTask(TTask* ATask);
	void CancelWaitTask();
	//-----------------------------------------------------------------------------------------------------------------	
	
	bool DoMessage(TMessageBoard *mb, bool Arunagain, TTask *Afromtask, TTaskDef *Atotaskdef);
	bool DoMessageOnAllTask(TMessageBoard *mb, bool Arunagain, TDoMessageDirection ADoMessageDirection, TTask *Afromtask = NULL, TTaskDef *Aexcludetask = NULL);
	bool DoMessageByTaskName(String Ataskname, TMessageBoard *mb, bool Arunagain);
	bool SendMessage_GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString);
	bool SendMessage_GetTaskNameString(TTaskDef *ATaskDef, String &APointerString);
	bool SendMessage_GetTaskNameString(TTask* ATask, String& APointerString);
	void SendMessage_OTAUpdateStarted();
	void SendMessage_FunctionKeyPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Aexcludetask=NULL, TTaskDef* Afromstreamtask=NULL);
	void SendMessage_KeyPress(char Akey, TTaskDef *Aexcludetask = NULL, TTaskDef* Afromstreamtask = NULL);
	void SendMessage_FREEPTR(void *Aptr);
	void SendMessage_REALLOCPTR(void* Aoldptr, void* Anewptr);
	void SendMessage_RTCSYNC();
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
	bool AutoCheckHeapIntegrity=false;

	uint32_t getFreePSRAM();
	uint32_t getFreeHeap();
	void *_malloc_psram(size_t size);
	void *_realloc_psram(void* Aptr, size_t Asize);
	void *_malloc(size_t size);
	void free(void *Aptr);
	void freeandnull(void **Aptr);
	//-----------------------------------------------------------------------------------------------------------------
	
	bool HandleFrameTransportInGetStream;
	DEFLIST_VAR(THDFT_ResponseItem,HDFT_ResponseItemList)

	uint32_t GetStream(void *Adata, uint32_t Amaxlength, TTaskDef *AStreamtaskdef, uint32_t Afromaddress = 0);
	uint32_t PutStream(void *Adata, uint32_t Alength, TTaskDef *AStreamtaskdef, uint32_t AToAddress = 0);
	bool IsTaskStream(TTaskDef* AStreamTaskDef);
	void BeginUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress);
	void EndUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress);
	bool GetStreamLocalAddress(TTaskDef* AStreamTaskDef, uint32_t* Alocaladdress);
	bool DisableTXStream(TTaskDef* AStreamTaskDef);
	bool EnableTXStream(TTaskDef* AStreamTaskDef);
	bool StatusDisableTXStream(TTaskDef* AStreamTaskDef);
	bool HandleDataFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport, TTaskDef *ATaskDefStream);
	bool GetFromErrFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport);
	THandleDataFrameTransport *AddToTask_HandleDataFrameTransport(TTaskDef *AStreamtaskdef, uint32_t Afromaddress);
	void CheckOld_HandleDataFrameTransport(TTask *ATask = NULL);
	bool SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, String AOnStreamTaskName, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	bool SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, TTaskDef *ATaskDefStream, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID);
	void SendResponseFrameOnProt(uint32_t AFrameID, TTaskDef *ATaskDefStream, uint32_t Afromaddress, uint32_t Atoaddress, TFrameType AframeType, TUniqueID ADeviceID);
	void HandleFrame(TFrameTransport *Aft, TTaskDef *ATaskDefStream);
	void HandleFrameLocal(TFrameTransport *Aft);
	bool RequestFarDevice(TUniqueID AFarDeviceID, TTaskDef* ATaskDefStream, uint32_t AOnAddress);
	void EraseAllFarDevices();
	void SaveCfgFarDevice(TFarDeviceID* Afdl);
	void LoadCfgFarDevices();
	void ResetCfgFarDevices();
	DEFLIST_VAR(TFarDeviceID, FarDeviceIDList)

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

	uint8_t SumEnableTXStream();
	void AllPutStreamGui(void *Adata, uint32_t Alength);
	void AllPutStreamLog(void *Adata, uint32_t Alength);
	int print(String Atext);
	void Log(char Achr, TTypeLog Atl = tlInfo);
	void Log(const char *Atxt, bool puttime = false, bool showtaskname = false, TTypeLog Atl = tlInfo);

	TConsoleScreen *ConsoleScreen;
	
	//-----------------------------------------------------------------------------------------------------------------
#ifdef XB_PREFERENCES
	size_t preferences_freeEntries;
	Preferences *xbpreferences;

	bool PREFERENCES_BeginSection(String ASectionname);
	void PREFERENCES_EndSection();
	void PREFERENCES_CLEAR();
	void PREFERENCES_CLEAR(String Akey);
	size_t PREFERENCES_PutArrayBytes(const char* key, const void* array, size_t sizearray);
	size_t PREFERENCES_GetArrayBytes(const char* key, void* array, size_t maxsizearray);
	size_t PREFERENCES_PutBool(const char* key, const bool value);
	bool PREFERENCES_GetBool(const char* key, const bool defaultvalue);
	size_t PREFERENCES_PutINT8(const char* key, const int8_t value);
	int8_t PREFERENCES_GetINT8(const char* key, const int8_t defaultvalue);
	size_t PREFERENCES_GetString(const char* key, char* value, const size_t maxlen);
	String PREFERENCES_GetString(const char* key, String defaultvalue);
	uint32_t PREFERENCES_GetUINT32(const char* key, uint32_t defaultvalue);
	uint16_t PREFERENCES_GetUINT16(const char* key, uint16_t defaultvalue);
	uint8_t PREFERENCES_GetUINT8(const char* key, uint8_t defaultvalue);
	size_t PREFERENCES_PutString(const char* key, const char* value);
	size_t PREFERENCES_PutString(const char* key, String value);
	size_t PREFERENCES_PutUINT32(const char* key, uint32_t value);
	size_t PREFERENCES_PutUINT16(const char* key, uint16_t value);
	size_t PREFERENCES_PutUINT8(const char* key, uint8_t value);
	size_t PREFERENCES_PutINT16(const char* key, int16_t value);
	int16_t PREFERENCES_GetINT16(const char* key, int16_t defaultvalue);
	size_t PREFERENCES_PutDouble(const char* key, double value);
	double PREFERENCES_GetDouble(const char* key, double defaultvalue);
#endif
	void LoadConfiguration(TTaskDef* ATaskDef);
	void LoadConfiguration(TTask* ATask);
	void LoadConfiguration();
	void SaveConfiguration(TTaskDef* ATaskDef);
	void SaveConfiguration(TTask* ATask);
	void SaveConfiguration();
	void ResetConfiguration(TTaskDef* ATaskDef);
	void ResetConfiguration(TTask* ATask);
	void ResetConfiguration();
	void AllSaveConfiguration(void);
	void AllResetConfiguration(void);

	//-----------------------------------------------------------------------------------------------------------------

};

#ifdef XB_GUI
void XB_BOARD_Repaint_TFarDeviceID();
#endif

bool BUFFER_Write_UINT8(TBuf* Abuf, uint8_t Av, bool Alogmessage=false);
uint32_t BUFFER_GetSizeData(TBuf* Abuf);
bool BUFFER_Read_UINT8(TBuf* Abuf, uint8_t* Av);
void BUFFER_Flush(TBuf* Abuf);
bool BUFFER_Handle(TBuf* Abuf, uint32_t Awaitforfreebyf);
uint8_t* BUFFER_GetReadPtr(TBuf* Abuf);
void BUFFER_Readed(TBuf* Abuf, uint32_t Areadedbyte);
void BUFFER_Reset(TBuf* Abuf);
uint8_t* BUFFER_GetBufferPtrAndReset(TBuf* Abuf);

extern TXB_board board;
extern TTaskDef XB_BOARD_DefTask;
extern volatile uint32_t DateTimeUnix;
extern volatile uint32_t DateTimeStart;
extern uint8_t xb_board_currentselecttask;
extern uint8_t xb_board_currentYselecttask;

void __HandlePTR(void** Aptr, TMessageBoard* Am);

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

#ifndef BOARD_NUM_DIGITAL_PINS
#define BOARD_NUM_DIGITAL_PINS NUM_DIGITAL_PINS
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
