#pragma region INCLUDES
#include <xb_board.h>

// Dla ESP8266 inkludy z funkcjami j�zyka C
#ifdef ESP8266
extern "C" {
#include "user_interface.h"	
}
#endif

#ifdef ESP32
#include <stddef.h>
#endif

// Dla STM32 inkludy z funkcjami j�zyka C
#ifdef ARDUINO_ARCH_STM32
extern "C" {
#include <string.h>
#include <stdlib.h>
}
#endif

#ifdef XB_PREFERENCES
#ifdef ESP32
#include "../../../hardware/espressif/esp32master/libraries/Preferences/src/Preferences.h"
#else
#error "XB_PREFERENCES not support"
#endif
#endif

#ifdef Serial0BoardBuf_BAUD
#include <XB_SERIAL.h>
#endif

#ifdef XB_GUI
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>
#endif


#pragma endregion

#pragma region GLOBAL_VARS
// Podstawowy obiekt tzw. "kernel"
TXB_board board; 

// Data i Czas w formacie Unix
volatile uint32_t DateTimeUnix;
// Sekundy kt�re up�yn�y od startu urz�dzenia
volatile uint32_t DateTimeStart;
// SysTick na ESP8266
#if defined(ESP8266) 
volatile uint32_t __SysTickCount;
#endif

// Konfiguracja ---------------------------------------------------------------------
bool xb_board_ShowGuiOnStart = false;
bool xb_board_ConsoleInWindow = false;
bool xb_board_CFG_ConsoleInWindow = false;
uint8_t xb_board_ConsoleWidth = CONSOLE_WIDTH_DEFAULT;
uint8_t xb_board_ConsoleHeight = CONSOLE_HEIGHT_DEFAULT;
bool xb_board_Consoleputtoserial = false;
bool xb_board_ShowListFarDeviceID = false;

// Zmienne i funkcjonalno�� GUI do zadania systemowego
#ifdef XB_GUI

#ifdef __riscv64
#define WINDOW_0_CAPTION "BOARD (Sipeed Maix), 400Mhz)"
#endif
#ifdef ESP8266
#define WINDOW_0_CAPTION "BOARD (ESP8266, 160Mhz)"
#endif
#ifdef ESP32
	#ifdef ARDUINO_ESP32S2_DEV
		#ifdef BOARD_HAS_PSRAM
		#define WINDOW_0_CAPTION "BOARD (ESP32S2 wROVER)"
		#else
		#define WINDOW_0_CAPTION "BOARD (ESP32S2)"
		#endif
	#elif defined(ARDUINO_ESP32C3_DEV)
		#ifdef BOARD_HAS_PSRAM
		#define WINDOW_0_CAPTION "BOARD (ESP32-C3-S32)"
		#else
		#define WINDOW_0_CAPTION "BOARD (ESP32-C3-S32)"
		#endif
	#else
		#ifdef BOARD_HAS_PSRAM
		#define WINDOW_0_CAPTION "BOARD (ESP32 wROVER)"
		#else
		#define WINDOW_0_CAPTION "BOARD (ESP32)"
		#endif
	#endif
#endif



#ifdef ARDUINO_ARCH_STM32
#define WINDOW_0_CAPTION "BOARD (STM32)"
#endif

#ifdef BOARD_HAS_PSRAM
#define WINDOW_0_HEIGHT board.TaskList_count + 14
#else
#define WINDOW_0_HEIGHT board.TaskList_count + 12
#endif

TWindowClass * xb_board_winHandle0;
TWindowClass* xb_board_winHandle1;
TWindowClass* xb_board_winHandle2;
uint8_t xb_board_currentselecttask = 0;
uint8_t xb_board_currentYselecttask;
bool xb_board_listtask_repaint = false;

TGADGETMenu *xb_board_menuHandle1;
TGADGETInputDialog *xb_board_inputdialog0;
#endif

#pragma endregion
#pragma region FUNKCJE_LICZNIKOW_CZASOWYCH
#if defined(ESP8266) 
Ticker SysTickCount_ticker;
Ticker DateTimeSecond_ticker;

void SysTickCount_proc(void)
{
	__SysTickCount++;
}

void DateTimeSecond_proc(void)
{
	DateTimeUnix++;
}

void TXB_board::SysTickCount_init(void)
{
	SysTickCount_ticker.attach_ms(1, SysTickCount_proc);
}

void TXB_board::DateTimeSecond_init(void)
{
	DateTimeSecond_ticker.attach(1, DateTimeSecond_proc);
}
#endif

#pragma endregion
#pragma region KONFIGURACJA


// -------------------------------------
bool XB_BOARD_LoadConfiguration()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("XBBOARD"))
	{
		board.DeviceName = board.PREFERENCES_GetString("DeviceName", board.DeviceName);
		xb_board_ShowGuiOnStart = board.PREFERENCES_GetBool("ShowGuiOnStart", xb_board_ShowGuiOnStart);
		xb_board_ConsoleInWindow = board.PREFERENCES_GetBool("ConInWin", xb_board_ConsoleInWindow);
		xb_board_CFG_ConsoleInWindow = board.PREFERENCES_GetBool("ConInWin", xb_board_ConsoleInWindow);
		xb_board_ConsoleWidth = board.PREFERENCES_GetUINT8("ConWidth", xb_board_ConsoleWidth);
		xb_board_ConsoleHeight = board.PREFERENCES_GetUINT8("ConHeight", xb_board_ConsoleHeight);
		board.AutoCheckHeapIntegrity = board.PREFERENCES_GetBool("ACHI", board.AutoCheckHeapIntegrity);
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false;
#endif
}

bool XB_BOARD_SaveConfiguration()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("XBBOARD"))
	{
		board.PREFERENCES_PutBool("ConInWin", xb_board_ConsoleInWindow);
		board.PREFERENCES_PutBool("ShowGuiOnStart", xb_board_ShowGuiOnStart);
		board.PREFERENCES_PutString("DeviceName", board.DeviceName);
		board.PREFERENCES_PutUINT8("ConWidth", xb_board_ConsoleWidth);
		board.PREFERENCES_PutUINT8("ConHeight", xb_board_ConsoleHeight);
		board.PREFERENCES_PutBool("ACHI", board.AutoCheckHeapIntegrity);
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false;
#endif
}

bool XB_BOARD_ResetConfiguration()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("XBBOARD"))
	{
		board.PREFERENCES_CLEAR();
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false;
#endif
}
#pragma endregion

#pragma region FUNKCJE_INICJUJACE_TXB_BOARD
TXB_board::TXB_board()
{
	// Reset zmiennych
	Tick_ESCKey = 0;
	board.TerminalFunction = 0;
	TaskList = NULL;
	TaskList_count = 0;
	TaskList_last = NULL;
	ConsoleScreen = NULL;
#ifdef XB_PREFERENCES
	xbpreferences = NULL;
#endif
	FarDeviceIDList = NULL;
	FarDeviceIDList_count = 0;
	FarDeviceIDList_last = NULL;
	AutoCheckHeapIntegrity = false;

	iteratetask_procedure = false;
	setup_procedure = false;
	CurrentTask = NULL;
	NoTxCounter = 0;
	TXCounter = 0;
	doAllInterruptRC = 0;
	Digital_Pins_Count = 0;
	PinInfoTable = NULL;
	GPIODriveList = NULL;

	Default_ShowLogInfo = true;
	Default_ShowLogWarn = true;
	Default_ShowLogError = true;

	HandleFrameTransportInGetStream = true;
	#ifdef BOARD_DEVICE_NAME
	DeviceName = BOARD_DEVICE_NAME;
#else
	DeviceName = "NoName";
#endif
#ifdef BOARD_DEVICE_VERSION
	DeviceVersion = BOARD_DEVICE_VERSION;
#else
	DeviceVersion = "0.0";
#endif
	DeviceID = GetUniqueID();
#ifndef PROJECT_NAME
#define PROJECT_NAME "no define"
#endif 
	ProjectName = PROJECT_NAME;
	HDFT_ResponseItemList = NULL;

	

#ifndef Serial0BoardBuf_BAUD
	Serial.begin(115200);
	delay(100);
#endif


}

TXB_board::~TXB_board()
{
	TTask *t = TaskList;
	while (t != NULL)
	{
		DelTask(t->TaskDef);
		t = TaskList;
	}
	freeandnull((void **)&PinInfoTable);
}
#pragma endregion
#pragma region FUNKCJE_NARZEDZIOWE
void TXB_board::SoftResetMCU(bool Asendmessage)
{
	if (Asendmessage)
	{
		TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
		mb.IDMessage = IM_BEFORE_RESET;
		DoMessageOnAllTask(&mb, true, doBACKWARD);
	}

#if defined(ESP8266) || defined(ESP32)
	ESP.restart();
	delay(5000);
#elif defined(ARDUINO_ARCH_STM32)
	board.Log("Reset no support!", true, true, tlError);
#else
	board.Log("Reset no support!", true, true, tlError);
#endif
}

TUniqueID TXB_board::GetUniqueID()
{
	TUniqueID V;
	V.ID.ID64 = 0;

#if defined(ESP8266) 
	uint8 mac[8];
	if (wifi_get_macaddr(0,mac))
	{
		V.ID.ID[0] = mac[0];
		V.ID.ID[1] = mac[1];
		V.ID.ID[2] = mac[2];
		V.ID.ID[3] = mac[3];
		V.ID.ID[4] = mac[4];
		V.ID.ID[5] = mac[5];
		V.ID.ID[6] = mac[6];
		V.ID.ID[7] = mac[7];
		V.ID.ID[6] = V.ID.ID[0];
		V.ID.ID[0] = V.ID.ID[1] + V.ID.ID[2];
		V.ID.ID[7] = V.ID.ID[5] + V.ID.ID[6];
	}
#endif
#if defined(ESP32)
	V.ID.ID64=ESP.getEfuseMac();
	V.ID.ID[6] = V.ID.ID[0];
	V.ID.ID[0] = V.ID.ID[1] + V.ID.ID[2];
	V.ID.ID[7] = V.ID.ID[5] + V.ID.ID[6];
#endif

#ifdef ARDUINO_ARCH_STM32

	uint8_t uid[12];
	HAL_GetUID((uint32_t *)&uid);
	
	V.ID.ID[0] = uid[0];
	V.ID.ID[1] = uid[1] + uid[7];
	V.ID.ID[2] = uid[2] + uid[8];
	V.ID.ID[3] = uid[3] + uid[9];
	V.ID.ID[4] = uid[4] + uid[10];
	V.ID.ID[5] = uid[5] + uid[11];
	V.ID.ID[6] = uid[6];
	V.ID.ID[7] = crc8(uid, 12);

	
#endif

	return V;
}
// This table comes from Dallas sample code where it is freely reusable,
// though Copyright (C) 2000 Dallas Semiconductor Corporation
static const uint8_t PROGMEM dscrc_table[] = {
	0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
	157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
	35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
	190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
	70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
	219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
	101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
	248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
	140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
	17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
	175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
	50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
	202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
	87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
	233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
	116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53 };

// Compute a Dallas Semiconductor 8 bit CRC. These show up in the ROM
// and the registers.  (note: this might better be done without to
// table, it would probably be smaller and certainly fast enough
// compared to all those delayMicrosecond() calls.  But I got
// confused, so I use this table from the examples.)
//
uint8_t TXB_board::crc8(const uint8_t *addr, uint8_t len)
{
	uint8_t crc = 0;

	while (len--) {
		crc = dscrc_table[(crc ^ *addr++)];
		//crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
	}
	return crc;
}

typedef enum
{
	sfNormalChar, 
	sfGetVarName, 
	sfCancelGetVarName
} TStepFilter;

void TXB_board::FilterString(const char *Asourcestring, String &Adestinationstring)
{
	uint32_t lensource = StringLength(Asourcestring, 0);
	uint32_t indx_s = 0;
	Adestinationstring = "";
	Adestinationstring.reserve(lensource * 2);
	char ch = 0;
	String varname = "";
	varname.reserve(256);
	String varvalue = "";
	varvalue.reserve(256);
	TStepFilter sf = sfNormalChar;
	bool result;
	
	while (indx_s < lensource)
	{
		ch = Asourcestring[indx_s];
		switch (sf)
		{
		case sfNormalChar:
			{
				if (ch == '%')
				{
					sf = sfGetVarName;
					varname = "";
					indx_s++;
				}
				else
				{
					Adestinationstring += ch;

					indx_s++;
				}
				break;
			}
		case sfGetVarName:
			{
				if (ch == '%')	
				{
					 // Koniec nazwy zmiennej
					if(varname.length() > 0)					
					{
						TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
						mb.IDMessage = IM_VAR;
						mb.fromTask = NULL;
						mb.Data.VarData.VarName = &varname;
						mb.Data.VarData.VarValue = &varvalue;
						result = DoMessageOnAllTask(&mb, true, doFORWARD);
						if (result)
						{
								// Wstaw zawarto��
							Adestinationstring += varvalue;
							varname = "";
							varvalue = "";
							indx_s++;
							sf = sfNormalChar;
						}
						else
						{
							  // wstaw nazwe zmiennej bo niema w systemie takiej

							Adestinationstring += '%';
							Adestinationstring += varname;
							Adestinationstring += '%';
							varname = "";
							sf = sfGetVarName;
							indx_s++;
						}
					}
					else
					{
						Adestinationstring += ch;
						sf = sfNormalChar;
					}
				}
				else
				{
					 // Kolejny znak nazwy zmienne;
					varname += ch;
					indx_s++;
					
					if (indx_s >= lensource)
					{
						 // Anuluj nazwe zmiennej bo koniec �r�d�a
						indx_s--;
						sf = sfCancelGetVarName;		
					}
				}
				break;
			}
		case sfCancelGetVarName:
			{
				Adestinationstring += '%';
				Adestinationstring += varname;
				varname = "";
				sf = sfNormalChar;
				indx_s++;
				break;
			}
			
		}
	
	}
}

void TXB_board::FilterString(const char* Asourcestring, TBuf *Adestinationstring)
{
	uint32_t lensource = StringLength(Asourcestring, 0);
	uint32_t indx_s = 0;
	char ch = 0;
	String varname = "";
	varname.reserve(256);
	String varvalue = "";
	varvalue.reserve(256);
	TStepFilter sf = sfNormalChar;
	bool result;

	while (indx_s < lensource)
	{
		ch = Asourcestring[indx_s];
		switch (sf)
		{
		case sfNormalChar:
		{
			if (ch == '%')
			{
				sf = sfGetVarName;
				varname = "";
				indx_s++;
			}
			else
			{
				BUFFER_Write_UINT8(Adestinationstring, (uint8_t)ch);

				indx_s++;
			}
			break;
		}
		case sfGetVarName:
		{
			if (ch == '%')
			{
				// Koniec nazwy zmiennej
				if (varname.length() > 0)
				{
					TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
					mb.IDMessage = IM_VAR;
					mb.fromTask = NULL;
					mb.Data.VarData.VarName = &varname;
					mb.Data.VarData.VarValue = &varvalue;
					result = DoMessageOnAllTask(&mb, true, doFORWARD);
					if (result)
					{
						// Wstaw zawarto��
						for (int i = 0; i < varvalue.length(); i++)  BUFFER_Write_UINT8(Adestinationstring, (uint8_t)varvalue[i]);
						varname = "";
						varvalue = "";
						indx_s++;
						sf = sfNormalChar;
					}
					else
					{
						// wstaw nazwe zmiennej bo niema w systemie takiej

						BUFFER_Write_UINT8(Adestinationstring, (uint8_t)'%');
						for (int i = 0; i < varname.length(); i++)  BUFFER_Write_UINT8(Adestinationstring, (uint8_t)varname[i]);
						BUFFER_Write_UINT8(Adestinationstring, (uint8_t)'%');

						varname = "";
						sf = sfGetVarName;
						indx_s++;
					}
				}
				else
				{
					Adestinationstring += ch;
					sf = sfNormalChar;
				}
			}
			else
			{
				// Kolejny znak nazwy zmienne;
				varname += ch;
				indx_s++;

				if (indx_s >= lensource)
				{
					// Anuluj nazwe zmiennej bo koniec �r�d�a
					indx_s--;
					sf = sfCancelGetVarName;
				}
			}
			break;
		}
		case sfCancelGetVarName:
		{
			BUFFER_Write_UINT8(Adestinationstring, (uint8_t)'%');
			for(int i=0;i<varname.length();i++)  BUFFER_Write_UINT8(Adestinationstring, (uint8_t)varname[i]);

			varname = "";
			sf = sfNormalChar;
			indx_s++;
			break;
		}
		}
	}
}



String TXB_board::DeviceIDtoString(TUniqueID Adevid)
{
	char strid[25];
	xb_memoryfill(strid, 25, 0);
	uint8tohexstr(strid, (uint8_t *)&Adevid, 8, ':');

	return String(strid);
}

String BOARD_GetString_TFrameType(TFrameType Aft)
{
	switch (Aft)
	{
		GET_ENUMSTRING(ftResponseOK, 2);
		GET_ENUMSTRING(ftResponseError, 2);
		GET_ENUMSTRING(ftResponseCRCError, 2);
		GET_ENUMSTRING(ftBufferIsFull, 2);
		GET_ENUMSTRING(ftOKWaitForNext, 2);
		GET_ENUMSTRING(ftUnrecognizedType, 2);
		GET_ENUMSTRING(ftThereIsNoSuchTask, 2);
	}
	return "No ident Enum";
}

#pragma endregion
#pragma region FUNKCJE_BUFFER_HANDLE
// Zapis bajtu do bufora
bool BUFFER_Write_UINT8(TBuf* Abuf, uint8_t Av, bool Alogmessage)
{
	if (Abuf->Buf == NULL)
	{
		Abuf->Buf = (uint8_t*)board._malloc_psram(Abuf->SectorSize);
		if (Abuf->Buf == NULL)
		{
			if (Alogmessage) board.Log("Memory problem in reserved buffer...", true, true, tlError);
			return false;
		}
		Abuf->Length = Abuf->SectorSize;
		Abuf->IndxR = 0;
		Abuf->IndxW = 0;
	}
	else
	{ // Tu je�li si� oka�e �e bufor pe�ny

		if (Abuf->IndxW >= Abuf->Length)
		{
			uint8_t *newptr= (uint8_t*)board._realloc_psram(Abuf->Buf, Abuf->Length + Abuf->SectorSize);

			if (newptr == NULL)
			{
				if (Alogmessage) board.Log("Memory problem in realloc buffer...", true, true, tlError);
				return false;
			}
			Abuf->Buf = newptr;
			Abuf->Length += Abuf->SectorSize;
		}
	}

	if (Abuf->IndxR == Abuf->IndxW)
	{
		Abuf->IndxR = 0;
		Abuf->IndxW = 0;
	}

	if (Abuf->IndxR >= Abuf->SectorSize)
	{
		if (Abuf->IndxW > Abuf->IndxR)
		{
			uint32_t l = Abuf->IndxW - Abuf->IndxR;
			for (uint32_t i = 0; i < l; i++)
			{
				Abuf->Buf[i] = Abuf->Buf[i + Abuf->IndxR];
			}
			Abuf->IndxR = 0;
			Abuf->IndxW = l;
		}
	}

	Abuf->Buf[Abuf->IndxW] = Av;
	Abuf->IndxW++;
	Abuf->LastTickUse = SysTickCount;

	if (Abuf->IndxW > Abuf->MaxLength)
	{
		Abuf->MaxLength = Abuf->IndxW;
	}
	
	if (Abuf->AlarmMaxLength != 0)
	{
		if (Abuf->MaxLength >= Abuf->AlarmMaxLength)
		{
			if (Alogmessage) board.Log("Max Length buffer is alarm length...", true, true, tlError);
			return false;
		}
	}

	
	return true;
}
// Odczyt bajtu z bufora
bool BUFFER_Read_UINT8(TBuf* Abuf, uint8_t* Av)
{
	if (Abuf->Buf == NULL)
	{
		return false;
	}

	if (Abuf->IndxR == Abuf->IndxW)
	{
		return false;
	}

	if (Av == NULL)
	{
		return false;
	}

	*Av = Abuf->Buf[Abuf->IndxR];
	Abuf->IndxR++;
	Abuf->LastTickUse = SysTickCount;
	return true;
}
// resetowanie indeks�w zapisu odczytu
void BUFFER_Flush(TBuf* Abuf)
{
	Abuf->IndxR = 0;
	Abuf->IndxW = 0;
	Abuf->LastTickUse = SysTickCount;
}
// Funkcja odpowiada za zwolnienie pami�ci zaj�tej przez bufor po okreslonym czasie bezczynno�ci
bool BUFFER_Handle(TBuf* Abuf, uint32_t Awaitforfreebyf)
{
	if (Abuf->Buf != NULL)
	{
		if (SysTickCount - Abuf->LastTickUse > Awaitforfreebyf)
		{
			
			board.free(Abuf->Buf);
			Abuf->Buf = NULL;
			Abuf->Length = 0;
			BUFFER_Flush(Abuf);
			return true;
		}
	}
	return false;
}
// Odczyt ilosci bajt�w aktualnie gotowych do odczytu z bufora
uint32_t BUFFER_GetSizeData(TBuf* Abuf)
{
	return Abuf->IndxW - Abuf->IndxR;
}
// Odczyt wska�nika danych aktualnie gotowych do odczytu
uint8_t* BUFFER_GetReadPtr(TBuf* Abuf)
{
	Abuf->LastTickUse = SysTickCount;
	if (Abuf->Buf == NULL) return NULL;
	return &Abuf->Buf[Abuf->IndxR];
}
// Przesuni�cie indeksu odczytu z bufora aktualnie gotowych danych do odczytu
void BUFFER_Readed(TBuf* Abuf, uint32_t Areadedbyte)
{
	if (Abuf->IndxR == Abuf->IndxW)
	{
		Abuf->LastTickUse = SysTickCount;
		return;
	}

	Abuf->IndxR += Areadedbyte;

	if (Abuf->IndxR >= Abuf->IndxW)
	{
		BUFFER_Flush(Abuf);
		return;
	}

	Abuf->LastTickUse = SysTickCount;
}
// Wymuszenie zwolnienia pami�ci zaj�tej przez bufor i resetowanie indeks�w odczytu i zapisu
void BUFFER_Reset(TBuf* Abuf)
{
	if (Abuf->Buf != NULL)
	{
		board.free(Abuf->Buf);
		Abuf->Buf = NULL;
		Abuf->Length = 0;
		BUFFER_Flush(Abuf);
	}
}
// Przej�cie pami�ci zaj�tej przez dane gotowe do odczytu przez otrzymanie wska�nika do danych
// Nast�pnie resetowanie bufora do gotowo�ci przyj�cia kolenych danych
uint8_t* BUFFER_GetBufferPtrAndReset(TBuf* Abuf)
{
	if (Abuf == NULL) return NULL;
	if (Abuf->IndxW == Abuf->IndxR) return NULL;
	if (Abuf->Buf == NULL) return NULL;
	uint32_t l = Abuf->IndxW - Abuf->IndxR;

	if (Abuf->IndxR > 0)
	{
		for (uint32_t i = 0; i < l; i++)
		{
			Abuf->Buf[i] = Abuf->Buf[i + Abuf->IndxR];
		}
		Abuf->IndxR = 0;
		Abuf->IndxW = l;
	}

	uint8_t* ptr =(uint8_t *) board._realloc_psram(Abuf->Buf, l);

	if (ptr != NULL)
	{

		Abuf->Buf = NULL;
		Abuf->IndxR = 0;
		Abuf->IndxW = 0;
		Abuf->LastTickUse = SysTickCount;
		Abuf->Length = 0;
	}

	return ptr;
}
#pragma endregion
#pragma region FUNKCJE_GPIO
// -----------------------------------------
TGPIODrive* TXB_board::GetGPIODriveByPin(uint16_t Apin)
{
	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin <= Apin) && (GD->ToPin >= Apin))
		{
			return GD;
		}
		GD = GD->Next;
	}
	return NULL;
}
// -----------------------------------------
// Dodanie drivera od GPIO
TGPIODrive* TXB_board::AddGPIODrive(uint16_t Acountpin,String Aname)
{
	if (CurrentTask != NULL)
		if (CurrentTask->TaskDef != NULL)
			return AddGPIODrive(Acountpin, CurrentTask->TaskDef,Aname);
	return NULL;
}
TGPIODrive* TXB_board::AddGPIODrive(uint16_t Acountpin, TTaskDef* Ataskdef, String Aname)
{
	uint16_t frompin = 0;
	uint16_t topin = 0;

	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if (GD->ToPin >frompin) 
		{
			frompin = GD->ToPin+1;
		}
		GD = GD->Next;
	}

	topin=frompin+(Acountpin-1);

	return AddGPIODrive(frompin,topin,Ataskdef,Aname);
}
TGPIODrive* TXB_board::AddGPIODrive(uint16_t Afrompin, uint16_t Atopin, String Aname)
{
	if (CurrentTask!=NULL)
		if (CurrentTask->TaskDef!=NULL)
			return AddGPIODrive(Afrompin,Atopin, CurrentTask->TaskDef,Aname);
	return NULL;
}
TGPIODrive* TXB_board::AddGPIODrive(uint16_t Afrompin, uint16_t Atopin,TTaskDef *Ataskdef, String Aname)
{
	if (Ataskdef == NULL) return NULL;
	if (Afrompin > Atopin)
	{
		uint16_t t = Atopin;
		Atopin = Afrompin;
		Afrompin = t;
	}
		
	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin == Afrompin) && (GD->ToPin == Atopin) && (GD->OwnerTask == Ataskdef))
		{
			return GD;
		}
		GD = GD->Next;
	}

	CREATE_STR_ADD_TO_LIST(GPIODriveList, TGPIODrive, GD);

	if (GD != NULL)
	{
		GD->FromPin = Afrompin;
		GD->ToPin = Atopin;
		GD->OwnerTask = Ataskdef;
		GD->UserData = NULL;
		GD->Name = Aname;
		return GD;
	}

	board.Log("Error add GPIO drive...", true, true, tlError);
	return NULL;
}
// -----------------------------------------
TGPIODrive* TXB_board::GetGPIODriveByIndex(uint8_t Aindex)
{
	TGPIODrive* GD = GPIODriveList;
	uint8_t i = 0;
	while (GD != NULL)
	{
		if (i == Aindex)
		{
			return GD;
		}
		i++;
		GD = GD->Next;
	}
	return NULL;
}
// -----------------------------------------
uint8_t TXB_board::GetGPIODriveCount()
{
	TGPIODrive* GD = GPIODriveList;
	uint8_t i = 0;
	while (GD != NULL)
	{
		i++;
		GD = GD->Next;
	}
	return i;
}

// -----------------------------------------
// Ustalenie ilo�ci pin�w w systemie
// -> Acount nowa ilo�� pin�w
bool TXB_board::SetDigitalPinCount(uint16_t Acount)
{
	if (Acount > Digital_Pins_Count)
	{
		board.freeandnull((void **)&PinInfoTable);
		PinInfoTable = (TPinInfo *)_malloc_psram(sizeof(TPinInfo) * Acount);
		Digital_Pins_Count = Acount;
	}
	
	if (PinInfoTable==NULL)
	{
		PinInfoTable = (TPinInfo*)_malloc_psram(sizeof(TPinInfo) * Digital_Pins_Count);
	}

	if (PinInfoTable == NULL)
	{
		Log("ERROR create pin table info...", true, true, tlError);
		return false;
	}

	return true;
}
// -----------------------------------------
// Ustawienie informacji o ustawieniach pinu
// Je�li funkcja zostanie wywo�ana w funkcji setup() zadania to zostan� wy�wietlone komunikaty o kolizji u�yteczno�ci pinu 
// -> Anumpin    - Numer pinu
// -> Afunction  - Numer funkcji jak� spe�nia pin
// -> Amode      - Tryb w jakim pin funkcjonuje
// -> Alogwarn   = true - Wy�wietla ostrze�enia kolizji
//               = false - Nie wy�wietla ostrze�enia kolizji
// <-            = true - wykonane z powodzeniem, ale kolizja mo�e zaistnie�
//               = false - b��d, pin nie obs�ugiwany
bool TXB_board::SetPinInfo(uint16_t Anumpin, uint8_t Afunction, uint8_t Amode, bool Alogwarn)
{
	if ((Anumpin >= 0) && (Anumpin < Digital_Pins_Count))
	{
		if (PinInfoTable != NULL)
		{
			if (PinInfoTable[Anumpin].use == 1)
			{
				if ((setup_procedure == true) || (Alogwarn == true))
				{
					String s = "WARN. Pin (" + String(Anumpin) + ") is use...";
					Log(s.c_str(), true, true, tlWarn);		
				}
			}
			PinInfoTable[Anumpin].use = 1;
			PinInfoTable[Anumpin].functionpin = Afunction;
			PinInfoTable[Anumpin].mode = Amode;
		}
	}
	else
	{
		Log("ERROR out of num pin", true, true, tlError);
		return false;	
	}
	return true;
}

//-------------------------
// Ustalenie trybu dla pinu
// -> pin  - numer pinu
// -> mode - tryb pinu
//	
bool TXB_board::pinMode(uint16_t pin, WiringPinMode mode)
{

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinMode;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Mode = mode;

	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin <= pin) && (GD->ToPin >= pin))
		{
			mb.Data.GpioData.GPIODrive = GD;
			DoMessage(&mb, true,CurrentTask,GD->OwnerTask);
			break;
		}
		GD = GD->Next;
	}

	if (GD == NULL)
	{
		board.Log("GPIO Drive not find...", true, true, tlError);
		return false;
	}

	mb.Data.GpioData.GpioAction = gaPinModeEvent;
	TTask* t = TaskList;
	while (t != NULL)
	{
		if (t->TaskDef != GD->OwnerTask)
		{
			DoMessage(&mb, true, CurrentTask, t->TaskDef);
		}
		t = t->Next;
	}

	return true;
}

//----------------------
// Ustawienie stanu pinu
// -> pin   - numer pinu
// -> value - warto�� stanu
void TXB_board::digitalWrite(uint16_t pin, uint8_t value)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinWrite;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Value = value;

	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin <= pin) && (GD->ToPin >= pin))
		{
			mb.Data.GpioData.GPIODrive = GD;
			DoMessage(&mb, true, CurrentTask, GD->OwnerTask);
			break;
		}
		GD = GD->Next;
	}

	if (GD == NULL)
	{
		board.Log("GPIO Drive not find...", true, true, tlError);
		return;
	}

	mb.Data.GpioData.GpioAction = gaPinWriteEvent;

	TTask* t = TaskList;
	while (t != NULL)
	{
		if (t->TaskDef != GD->OwnerTask)
		{
			DoMessage(&mb, true, CurrentTask, t->TaskDef);
		}
		t = t->Next;
	}

}
//------------------
// Odczyt stanu pinu
// -> pin - numer pinu
// <- aktualny stan pinu
uint8_t TXB_board::digitalRead(uint16_t pin)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinRead;
	mb.Data.GpioData.NumPin = pin;

	uint8_t result = 0;

	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin <= pin) && (GD->ToPin >= pin))
		{
			mb.Data.GpioData.GPIODrive = GD;
			DoMessage(&mb, true, CurrentTask, GD->OwnerTask);
			result = mb.Data.GpioData.ActionData.Value;
			break;
		}
		GD = GD->Next;
	}

	if (GD == NULL)
	{
		board.Log("GPIO Drive not find...", true, true, tlError);
		return result;
	}

	mb.Data.GpioData.GpioAction = gaPinReadEvent;
	TTask* t = TaskList;
	while (t != NULL)
	{
		if (t->TaskDef != GD->OwnerTask)
		{
			mb.Data.GpioData.ActionData.Value = result;
			DoMessage(&mb, true, CurrentTask, t->TaskDef);
		}
		t = t->Next;
	}

	return result;
}
//------------------------------
// Wykonanie toggle bit na pinie
// -> pin - numer pinu
// <- aktualny stan pinu
uint8_t TXB_board::digitalToggle(uint16_t pin)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinToggle;
	mb.Data.GpioData.NumPin = pin;

	uint8_t result = 0;

	TGPIODrive* GD = GPIODriveList;
	while (GD != NULL)
	{
		if ((GD->FromPin <= pin) && (GD->ToPin >= pin))
		{
			mb.Data.GpioData.GPIODrive = GD;
			DoMessage(&mb, true, CurrentTask, GD->OwnerTask);
			result = mb.Data.GpioData.ActionData.Value;
			break;
		}
		GD = GD->Next;
	}

	if (GD == NULL)
	{
		board.Log("GPIO Drive not find...", true, true, tlError);
		return result;
	}

	mb.Data.GpioData.GpioAction = gaPinToggleEvent;
	TTask* t = TaskList;
	while (t != NULL)
	{
		if (t->TaskDef != GD->OwnerTask)
		{
			mb.Data.GpioData.ActionData.Value = result;
			DoMessage(&mb, true, CurrentTask, t->TaskDef);
		}
		t = t->Next;
	}

	return result;
}
// ----------------------------------------------------------------------
// Wys�anie messaga informuj�cego �e urz�dzenie odczyta�o dane z zewn�trz
// ! je�li zdefiniowano BOARD_LED_RX_PIN to ustawienie tego pinu w stan wysoki na okre�lony czas, ma to zadanie poinformowa� u�ytkownika migaj�cym ledem �e nast�puje transmisja
// -> AuserId - jest to numer zdefiniowany przez aplikacje , kt�ry to b�dzie przekazywany w messagu do ka�dego zadania
void TXB_board::Blink_RX(int8_t Auserid)
{
#ifdef BOARD_LED_RX_PIN
#if defined(BOARD_LED_RX_STATUS_OFF)
	digitalWrite(BOARD_LED_RX_PIN, !(BOARD_LED_RX_STATUS_OFF));
#else
	digitalWrite(BOARD_LED_RX_PIN, HIGH);
#endif
	Tick_RX_BLINK = SysTickCount;
#endif
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_RX_BLINK;
	mb.Data.BlinkData.UserID = Auserid;
	DoMessageOnAllTask(&mb, true, doFORWARD,CurrentTask , &XB_BOARD_DefTask); 
}
// ----------------------------------------------------------------------
// Wys�anie messaga informuj�cego �e urz�dzenie wys�a�o dane na zewn�trz
// ! je�li zdefiniowano BOARD_LED_TX_PIN to ustawienie tego pinu w stan wysoki na okre�lony czas, ma to zadanie poinformowa� u�ytkownika migaj�cym ledem �e nast�puje transmisja
// -> AuserId - jest to numer zdefiniowany przez aplikacje , kt�ry to b�dzie przekazywany w messagu do ka�dego zadania
void TXB_board::Blink_TX(int8_t Auserid)
{
#ifdef BOARD_LED_TX_PIN
#if defined(BOARD_LED_TX_STATUS_OFF)
	digitalWrite(BOARD_LED_TX_PIN, !(BOARD_LED_TX_STATUS_OFF));
#else
	digitalWrite(BOARD_LED_TX_PIN, HIGH);
#endif
	Tick_TX_BLINK = SysTickCount;
#endif
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_TX_BLINK;
	mb.Data.BlinkData.UserID = Auserid;
	DoMessageOnAllTask(&mb, true, doFORWARD, CurrentTask, &XB_BOARD_DefTask); 
}
// ----------------------------------------------------------------------
// Wys�anie messaga informuj�cego �e urz�dzenie �e urz�dzenie �yje
// ! je�li zdefiniowano BOARD_LED_LIFE_PIN to nast�puje togglowanie stanu 
void TXB_board::Blink_Life()
{
#ifdef  BOARD_LED_LIFE_PIN
	digitalToggle(BOARD_LED_LIFE_PIN);
#endif
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_LIVE_BLINK;
	DoMessageOnAllTask(&mb, true, doFORWARD, CurrentTask, &XB_BOARD_DefTask); 
}
#pragma endregion
#pragma region FUNKCJE_TASK
// Procedura wywo�ywana przez g��wne zadanie frameworka
// ! W tej procedurze nast�puje odliczanie sekundowe czasu, blinkowanie LIFE ledem (je�li zdefiniowany),
// ! wygaszanie led�w od blink RX blink TX (je�li zdefiniowane), 
void TXB_board::handle(void)
{
	// Sprawdzenie  przy pierwszym  wejsciu do p�tli loop ilo�ci wolnej pami�ci
	if(MaximumFreeHeapInLoop == 0)
	{
		FreePSRAMInLoop = getFreePSRAM();
		MinimumFreePSRAMInLoop = FreePSRAMInLoop;
		MaximumFreePSRAMInLoop = FreePSRAMInLoop;

		//FreeHeapInLoop = getFreeHeap();
		//MaximumFreeHeapInLoop = FreeHeapInLoop;
		//MinimumFreeHeapInLoop = FreeHeapInLoop;
	}
	
#ifdef XB_GUI

	if (xb_board_ConsoleInWindow)
	{
		if (ConsoleScreen == NULL)
		{
			ConsoleScreen = new TConsoleScreen();
			
		}
	}
	else
	{
		if (ConsoleScreen != NULL)
		{
			delete (ConsoleScreen);
			ConsoleScreen = NULL;
		}
	}

#endif

	DEF_WAITMS_VAR(LOOPW);
	BEGIN_WAITMS(LOOPW, 1000)
	{
		Blink_Life();

#if defined(ARDUINO_ARCH_STM32) || defined(ESP32) || defined(__riscv64)
		DateTimeUnix++;
#endif
		
#ifdef XB_GUI
		if (xb_board_winHandle0 != NULL)
		{
			xb_board_winHandle0->RepaintDataCounter++;
		}
		else
		{
			if (xb_board_ShowGuiOnStart)
			{
				SendMessage_FunctionKeyPress(KF_F1, 0);
			}
		}
#endif
		
		CheckOld_HandleDataFrameTransport();
	}
	END_WAITMS(LOOPW);

	DEF_WAITMS_VAR(LOOPW3);
	BEGIN_WAITMS(LOOPW3, 100)
	{
		THDFT_ResponseItem* hdft_Item = HDFT_ResponseItemList;
		while (hdft_Item != NULL)
		{
			uint32_t result = PutStream(&hdft_Item->hdft, hdft_Item->ltsize, hdft_Item->TaskDefStream, hdft_Item->DestAddress);
			if (result == hdft_Item->ltsize)
			{
				DELETE_FROM_LIST_STR(HDFT_ResponseItemList, hdft_Item);
				board.free(hdft_Item);
				break;
			}
			hdft_Item = hdft_Item->Next;
		}
	}
	END_WAITMS(LOOPW3);


	//

	//--------------------------------------
	
#ifdef BOARD_LED_TX_PIN
	if (Tick_TX_BLINK != 0)
	{
		if (SysTickCount - Tick_TX_BLINK > TickEnableBlink)
		{
			Tick_TX_BLINK = 0;
#if defined(BOARD_LED_TX_STATUS_OFF)
			digitalWrite(BOARD_LED_TX_PIN, BOARD_LED_TX_STATUS_OFF);
#else
			digitalWrite(BOARD_LED_TX_PIN, LOW);
#endif
		}
	}
#endif

#ifdef BOARD_LED_RX_PIN
	if (Tick_RX_BLINK != 0)
	{
		if (SysTickCount - Tick_RX_BLINK > TickEnableBlink)
		{
			Tick_RX_BLINK = 0;
#if defined(BOARD_LED_RX_STATUS_OFF)
			digitalWrite(BOARD_LED_RX_PIN, BOARD_LED_RX_STATUS_OFF);
#else
			digitalWrite(BOARD_LED_RX_PIN, LOW);
#endif
		}
	}
#endif
	//--------------------------------------
//	DEF_WAITMS_VAR(LOOPW2);
//	BEGIN_WAITMS(LOOPW2, 5)
	{
		TTask *t = TaskList;
		while (t != NULL)
		{
			if ((t->CountGetStreamAddressAsKeyboard > 0) && (t->GetStreamAddressAsKeyboard!=NULL))
			{
				uint32_t res = 0;
				for (uint32_t l = 0; l < t->CountGetStreamAddressAsKeyboard; l++)
				{
					uint8_t bufkey[7] = { 0, 0, 0, 0, 0, 0, 0 };
					res = GetStream(bufkey, 7, t->TaskDef, t->GetStreamAddressAsKeyboard[l]);
					if (res > 0)
					{
						for (uint32_t i = 0; i < res; i++)
						{
							//String s = "Code:" + String(bufkey[i], HEX) + " , " + String(bufkey[i], DEC);
							//board.Log(s.c_str(), true, true);
							SendMessage_KeyPress((char)bufkey[i],NULL,t->TaskDef);
						}
						break;
					}
				}
				if (res > 0) break;
			}
			t = t->Next;
		}
	}
//	END_WAITMS(LOOPW2)
		
	//--------------------------------------
	if(Tick_ESCKey != 0)
	{
		if (SysTickCount - Tick_ESCKey > 200)
		{
			Tick_ESCKey = 0;
			TerminalFunction = 0;
			SendMessage_FunctionKeyPress(KF_ESC, 0, &XB_BOARD_DefTask);
		}
	}

}
// ------------------------------------
// Dodanie do frameworka nowego zadania
// -> Ataskdef - wska�nik definicji zadania
// -> DeviceID - 8 bajtowy numer ID urz�dzenia na kt�rym zadanie zosta�o uruchomione
// <- Wska�nik utworzonej struktury opisuj�cej zadanie
TTask *TXB_board::AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID)
{
	TTask *ta = TaskList;
	while (ta != NULL)
	{
		if (Ataskdef == ta->TaskDef)
		{
			String taskname = "";
			SendMessage_GetTaskNameString(Ataskdef, &taskname);
			Log(String("The task [" + taskname + "] has already been added...").c_str(), true, true, tlWarn);
			return ta;
		}
		ta = ta->Next;
	}

	TTask *t = (TTask *)board._malloc(sizeof(TTask));
	if (t != NULL)
	{
		ADD_TO_LIST_STR(TaskList, TTask, t);

		t->TaskDef = Ataskdef;
		t->TaskDef->Task = t;

		t->ShowLogInfo = Default_ShowLogInfo;
		t->ShowLogWarn = Default_ShowLogWarn;
		t->ShowLogError = Default_ShowLogError;
		if (ADeviceID != 0)
		{
			t->DeviceID.ID.ID64 = ADeviceID;
		}
		else
		{
			t->DeviceID.ID.ID64 = board.DeviceID.ID.ID64;
		}


		//TaskCount++;

		if (Ataskdef->dosetup != NULL)
		{
			if (t->dosetupRC == 0)
			{
				setup_procedure = true;
				t->dosetupRC++;
				TTask *lastcurrenttask = CurrentTask;
				CurrentTask = t;
				Ataskdef->dosetup();
				CurrentTask = lastcurrenttask;
				t->dosetupRC--;
				setup_procedure = false;
			}
		}
		return t;
	}
	return NULL;
}
//------------------------------------------
// Usuni�cie z frameworka wskazanego zadania
// -> Ataskdef - wska�nik definicji usuwanego zadania
// <- true OK
//    false B��d
bool TXB_board::DelTask(TTaskDef *Ataskdef)
{
	if (Ataskdef != NULL)
	{
		if (Ataskdef->Task != NULL)
		{
			TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
			mb.IDMessage = IM_DELTASK;
			DoMessage(&mb, true, CurrentTask, Ataskdef);
			DELETE_FROM_LIST_STR(TaskList, Ataskdef->Task);
			board.free(Ataskdef->Task->GetStreamAddressAsKeyboard);
			board.free(Ataskdef->Task);
			Ataskdef->Task = NULL;
			//TaskCount--;
			return true;
		}
	}
	return false;
}
// ---------------------------------------------------------------------------------------------------------------

void TXB_board::DoLoopTask(TTask* t)
{
	if (t == NULL) return;
	if (t->TaskDef == NULL) return;
	t->doloopRC++;
	{
		CurrentTask = t;
		t->TickWaitLoop = t->TaskDef->doloop();
		t->TickReturn = SysTickCount;
		CurrentTask = XB_BOARD_DefTask.Task;
	}
	t->doloopRC--;
}

// ---------------------------------------------------------------------------------------------------------------
// G��wna procedura uruchamiaj�ca zadania z podzia�em na priorytety, zg�osze� z przerwa� i czekaj�cych zadany czas
void TXB_board::IterateTask()
{
	
	static TTask* CurrentWaitLoopTask = TaskList;
	static TTask* CurrentIterateTaskPriority = TaskList;
	static TTask* CurrentIterateTaskRealTime = TaskList;

	iteratetask_procedure = true;
	{
		// Sprawdzenie czy uruchomi� przerwanie
		if(doAllInterruptRC > 0)
		{
			doAllInterruptRC--;

			TTask* t = TaskList;
			bool isint = false;
			while (t != NULL)
			{
				if (t->TaskDef->dointerrupt != NULL)
				{
					if (t->dointerruptRC > 0)
					{
						
						CurrentTask = t;
						DoInterrupt(t->TaskDef);
						CurrentTask = XB_BOARD_DefTask.Task;
						isint = true;
						
					}
				}
				t = t->Next;
			}

			if (isint) 
			{
				iteratetask_procedure = false;
				return;
			}
		}

		// ----------------------------------------------------------
		// zapami�tanie ilo�ci wolnej pami�ci ram i minimalnego stanu
		DEF_WAITMS_VAR(GFH);
		BEGIN_WAITMS(GFH, 500);
		{
#ifdef ESP32
			{
				FreePSRAMInLoop = getFreePSRAM();
				if (FreePSRAMInLoop < MinimumFreePSRAMInLoop)
				{
					MinimumFreePSRAMInLoop = FreePSRAMInLoop;
				}

				FreeHeapInLoop = getFreeHeap();
				if (FreeHeapInLoop < MinimumFreeHeapInLoop)
				{
					MinimumFreeHeapInLoop = FreeHeapInLoop;
				}

				FreeMaxAllocInLoop = getMaxAllocHeap();
				if (FreeMaxAllocInLoop < MinimumFreeMaxAllocInLoop)
				{
					MinimumFreeMaxAllocInLoop = FreeMaxAllocInLoop;
				}

				if (AutoCheckHeapIntegrity)
				{
					if (!heap_caps_check_integrity_all(true))
					{
						heap_caps_dump_all();
					}
				}
			}
#else
			FreeHeapInLoop = getFreeHeap();
			if (FreeHeapInLoop < MinimumFreeHeapInLoop)
			{
				MinimumFreeHeapInLoop = FreeHeapInLoop;
			}
#endif

		}
		END_WAITMS(GFH);
		//--------------------------------------


		//--------------------------------------
		// Sprawdzenie czy w kt�rym� zadaniu min�� czas na uruchomienie Loop()
		{
			if (CurrentWaitLoopTask == NULL) CurrentWaitLoopTask = TaskList;
			while (CurrentWaitLoopTask != NULL)
			{
				if (CurrentWaitLoopTask->TaskDef->doloop != NULL)
				{
					if (CurrentWaitLoopTask->TickWaitLoop != 0)
					{
						if (SysTickCount - CurrentWaitLoopTask->TickReturn >= CurrentWaitLoopTask->TickWaitLoop)
						{
							DoLoopTask(CurrentWaitLoopTask);
							CurrentWaitLoopTask = CurrentWaitLoopTask->Next;
							//break;
							iteratetask_procedure = false;
							return;
						}
					}
				}
				CurrentWaitLoopTask = CurrentWaitLoopTask->Next;
			}

		}
		//--------------------------------------

		//--------------------------------------
		// Uruchomienie zada� tzw realtime
		{
			if (CurrentIterateTaskRealTime == NULL)
			{
				CurrentIterateTaskRealTime = TaskList;
			}
			else
			{
				while (CurrentIterateTaskRealTime != NULL)
				{
					if (CurrentIterateTaskRealTime->TaskDef->doloop != NULL)
					{
						if (CurrentIterateTaskRealTime->TaskDef->Priority == 0)
						{
							if (CurrentIterateTaskRealTime->TickWaitLoop == 0)
							{
								DoLoopTask(CurrentIterateTaskRealTime);
								CurrentIterateTaskRealTime = CurrentIterateTaskRealTime->Next;
								iteratetask_procedure = false;
								return;
							}
						}
					}
					CurrentIterateTaskRealTime = CurrentIterateTaskRealTime->Next;
				}
			}
		}
		//--------------------------------------

		//--------------------------------------
		// Uruchomienie zada� z podzia�em na priorytety
		{
			if (CurrentIterateTaskPriority == NULL) CurrentIterateTaskPriority = TaskList;
		
			while (CurrentIterateTaskPriority != NULL)
			{
				if (CurrentIterateTaskPriority->TaskDef->doloop != NULL)
				{
					if (CurrentIterateTaskPriority->TaskDef->Priority > 0)
					{
						if (CurrentIterateTaskPriority->TickWaitLoop == 0)
						{
							CurrentIterateTaskPriority->CounterPriority++;
							if (CurrentIterateTaskPriority->CounterPriority >= CurrentIterateTaskPriority->TaskDef->Priority)
							{
			
								CurrentIterateTaskPriority->CounterPriority = 0;
								DoLoopTask(CurrentIterateTaskPriority);
								CurrentIterateTaskPriority = CurrentIterateTaskPriority->Next;
								//break;
								iteratetask_procedure = false;
								return;
							}
						}
					}
				}
				CurrentIterateTaskPriority = CurrentIterateTaskPriority->Next;
			}
		}
		//--------------------------------------
	}

	iteratetask_procedure = false;
}

// -----------------------------------------------------------
// Pobranie adresu struktury opisuj�cej zadanie wed�ug indeksu
// -> Aindex - indeks zadania weg�ug kolejno�ci dodania do frameworka
// <- wska�nik zadania
TTask *TXB_board::GetTaskByIndex(uint8_t Aindex)
{
	TTask *t = TaskList;
	uint8_t i = 0;
	while (t != NULL)
	{
		if (i == Aindex) return t;
		i++;
		t = t->Next;
	}
	return NULL;
}
// ---------------------------------------------------------
// Pobranie adresu struktury opisuj�cej zadanie wed�ug nazwy
// -> ATaskName - Nazwa zadania 
// <- wska�nik definicji zadania
TTaskDef *TXB_board::GetTaskDefByName(String ATaskName)
{
	TTask *t = TaskList;
	String tmpstr;
	tmpstr.reserve(32);
	
	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (SendMessage_GetTaskNameString(t->TaskDef, &tmpstr))
			{
				if (tmpstr == ATaskName) return t->TaskDef;
			}
		}
		t = t->Next;
	}
	return NULL;
}
// ---------------------------------------------------------
// Pobranie adresu struktura zadania wed�ug nazwy
// -> ATaskName - Nazwa zadania 
// <- wska�nik definicji zadania
TTask* TXB_board::GetTaskByName(String ATaskName)
{
	TTask* t = TaskList;
	String tmpstr;
	tmpstr.reserve(32);

	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (SendMessage_GetTaskNameString(t->TaskDef, &tmpstr))
			{
				if (tmpstr == ATaskName) return t;
			}
		}
		t = t->Next;
	}
	return NULL;
}
// ------------------------------------------------------------------------------------
// Wyzwolenie przy nast�pnej iteracji procedury z definicji zadania do obs�ugi przerwa�
// -> Wska�nik definicji zadania kt�rego ma zosta� uruchomiona procedura interrupt
void TXB_board::TriggerInterrupt(TTaskDef *Ataskdef)
{
	if (Ataskdef != NULL)
	{
		if (Ataskdef->Task != NULL)
		{
			Ataskdef->Task->dointerruptRC++;
			doAllInterruptRC++;
		}
	}
}
// ---------------------------------------------------------
// Wykonanie procedure obs�ugi przerwania wskazanego zadania
// -> Wska�nik definicji zadania kt�rego ma zosta� uruchomiona procedura interrupt
void TXB_board::DoInterrupt(TTaskDef *Ataskdef)
{
	if (Ataskdef != NULL)
	{
		if (Ataskdef->dointerrupt != NULL)
		{
			Ataskdef->dointerrupt();	
		}

		if (Ataskdef->Task != NULL)
		{
			Ataskdef->Task->dointerruptRC--;
			if (Ataskdef->Task->dointerruptRC < 0)
			{
				Ataskdef->Task->dointerruptRC = 0;
				Log("Error trigger interrupt status.", true, true, tlError);
			}
		}
	}
}
// ---------------------------------------------------------
// Anulowanie pozosta�ego czasu postoju zadania
void TXB_board::CancelWaitTask()
{
	CancelWaitTask(CurrentTask);
}
// ---------------------------------------------------------
// Anulowanie pozosta�ego czasu postoju zadania
// -> Wskazanie zadania do anulowania czasu postoju
void TXB_board::CancelWaitTask(TTask* ATask)
{
	if (ATask != NULL)
	{
		ATask->TickWaitLoop = 0;
		ATask->TickReturn = 0;
	}

}


#pragma endregion
#pragma region FUNKCJE_MESSAGOW
// -----------------------------------------------------
// Wykonanie procedury obs�ugi messaga wybranego zadania
// -> mb - Wska�nik struktury opisuj�cej messaga
// -> Arunagain  - Je�li true to wywo�anie procedury messaga z mo�liwo�ci� wykonania ponownie
//               - Je�li false nie wykona si� procedura messaga odwo�a si� poraz kolejny 
//                 Argument s�u�y do nikni�cia ewentualnego zap�tlenia odwo�a� messag�w.
// -> Afromtask  - Zadania od kt�rego wysy�any jest messag
// -> Atotaskdef - Definicja zadania do kt�rego messag jest wysy�any
bool TXB_board::DoMessage(TMessageBoard *mb, bool Arunagain, TTask *Afromtask, TTaskDef *Atotaskdef)
{
	bool res = false;

	if (Atotaskdef != NULL)
	{
		if (Atotaskdef->Task != NULL)
		{
			if (Atotaskdef->domessage != NULL)
			{
				if ((Atotaskdef->Task->domessageRC == 0) || (Arunagain == true))
				{
					Atotaskdef->Task->domessageRC++;
					{
						TTask *lt = CurrentTask;
						CurrentTask = Atotaskdef->Task;
						mb->fromTask = Afromtask;
						res = Atotaskdef->domessage(mb);
						CurrentTask = lt;
					}
					Atotaskdef->Task->domessageRC--;
				}
			}
		}
	}
	return res;
}
//---------------------------------------------------------
// Wykonanie procedury obs�ugi messaga dla wszystkich zada�
// -> mb - Wska�nik struktury opisuj�cej messaga
// -> Arunagain    - Je�li true to wywo�anie procedury messaga z mo�liwo�ci� wykonania ponownie
//                 - Je�li false nie wykona si� procedura messaga odwo�a si� poraz kolejny 
//                   Argument s�u�y do nikni�cia ewentualnego zap�tlenia odwo�a� messag�w.
// -> Afromtask    - Zadania od kt�rego wysy�any jest messag
// -> Aexcludetask - Wska�nik definicji zadania kt�re ma zosta� wykluczone z ob��ugi messaga
bool TXB_board::DoMessageOnAllTask(TMessageBoard *mb, bool Arunagain, TDoMessageDirection ADoMessageDirection, TTask *Afromtask, TTaskDef *Aexcludetask)
{
	uint32_t res = 0;
	
	if (Afromtask == NULL)
	{
		Afromtask = CurrentTask;
	}
	
	switch (ADoMessageDirection)
	{
	case doFORWARD:
		{
			TTask *t = TaskList;
			while (t != NULL)
			{
				if (t->TaskDef != NULL)
				{
					if (Aexcludetask != t->TaskDef)
					{
						res += DoMessage(mb, Arunagain, Afromtask, t->TaskDef);
					}
				}
				t = t->Next;
			}
			break;
		}

	case doBACKWARD:
		{
			TTask *t = TaskList;
			while (t != NULL) 
			{ 
				if (t->Next == NULL) break;
				t = t->Next; 
			}

			while (t != NULL)
			{
				if (t->TaskDef != NULL)
				{
					if (Aexcludetask != t->TaskDef)
					{
						res += DoMessage(mb, Arunagain,  Afromtask, t->TaskDef);
					}
				}
				t = t->Prev;
			}
			break;
		}
	default: break;
	}
	return (res > 0);
}
//---------------------------------------------------------------
// Wykonanie procedury obs�ugi messaga zadania o wskazanej nazwie
// -> Ataskname    - Nazwa zadania
// -> mb           - Wska�nik struktury opisuj�cej messaga
// -> Arunagain    - Je�li true to wywo�anie procedury messaga z mo�liwo�ci� wykonania ponownie
//                 - Je�li false nie wykona si� procedura messaga odwo�a si� poraz kolejny 
//                   Argument s�u�y do nikni�cia ewentualnego zap�tlenia odwo�a� messag�w.
bool TXB_board::DoMessageByTaskName(String Ataskname,TMessageBoard *mb, bool Arunagain)
{
	TTask *t = TaskList;
	String tn;

	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (t->TaskDef->domessage)
			{
				tn = "";
				if (SendMessage_GetTaskNameString(t->TaskDef, &tn))
				{
					if (Ataskname == tn)
					{
						return DoMessage(mb, Arunagain, NULL, t->TaskDef);
					}
				}
			}
		}
		t = t->Next;
	}
	return false;
}
// ----------------------------------------------------------------
// Wys�anie messaga do zadania w celu uzyskania stringu statusowego
// -> ATaskDef       - Definicja zadania od kt�rego uzyskamy string status
// -> APointerString - Referencja klasy String w kt�rym znajdzie si� status
// <- True           - wykonano poprawnie procedure pobrania status stringu
//    False          - Zadanie nie obs�uguje messaga 
bool TXB_board::SendMessage_GetTaskStatusString(TTaskDef *ATaskDef, String *APointerString)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKSTATUS_STRING;
	mb.Data.PointerString = APointerString;
	bool res = DoMessage(&mb, true, CurrentTask, ATaskDef);
	*mb.Data.PointerString += "                   ";
	return res;
}
// --------------------------------------------------------------------
// Wys�anie messaga do zadania w celu uzyskania stringu z nazw� zadania
// -> ATaskDef       - Definicja zadania od kt�rego uzyskamy string z nazw�
// -> APointerString - Referencja klasy String w kt�rym znajdzie si� nazwa
// <- True           - wykonano poprawnie procedure pobrania nazwy zadania
//    False          - Zadanie nie obs�uguje messaga 
bool TXB_board::SendMessage_GetTaskNameString(TTaskDef *ATaskDef,String *APointerString)
{
	if (ATaskDef == NULL) return false;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKNAME_STRING;
	mb.Data.PointerString = APointerString;
	bool res = DoMessage(&mb, true, CurrentTask, ATaskDef);
	if (APointerString->length() > BOARD_TASKNAME_MAXLENGTH)
	{
		*APointerString = APointerString->substring(0, BOARD_TASKNAME_MAXLENGTH);
	}
	return res;
}
// --------------------------------------------------------------------
// Wys�anie messaga do zadania w celu uzyskania stringu z nazw� zadania
// -> ATask          - Zadanie od kt�rego uzyskamy string z nazw�
// -> APointerString - Referencja klasy String w kt�rym znajdzie si� nazwa
// <- True           - wykonano poprawnie procedure pobrania nazwy zadania
//    False          - Zadanie nie obs�uguje messaga 
bool TXB_board::SendMessage_GetTaskNameString(TTask* ATask, String *APointerString)
{
	return SendMessage_GetTaskNameString(ATask->TaskDef, APointerString);
}
// -----------------------------------------------------------------------------
// Wys�anie messaga informuj�cego zadanie �e rozpocznie si� procedura OTA Update
void TXB_board::SendMessage_OTAUpdateStarted()
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_OTA_UPDATE_STARTED;
	mb.Data.uData64 = 0;
	DoMessageOnAllTask(&mb, true, doBACKWARD); 
}

void TXB_board::SendMessage_FunctionKeyPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Aexcludetask, TTaskDef* Afromstreamtask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	mb.Data.KeyboardData.FromStreamTask = Afromstreamtask;
	DoMessageOnAllTask(&mb, true, doFORWARD, NULL, Aexcludetask);  
} 

void TXB_board::SendMessage_KeyPress(char Akey,TTaskDef *Aexcludetask, TTaskDef* Afromstreamtask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	mb.Data.KeyboardData.FromStreamTask = Afromstreamtask;
	DoMessageOnAllTask(&mb, true, doFORWARD, NULL, Aexcludetask);  
}

void TXB_board::SendMessage_FREEPTR(void *Aptr)
{
	if (Aptr != NULL)
	{
		TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
		mb.IDMessage = IM_HANDLEPTR;
		mb.Data.HandlePTRData.TypeHandlePTRAction = thpaFreePTR;
		mb.Data.HandlePTRData.FreePTR = Aptr;
		DoMessageOnAllTask(&mb, true, doBACKWARD);
	}
}

void TXB_board::SendMessage_REALLOCPTR(void* Aoldptr, void* Anewptr)
{
	if (Aoldptr != NULL)
	{
		TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
		mb.IDMessage = IM_HANDLEPTR;
		mb.Data.HandlePTRData.TypeHandlePTRAction = thpaReallocPTR;
		mb.Data.HandlePTRData.OldPTR = Aoldptr;
		mb.Data.HandlePTRData.NewPTR = Anewptr;
		DoMessageOnAllTask(&mb, true, doBACKWARD);
	}
}

void TXB_board::SendMessage_RTCSYNC()
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_RTCSYNC;
	DoMessageOnAllTask(&mb, true, doFORWARD);
}

int TXB_board::SendMessage_GetVarCount(TTask *Atask)
{
	String varname;
	String varvalue;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_VAR;
	mb.Data.VarData.Action = vdaCountVar;
	mb.Data.VarData.VarName = &varname;
	mb.Data.VarData.VarValue = &varvalue;
	bool result = DoMessage(&mb, true, NULL,Atask->TaskDef);
	return  mb.Data.VarData.CountVar;
}

String TXB_board::SendMessage_GetVarName(int AindxVar,TTask* Atask)
{
	String varname;
	String varvalue;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_VAR;
	mb.Data.VarData.Action = vdaGetVarName;
	mb.Data.VarData.VarName = &varname;
	mb.Data.VarData.VarValue = &varvalue;
	mb.Data.VarData.IndxVar = AindxVar + 1;
	bool result = DoMessage(&mb, true, NULL, Atask->TaskDef);
	if (result)
	{
		return varname;
	}
	else
	{
		return "";
	}
}

String TXB_board::SendMessage_GetVarValue(String Avarname, TTask* Atask)
{
	
	String varvalue;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_VAR;
	mb.Data.VarData.Action = vdaGetValue;
	mb.Data.VarData.VarName = &Avarname;
	mb.Data.VarData.VarValue = &varvalue;
	bool result = DoMessage(&mb, true, NULL, Atask->TaskDef);
	if (result)
	{
		return varvalue;
	}
	else
	{
		return "";
	}
}

String TXB_board::SendMessage_GetVarDescription(String Avarname, TTask* Atask)
{

	String varvalue;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_VAR;
	mb.Data.VarData.Action = vdaGetVarDesc;
	mb.Data.VarData.VarName = &Avarname;
	mb.Data.VarData.VarValue = &varvalue;
	bool result = DoMessage(&mb, true, NULL, Atask->TaskDef);
	if (result)
	{
		return varvalue;
	}
	else
	{
		return "";
	}
}

#pragma endregion
#pragma region FUNKCJE_OBSLUGI_RAM
// -------------------------------------
// Zwr�cenie ilo�ci wolnej pami�ci PSRAM
// <- Ilo�� bajt�w
#ifdef ARDUINO_ARCH_STM32
extern "C" void * _sbrk(int);
extern uint32_t _estack;
#endif

uint32_t TXB_board::getFreePSRAM()
{
#if defined(BOARD_HAS_PSRAM) && defined(ESP32)
	uint32_t freepsram = ESP.getFreePsram();
	return freepsram;
#else
#ifdef __riscv64
	return get_free_heap_size();
#endif
#ifdef ARDUINO_ARCH_STM32
	return getFreeHeap();
#endif
#if defined(ESP32) || defined(ESP8266)
	return ESP.getFreeHeap();
#endif
#endif
}
// -------------------------------------
// Zwr�cenie ilo�ci wolnej pami�ci RAM
// <- Ilo�� bajt�w
uint32_t TXB_board::getFreeHeap()
{
#if defined(ESP8266) || defined(ESP32)
	return ESP.getFreeHeap();
#endif

#ifdef __riscv64
	return get_free_heap_size();
#endif

#ifdef ARDUINO_ARCH_STM32
	{
		uint8_t v = 0;
		char * heap_end = (char *)_sbrk(0);
		return ((uint32_t)&v) - ((uint32_t)heap_end);
	}
#endif
}

// -------------------------------------
// Zwr�cenie ilo�ci maksymalnie wielkiego bloku wolnej pami�ci RAM
// <- Ilo�� bajt�w
uint32_t TXB_board::getMaxAllocHeap()
{
#if defined(ESP8266) || defined(ESP32)
	return ESP.getMaxAllocHeap();
#endif

#ifdef __riscv64
	return get_free_heap_size();
#endif

#ifdef ARDUINO_ARCH_STM32
	{
		uint8_t v = 0;
		char* heap_end = (char*)_sbrk(0);
		return ((uint32_t)&v) - ((uint32_t)heap_end);
	}
#endif
}


void __HandlePTR(void** Aptr, TMessageBoard* Am)
{
	if (Am->Data.HandlePTRData.TypeHandlePTRAction == thpaFreePTR) \
	{ 
		if (Am->Data.HandlePTRData.FreePTR == *Aptr) *Aptr = NULL; \
	} 
	else if (Am->Data.HandlePTRData.TypeHandlePTRAction == thpaReallocPTR) \
	{ 
		if (Am->Data.HandlePTRData.OldPTR == *Aptr) *Aptr = Am->Data.HandlePTRData.NewPTR; \
	} 
}

#ifdef __cplusplus
extern "C" {
#endif

	int OurReservedBlock;
	int psraminited;

	void ___free(void* Aptr)
	{
		if (!heap_caps_check_integrity_all(true))
		{
			heap_caps_dump_all();
		}
#if !defined(_VMICRO_INTELLISENSE)
		free(Aptr);
#endif
		if (Aptr != NULL)
		{
			OurReservedBlock--;
		}
	}

	void* ___malloc(size_t Asize)
	{
		if (board.AutoCheckHeapIntegrity)
		{
			if (!heap_caps_check_integrity_all(true))
			{
				heap_caps_dump_all();
			}
		}

#if !defined(_VMICRO_INTELLISENSE)
		void* ptr = malloc(Asize);
		if (ptr != NULL)
		{
			OurReservedBlock++;
			for (uint32_t i = 0; i < Asize; i++) ((uint8_t*)ptr)[i] = 0;
		}
		return ptr;
#endif
	}

	void* ___malloc_psram(size_t Asize)
	{
		void* ptr = nullptr;

		if (board.AutoCheckHeapIntegrity)
		{
			if (!heap_caps_check_integrity_all(true))
			{
				heap_caps_dump_all();
			}
		}

#if !defined(_VMICRO_INTELLISENSE)

#ifdef BOARD_HAS_PSRAM

		//bool b = psramFound();
		//if (!b)
		//{
		//	b = psramInit();
		//	psraminited++;
		//}
		//if ((b) || (psraminited > 0))
		{
			ptr = ps_malloc(Asize);
			//heap_caps_malloc(Asize, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
			if (ptr == NULL)
			{
				ptr = malloc(Asize);
			}
		}
		//else
		//	ptr = malloc(Asize);
#else
		ptr = malloc(Asize);
#endif

		if (ptr != NULL)
		{
			OurReservedBlock++;
			for (uint32_t i = 0; i < Asize; i++) ((uint8_t*)ptr)[i] = 0;
		}
		return ptr;
#endif
	}

	void* ___realloc(void* Aptr, size_t Asize)
	{
		if (board.AutoCheckHeapIntegrity)
		{
			if (!heap_caps_check_integrity_all(true))
			{
				heap_caps_dump_all();
			}
		}

#if !defined(_VMICRO_INTELLISENSE)
		void* ptr = realloc(Aptr, Asize);
		if (Aptr == NULL) OurReservedBlock++;
		return ptr;
#endif
	}

	void* ___realloc_psram(void* Aptr, size_t Asize)
	{
		void* ptr;

		if (board.AutoCheckHeapIntegrity)
		{
			if (!heap_caps_check_integrity_all(true))
			{
				heap_caps_dump_all();
			}
		}

#if !defined(_VMICRO_INTELLISENSE)


#ifdef BOARD_HAS_PSRAM
		//bool b = psramFound();
		//if (!b)
		//{
		//	b = psramInit();
		//	psraminited++;
		//}
		//if ((b) || (psraminited > 0))
			ptr = ps_realloc(Aptr, Asize); 
			//heap_caps_realloc( Aptr,Asize, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);	
		//else
		//	ptr = realloc(Aptr, Asize);
#else
		ptr = realloc(Aptr, Asize);
#endif
		if (Aptr == NULL) OurReservedBlock++;
		return ptr;
#endif
	}
#ifdef __cplusplus
}
#endif
//------------------------------------------------------------------------------------------------------------------------------
// Rezerwacja pami�ci SPI RAM, je�li p�ytka nie udost�pnia takiego rodzaju pami�ci to nast�pi przydzielenie z podstawowej sterty
// -> Asize - Ilo�� rezerwowanej pami�ci PSRAM
// <- Wska�nik zarezerwowanego bloku pami�ci PSRAM
void *TXB_board::_malloc_psram(size_t Asize)
{
	
	void* ptr = ___malloc_psram(Asize);
	return ptr;
}
//------------------------------------------------------------------------------------------------------------------------------
// Rezerwacja pami�ci SPI RAM, je�li p�ytka nie udost�pnia takiego rodzaju pami�ci to nast�pi przydzielenie z podstawowej sterty
// -> Asize - Ilo�� rezerwowanej pami�ci PSRAM
// <- Wska�nik zarezerwowanego bloku pami�ci PSRAM
void* TXB_board::_realloc_psram(void *Aptr,size_t Asize)
{
	void* ptr = ___realloc_psram(Aptr,Asize);

	if (ptr != NULL)
	{
		if (ptr != Aptr)
		{
			board.SendMessage_REALLOCPTR(Aptr, ptr);
		}
	}
	return ptr;
}
//---------------------------------------
// Rezerwacja pami�ci RAM mikrokontrolera
// -> Asize - Ilo�� rezerwowanej pami�ci 
// <- Wska�nik zarezerwowanego bloku pami�ci 
void *TXB_board::_malloc(size_t size)
{
	void* ptr = ___malloc(size);
	return ptr;
}
// ------------------
// Zwolnienie pami�ci
// -> Aptr wska�nik na blok pami�ci zwalnianej
// ! przed zwolnieniem pami�ci zostaje wys�any messag do wszystkich zada� informuj�cy �e wskazany blok pami�ci zostaje zwolniony
void TXB_board::free(void *Aptr)
{
	if (Aptr != NULL)
	{
		SendMessage_FREEPTR(Aptr);
		___free(Aptr);
	}
}

// --------------------------------------------------------------------------------------------------------------------
// Zwolnienie pami�ci z pod wskazanego (wska�niniem) wska�nika i nast�pnie nadanie warto��i NULL wska�nikowi wskazanemu
//  -> Wska�nik wska�nika zwalnianego bloku pami�ci
void TXB_board::freeandnull(void **Aptr)
{
	if (*Aptr != NULL)
	{
		board.free(*Aptr);
		*Aptr = NULL;
	}
}

#pragma endregion
#pragma region FUNKCJE_STREAMU_FRAMES
//-----------------------------------------------------
// Odczyt ze wskazanego streamu okre�lon� ilo�� bajt�w
// -> Adata          - wska�nik na blok pami�ci gdzie maj� zosta� zapisane odczytane bajty
// -> Amaxlength     - wielko�� maksymalna bloku pami�ci i r�wnocze�nie maksymalna ilo�� bajt�w do odczytu
// -> AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// -> Afromaaddress	 - adres / kana� z kt�rego b�dzi� czytany stream
//                   ! Je�li zero to stream powinien udost�pni� dane pierwsze w kolejce do odczytu z dowolnego adresu
// <- ilo�� bajt�w odczytanych
uint32_t TXB_board::GetStream(void *Adata, uint32_t Amaxlength, TTaskDef *AStreamtaskdef, uint32_t Afromaddress)
{
	if (Amaxlength == 0) return 0;
	if (Adata == NULL) return 0;
	if (AStreamtaskdef == NULL) return 0;
	if (AStreamtaskdef->Task == NULL) return 0;
	
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saGet;
	mb.Data.StreamData.Data = Adata;
	mb.Data.StreamData.Length = Amaxlength;
	mb.Data.StreamData.FromAddress = Afromaddress;
	
	THandleDataFrameTransport *hdft = AStreamtaskdef->Task->HandleDataFrameTransportList;
	
	while (hdft != NULL)
	{
		if (hdft->isdatatoread)
		{
			if (GetFromErrFrameTransport(&mb, hdft))
			{
				hdft->TickCreate = SysTickCount;
				return mb.Data.StreamData.LengthResult;
			}
		}

		hdft = hdft->Next;
	}
	if (DoMessage(&mb, true, NULL, AStreamtaskdef))
	{
		hdft = AddToTask_HandleDataFrameTransport(AStreamtaskdef, mb.Data.StreamData.FromAddress);
		if (hdft != NULL) 
		{
			HandleDataFrameTransport(&mb, hdft, AStreamtaskdef);
		}
		return mb.Data.StreamData.LengthResult;
	}
	CheckOld_HandleDataFrameTransport(AStreamtaskdef->Task);
	return 0;
}
// ------------------------------------------------------------
// Wys�anie bloku pami�ci wskazanym streamem pod wskazany adres
// -> Adata          - wska�nik na blok pami�ci do wys�ania
// -> Alength        - wielko�� w bajtach bloku pami�ci do wys�ania
// -> AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// -> AToAddress     - Adres pod kt�ry zostanie wys�any wskazany blok pami�ci
// <- Ilo�� bajt�w ile zdo�ano wys�a� wskazanym streamem
uint32_t TXB_board::PutStream(void *Adata, uint32_t Alength, TTaskDef *AStreamtaskdef, uint32_t AToAddress)
{
	if (Adata == NULL) return 0;
	if (AStreamtaskdef == NULL) return 0;
	if (Alength == 0) 
	{
		Alength = StringLength((const char *)Adata, 0);
	}
	if (Alength == 0) 	return 0;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saPut;
	mb.Data.StreamData.Data = Adata;
	mb.Data.StreamData.Length = Alength;
	mb.Data.StreamData.LengthResult = 0;
	mb.Data.StreamData.ToAddress = AToAddress;
	
	if (DoMessage(&mb, true, NULL, AStreamtaskdef))
	{
		return mb.Data.StreamData.LengthResult;
	}
	return 0;
}
// ---------------------------------------------------------------------------------------------------------
// Sprawdzenie czy wskazane zadanie obs�uguje stream
// -> *AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
bool TXB_board::IsTaskStream(TTaskDef* AStreamTaskDef)
{
	if (AStreamTaskDef == NULL) return false;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saTaskStream;
	return DoMessage(&mb, true, NULL, AStreamTaskDef);
}
// ---------------------------------------------------------------------------------------------------------
// Poinformowanie zadania obs�uguj�cego stream, �e cykliczny odczyt streamu zostanie przej�ty przez inne zadanie
// -> AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// -> AToAddress     - Adres/Kana�/Numer urz�dzenia kt�rego b�dzie obs�ugiwany odczyt streamu
void TXB_board::BeginUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress)
{

	if (AStreamtaskdef == NULL) return;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saBeginUseGet;
	mb.Data.StreamData.ToAddress = AToAddress;
	
	DoMessage(&mb, true, NULL, AStreamtaskdef);
	return;
}
// ---------------------------------------------------------------------------------------------------------
// Poinformowanie zadania obs�uguj�cego stream, �e cykliczny odczyt streamu ma zosta� 
// przekazany z powrotem do zadania ob��ugij�cego stream
// -> AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// -> AToAddress     - Adres/Kana�/Numer urz�dzenia kt�rego oddana ma zosta� obs�uga zadania od straemu
void TXB_board::EndUseGetStream(TTaskDef *AStreamtaskdef, uint32_t AToAddress)
{
	if (AStreamtaskdef == NULL) return;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saEndUseGet;
	mb.Data.StreamData.ToAddress = AToAddress;
	
	DoMessage(&mb, true, NULL, AStreamtaskdef);
	return;
}
// ---------------------------------------------------------------------------------------------------------
// Zapytanie zadania streamu o udost�pnienie lokalnego adresu
// -> *AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// -> *Alocaladdress -  wska�nik na warto�� zwr�con� czyli lokalny adres
bool TXB_board::GetStreamLocalAddress(TTaskDef* AStreamTaskDef, uint32_t* Alocaladdress)
{
	if (AStreamTaskDef == NULL) return false;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saGetLocalAddress;
	mb.Data.StreamData.FromAddress = *Alocaladdress;
	mb.Data.StreamData.ToAddress = *Alocaladdress;

	if (DoMessage(&mb, true, NULL, AStreamTaskDef))
	{
		*Alocaladdress = mb.Data.StreamData.ToAddress;
		return true;
	}
	
	return false;
}
// ---------------------------------------------------------------------------------------------------------
// Wy��czenie nadawania dla podanego streamu
// -> *AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
bool TXB_board::DisableTXStream(TTaskDef* AStreamTaskDef)
{
	if (AStreamTaskDef == NULL) return false;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saDisableTX;
	return DoMessage(&mb, true, NULL, AStreamTaskDef);
}
// ---------------------------------------------------------------------------------------------------------
// W��czenie nadawania dla podanego streamu
// -> *AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
bool TXB_board::EnableTXStream(TTaskDef* AStreamTaskDef)
{
	if (AStreamTaskDef == NULL) return false;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saEnableTX;
	return DoMessage(&mb, true, NULL, AStreamTaskDef);
}
// ---------------------------------------------------------------------------------------------------------
// Sprawdzenie czy w��czone nadawanie dla podanego streamu
// -> *AStreamtaskdef - wska�nik zadania obs�uguj�cego stream
// <- True to wy��czony TX
bool TXB_board::StatusDisableTXStream(TTaskDef* AStreamTaskDef)
{
	if (AStreamTaskDef == NULL) return false;

	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_STREAM;
	mb.Data.StreamData.StreamAction = saStatusDisableTX;
	return DoMessage(&mb, true, NULL, AStreamTaskDef);
}
// ---------------------------------------------------------------------------------------------------------
// Funkcja wywo�ywana w GetStream, s�u�y do wyci�gania ze streamu ramki kt�ra zaczyna si� od 4 bajtowego ACK
// -> mb			            - Wska�nik struktury messaga GetStream 
// -> AHandleDataFrameTransport - Wska�nik na strukture opisuj�c� bufor odczytanej ramki, s�u�y do skompletowania fragment�w streamu z jednego �r�d�a
// -> ATaskDefStream			- Wska�nik definicji zadania streamu z kt�rego nadchodz� dane
// <- true                      - Wykonano poprawnie
bool TXB_board::HandleDataFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport, TTaskDef *ATaskDefStream)
{
	int32_t indx = 0;
	uint32_t indxstartinterpret = 0xffff;
	uint8_t v = 0;
	bool isininterpret = false;
	
	while (indx < mb->Data.StreamData.LengthResult)
	{
		v = ((uint8_t *)mb->Data.StreamData.Data)[indx];

		// Sprawdzenie czy nadchodz�cy bajt jest pierwszym od ACK
		if((AHandleDataFrameTransport->indx_interpret == 0) && (v == FRAME_ACK_A))
		{
			// Uruchominie interpretowania 
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			AHandleDataFrameTransport->isdataframe_interpret = true;

			isininterpret = true;
			indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz�cy bajt jest drugim od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 1) && (v == FRAME_ACK_B))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)
				if (indxstartinterpret == 0xffff) 
					indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz�cy bajt jest trzecim od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 2) && (v == FRAME_ACK_C))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)					
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz�cy bajt jest czwartym od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 3) && (v == FRAME_ACK_D))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)	
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz�cy bajt to 4 pozycja i czy ma warto�� oczekiwan� SIZE
		else if((AHandleDataFrameTransport->indx_interpret == 4) && (v <= sizeof(TFrameTransport)) && (v >= (offsetof(TFrameTransport, LengthFrame))))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)				
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx;
		}
		// Sprawdzenie czy kolejny
		else if(AHandleDataFrameTransport->indx_interpret > 4)
		{
			// zapisywanie kolejnego bajtu do bufora
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;

			// Sprawdzenie czy jest to ostatni bajt zdeklarowany na poczatku struktury ramki
			if((AHandleDataFrameTransport->indx_interpret - sizeof(TFrameTransportACK)) >= AHandleDataFrameTransport->Data.str.FT.size)
			{
				HandleFrame(&AHandleDataFrameTransport->Data.str.FT, ATaskDefStream);
				
				
				indx++;
				uint8_t *src = &((uint8_t *)mb->Data.StreamData.Data)[indx];
				uint8_t *dest;
				if (isininterpret)
				{
					if (indxstartinterpret == 0xffff) indxstartinterpret = indx; 	
					dest = &((uint8_t *)mb->Data.StreamData.Data)[indxstartinterpret];
				}
				else
				{
					dest = &((uint8_t *)mb->Data.StreamData.Data)[0];
				}
				
				int32_t len =	mb->Data.StreamData.LengthResult - indx;
				
				mb->Data.StreamData.LengthResult = mb->Data.StreamData.LengthResult - ((uint32_t)(src - dest));
				indx = indx - (uint32_t)(src - dest);
				
				while (len > 0)
				{
					dest[len - 1] = src[len - 1];
					len--;
				}
				
				AHandleDataFrameTransport->indx_interpret = 0;
				AHandleDataFrameTransport->isdataframe_interpret = false;
				isininterpret = false;
				indxstartinterpret = 0xffff;
				indx--;
			}
		}
		// Bajty si� nie zgadzaj� do wzoru nadchodz�cej ramki
		else
		{
			// Sprawdzenie czy by�o teraz rozpocz�te interpretowanie
			if(isininterpret)
			{
				// je�li by�o to le� dalej z interpretacj�
				AHandleDataFrameTransport->indx_interpret = 0;
				AHandleDataFrameTransport->isdataframe_interpret = false;
				isininterpret = false;
				indxstartinterpret = 0xffff;
			}
			else
			{
				// je�li teraz nie by�o rozpocz�cia to wstawi� co zbuforowane do streamu
				if(AHandleDataFrameTransport->isdataframe_interpret)
				{
					//AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			
					uint32_t c = AHandleDataFrameTransport->indx_interpret - indx;

					// sprawdzenie czy zmie�ci si� do tego bufora streamu
					if((mb->Data.StreamData.Length - mb->Data.StreamData.LengthResult) >= c)
					{
						int32_t indx_s = mb->Data.StreamData.LengthResult - 1;
						int32_t indx_d = mb->Data.StreamData.LengthResult - 1 + c;

						// Mo�na przesun�� to co zosta�o zapami�tane
						while(indx_s >= 0)
						{
							((uint8_t *)mb->Data.StreamData.Data)[indx_d] = ((uint8_t *)mb->Data.StreamData.Data)[indx_s];
							indx_d--;
							indx_s--;
						}

						mb->Data.StreamData.LengthResult += c;
						
						indx_s = 0;

						while (indx_s < AHandleDataFrameTransport->indx_interpret)
						{
							((uint8_t *)mb->Data.StreamData.Data)[indx_s] = AHandleDataFrameTransport->Data.buf[indx_s];
							indx_s++;
						}
					}
					else
					{
						Log("Data has been lost in the download stream [", true, true, tlError);
						{
							String tmps = "";
							SendMessage_GetTaskNameString(ATaskDefStream, &tmps);
							Log(tmps.c_str(), false, false, tlError);
						}
						Log("]. The read buffer is too little.", false, false, tlError);
					}

					// le� dalej z interpretacj�
					AHandleDataFrameTransport->indx_interpret = 0;
					AHandleDataFrameTransport->isdataframe_interpret = false;
					isininterpret = false;
					indxstartinterpret = 0xffff;
				}
			}
		}
		indx++;
	}
	// Sprawdzenie czy odnotowano jakie� interpretowanie
	if(indxstartinterpret != 0xffff)
	{
		mb->Data.StreamData.LengthResult = indxstartinterpret;
	}
	return true;
}
// --------------------------------------------------------------------------------------
// Je�li ramka zosta�a b��dnie zinterpretowana to nast�puje oddanie z powrotem do streamu 
// -> mb			            - Wska�nik struktury messaga GetStream 
// -> AHandleDataFrameTransport - Wska�nik na strukture opisuj�c� bufor odczytanej ramki, s�u�y do skompletowania fragment�w streamu z jednego �r�d�a
// <- true - Nast�pi�o oddanie danych do bufora wskazanego w streamie
//    false - Musi nast�pi� kolejne wywo�anie messaga streamu w celu odczytu danych
bool TXB_board::GetFromErrFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport)
{	
	if (AHandleDataFrameTransport != NULL)
	{
		if (AHandleDataFrameTransport->isdatatoread)
		{
			uint32_t indx = 0;
			while (indx < mb->Data.StreamData.Length)
			{
				((uint8_t *)mb->Data.StreamData.Data)[indx] = AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indxdatatoread];
				AHandleDataFrameTransport->indxdatatoread++;
				indx++;
				mb->Data.StreamData.LengthResult = indx;

				if (AHandleDataFrameTransport->indxdatatoread >= AHandleDataFrameTransport->max_indxdatatoread)
				{
					xb_memoryfill(AHandleDataFrameTransport, sizeof(THandleDataFrameTransport), 0);
					break;
				}
			}
			mb->Data.StreamData.LengthResult = indx;
			return true;
		}
	}
	return false;
}
// ------------------------------------------------------------------------------------------------------------------------------------------
// Dodanie struktury opisuj�cej bufor odczytywanej ramki, struktura jest przypisana do zadania osb�usguj�cego stream z kt�rego nadchodz� dane
// -> AStreamtaskdef - Wska�nik na definicje zadania streamu z kt�rego b�d� interpretowane ramki
// -> Afromaddress   - Adres z kt�rego nadchodz� dane do intepretowania ramki
// <- Wska�nik na strukture opisuj�c� bufor odczytanej ramki
THandleDataFrameTransport *TXB_board::AddToTask_HandleDataFrameTransport(TTaskDef *AStreamtaskdef, uint32_t Afromaddress)
{
	if (AStreamtaskdef == NULL) return NULL;
	if (AStreamtaskdef->Task == NULL) return NULL;
	
	THandleDataFrameTransport *hdft = AStreamtaskdef->Task->HandleDataFrameTransportList;
	while (hdft != NULL)
	{
		if (Afromaddress == hdft->FromAddress)
		{
			hdft->TickCreate = SysTickCount;
			return hdft;
		}
		hdft = hdft->Next;
	}

	hdft = (THandleDataFrameTransport *)board._malloc(sizeof(THandleDataFrameTransport));
	if (hdft == NULL) return NULL;
	ADD_TO_LIST_STR(AStreamtaskdef->Task->HandleDataFrameTransportList, THandleDataFrameTransport, hdft);
	hdft->FromAddress = Afromaddress;
	hdft->TickCreate = SysTickCount;
	
	return hdft;
}
// -----------------------------------------------------------------------------------------------------------------------------
// Sprawdzenie oraz usuni�cie bufora odczytywanej ramki je�li na 10 sekund zatrzyma�a si� transmisja lub je�li nie by� potrzebny
// Atask -> Wska�nik zadania kt�re ma zosta� sprawdzone
//          ! Je�li podane zostanie NULL to sprawdzenie b�dzie si� odno�ci� do wszystkich zada�
void TXB_board::CheckOld_HandleDataFrameTransport(TTask *Atask)
{

	TTask *t = TaskList;
	if (Atask != NULL)
	{
		t = Atask;
	}
	while (t != NULL)
	{
		THandleDataFrameTransport *hdft = t->HandleDataFrameTransportList;
		while (hdft != NULL)
		{
			if (SysTickCount - hdft->TickCreate > TIMEOUT_HANDLEDATAFRAMETRANSPORT)
			{
				DELETE_FROM_LIST_STR(t->HandleDataFrameTransportList, hdft);
				board.free(hdft);
				hdft = t->HandleDataFrameTransportList;
			}
			else
			{
				hdft = hdft->Next;
			}
		}
		if (Atask == NULL)
		{
			t = t->Next;	
		}
		else break;
	}
}
// ---------------------------------------------------------------
// Wys�anie ramki danych okre�lonym streamem do wskazanego zadania
// -> ASourceTaskName   - Nazwa zadania z kt�rego ramka jest wysy�ana 
// -> ASourceAddress    - Adres z kt�rego nast�puje wys�anie ramki potrzebny do odpowiedzi
// -> AONStreamTaskName - Nazwa zadania streamu kt�rym b�dzie wysy�ana ramka
//                        ! Je�li podamy nazw� streamu "local", to ramka trafi lokalnie do zadania wskazanego
// -> ADestAddress		- Adres pod kt�ry zostanie wys�ana ramka
// -> ADestTaskName     - Nazwa zadania do kt�rego ramka danych ma trafi�
//                        ! Je�li podamy NULL, to ramka trafi lokalnie do zadania wskazanego
// -> ADataFrame        - Wska�nik na zawarto�� ramki do wys�ania
// -> Alength			- Wielko�� w bajtach ramki do wys�ania
// -> AframeID          - Wska�nik pod kt�rym zostanie zapisany unikalny numer nadawczy ramki
//                        ! Warto�� jest potrzebna do idendyfikacji potwiedzenia odebrania ramki
// <- true - Pomy�lnie wys�ano ramk�
//    false - Wyst�pi� problem z wys�aniem ramki
bool TXB_board::SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, String AOnStreamTaskName, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID)
{
	TTaskDef *TaskDefStream = NULL;
	if (AOnStreamTaskName != "local")
	{
		TaskDefStream = GetTaskDefByName(AOnStreamTaskName);
		if (TaskDefStream == NULL) 
		{
			board.Log("Error: Stream task name not found...", true, true, tlError);
			return false;
		}
	}
	return SendFrameToDeviceTask(ASourceTaskName ,ASourceAddress , TaskDefStream,ADestAddress,ADestTaskName,ADataFrame, Alength,AframeID);
}
// ---------------------------------------------------------------
// Wys�anie ramki danych okre�lonym streamem do wskazanego zadania
// -> ASourceTaskName   - Nazwa zadania z kt�rego ramka jest wysy�ana 
// -> ASourceAddress    - Adres z kt�rego nast�puje wys�anie ramki potrzebny do odpowiedzi
// -> ATaskDefStream    - Wska�nik definicji zadania streamu kt�rym b�dzie wysy�ana ramka
// -> ADestAddress		- Adres pod kt�ry zostanie wys�ana ramka
// -> ADestTaskName     - Nazwa zadania do kt�rego ramka danych ma trafi�
//                        ! Je�li podamy NULL, to ramka trafi lokalnie do zadania wskazanego
// -> ADataFrame        - Wska�nik na zawarto�� ramki do wys�ania
// -> Alength			- Wielko�� w bajtach ramki do wys�ania
// -> AframeID          - Wska�nik pod kt�rym zostanie zapisany unikalny numer nadawczy ramki
//                        ! Warto�� jest potrzebna do idendyfikacji potwiedzenia odebrania ramki
// <- true - Pomy�lnie wys�ano ramk�
//    false - Wyst�pi� problem z wys�aniem ramki
bool TXB_board::SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, TTaskDef *ATaskDefStream, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID)
{	
	uint32_t reslen = 0;
	static THDFT *hdft = NULL;
	if (hdft != NULL)
	{
		//board.free(hdft);
		___free(hdft);
		hdft = NULL;
	}
//	hdft = (THDFT *)board._malloc(sizeof(THDFT));
	hdft = (THDFT*)___malloc(sizeof(THDFT));

	if (hdft == NULL)
	{
		board.Log("Error memory in send frame...", true, true, tlError);
		return false;
	}

	if (Alength > sizeof(hdft->FT.Frame))
	{
		board.Log("Error: send frame too long...", true, true,tlError);
		
		return false;
	}

	if (ADestTaskName.length() > 15)
	{
		board.Log("Error: Destination task name too long (15 char max)...", true, true,tlError);
		return false;
	}

	if (ASourceTaskName.length() > 15)
	{
		board.Log("Error: Sounrce task name too long (15 char max)...", true, true, tlError);
		return false;
	}

	if (ATaskDefStream != NULL)
	{
		hdft->ACK.a = FRAME_ACK_A;
		hdft->ACK.b = FRAME_ACK_B;
		hdft->ACK.c = FRAME_ACK_C;
		hdft->ACK.d = FRAME_ACK_D;
	}
	
	*AframeID = SysTickCount;
	hdft->FT.SourceAddress = ASourceAddress;
	hdft->FT.SourceDeviceID = board.DeviceID;
	hdft->FT.DestAddress = ADestAddress;
	hdft->FT.DestDeviceID.ID.ID64 = 0;
	hdft->FT.FrameID = *AframeID;
	hdft->FT.FrameType = ftData;
	
	xb_memorycopy(ADataFrame, &hdft->FT.Frame, Alength);
	hdft->FT.LengthFrame = Alength;

	xb_memorycopy((void *)(ASourceTaskName.c_str()), &hdft->FT.SourceTaskName, ASourceTaskName.length());
	xb_memorycopy((void *)(ADestTaskName.c_str()), &hdft->FT.DestTaskName, ADestTaskName.length());
	
#ifdef __riscv64
	hdft->FT.size = (((uint64_t)&hdft->FT.Frame) - ((uint64_t)&hdft->FT)) + hdft->FT.LengthFrame;
#else
	hdft->FT.size = (((uint32_t)&hdft->FT.Frame) - ((uint32_t)&hdft->FT)) + hdft->FT.LengthFrame;
#endif
	uint32_t ltsize = sizeof(THDFT) - (sizeof(TFrameTransport) - hdft->FT.size);
	if (ATaskDefStream != NULL)
	{
		hdft->FT.crc8 = board.crc8((uint8_t *)&hdft->FT, hdft->FT.size);
		reslen=PutStream(hdft, ltsize, ATaskDefStream,ADestAddress);
		if (reslen != ltsize)
		{
			*AframeID = 0;
			return false;
		}
	}
	else
	{
		HandleFrameLocal(&hdft->FT);
	}
	return true;
}
// ------------------------------
// Wys�anie ramki potwierdzaj�cej
// -> AFrameID - Numer ID ramki potwierdzanej
// -> ATaskDefStream - Wska�nik zadania streamu kt�rym zostanie wys�ana ramka odpowiadaj�ca
//                     ! Je�li NULL to ramka potwierdzaj�ca zostanie rozg�oszona lokalnie
// -> ASourceAddress - Adres z podkt�rego wysy�ane jest potwierdzaj�ca ramka
// -> ADestAddress   - Adres pod kt�ry potwierdzaj�ca ramka ma trafi�
// -> AframeType	 - Okre�lenie typu ramki, w przypadku wysy�ania potwierdzenie jest to rezultat
// -> ADestDeviceID  - ID urz�dzenia do kt�rego ramka potwierdzaj�ca ma trafi�. Potrzebny w celu wst�pnej segragacji przez framework
void TXB_board::SendResponseFrameOnProt(uint32_t AFrameID, TTaskDef *ATaskDefStream, uint32_t ASourceAddress, uint32_t ADestAddress, TFrameType AframeType, TUniqueID ADestDeviceID)
{
	THDFT_ResponseItem* hdft_Item = (THDFT_ResponseItem * )board._malloc(sizeof(THDFT_ResponseItem));

	if (ATaskDefStream != NULL)
	{
		hdft_Item->hdft.ACK.a = FRAME_ACK_A;
		hdft_Item->hdft.ACK.b = FRAME_ACK_B;
		hdft_Item->hdft.ACK.c = FRAME_ACK_C;
		hdft_Item->hdft.ACK.d = FRAME_ACK_D;
	}

	hdft_Item->hdft.FT.SourceAddress = ASourceAddress;
	hdft_Item->hdft.FT.SourceDeviceID = board.DeviceID;
	hdft_Item->hdft.FT.DestAddress = ADestAddress;
	hdft_Item->hdft.FT.DestDeviceID = ADestDeviceID;
	hdft_Item->hdft.FT.LengthFrame = 0;
	hdft_Item->hdft.FT.FrameID = AFrameID;
	hdft_Item->hdft.FT.FrameType = AframeType;
	
#ifdef __riscv64
	hdft_Item->hdft.FT.size = (((uint64_t)& hdft_Item->hdft.FT.Frame) - ((uint64_t)& hdft_Item->hdft.FT)) + hdft_Item->hdft.FT.LengthFrame;
#else
	hdft_Item->hdft.FT.size = (((uint32_t)& hdft_Item->hdft.FT.Frame) - ((uint32_t)& hdft_Item->hdft.FT)) + hdft_Item->hdft.FT.LengthFrame;
#endif
	hdft_Item->ltsize = sizeof(THDFT) - (sizeof(TFrameTransport) - hdft_Item->hdft.FT.size);
	hdft_Item->TaskDefStream = ATaskDefStream;
	hdft_Item->DestAddress = ADestAddress;

	if (ATaskDefStream != NULL)
	{
		hdft_Item->hdft.FT.crc8 = board.crc8((uint8_t *)& hdft_Item->hdft.FT, hdft_Item->hdft.FT.size);
		ADD_TO_LIST_STR(board.HDFT_ResponseItemList, THDFT_ResponseItem, hdft_Item);
	}
	else
	{
		HandleFrameLocal(&hdft_Item->hdft.FT);
		board.free(hdft_Item);
	}
	
	return;
}
// -----------------------------------------------------------------------------------------------------------------------
// Sprawdzenie sumy kontrolnej wskazanej ramki, wys�anie messaga do wskazanego zadania w ramce z wska�nikiem na dane ramki
// -> Aft - Wska�nik ramki transportowej
// -> wska�nik definicji zadania streamu kt�rym ramka dotar�a do urz�dzenia
void TXB_board::HandleFrame(TFrameTransport *Aft, TTaskDef *ATaskDefStream)
{
	uint8_t crc = Aft->crc8;
	Aft->crc8 = 0;

	if (crc == crc8((uint8_t *)Aft, Aft->size))
	{
		if (Aft->FrameType == ftData)
		{
			TTaskDef *DestTaskDefReceive = GetTaskDefByName(String(Aft->DestTaskName));
			if (DestTaskDefReceive == NULL)
			{
				SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream ,Aft->DestAddress, Aft->SourceAddress , ftThereIsNoSuchTask, Aft->SourceDeviceID);
			}
			else
			{
				TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
				mb.IDMessage = IM_FRAME_RECEIVE;
				mb.Data.FrameReceiveData.DataFrame = &Aft->Frame;
				mb.Data.FrameReceiveData.SizeFrame = Aft->LengthFrame;
				mb.Data.FrameReceiveData.TaskDefStream = ATaskDefStream;
				mb.Data.FrameReceiveData.SourceAddress = Aft->SourceAddress;
				mb.Data.FrameReceiveData.SourceTaskName = Aft->SourceTaskName;
				mb.Data.FrameReceiveData.DestAddress = Aft->DestAddress;

				bool res = DoMessage(&mb,true,NULL,DestTaskDefReceive);

				if (res)
				{
					RequestFarDevice(Aft->SourceDeviceID, ATaskDefStream, Aft->SourceAddress);

					SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, Aft->DestAddress, Aft->SourceAddress, ftResponseOK, Aft->SourceDeviceID);
				}
				else
				{
					TFrameType ft;
					switch (mb.Data.FrameReceiveData.FrameReceiveResult)
					{
					case frrOK: ft = ftResponseOK; break;
					case frrError: ft = ftResponseError; break;
					case frrBufferIsFull: ft = ftBufferIsFull; break;
					case frrOKWaitNext: ft = ftOKWaitForNext; break;
					case frrUnrecognizedType: ft = ftUnrecognizedType; break;
					default: ft = ftResponseError;	
					}
					SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, Aft->DestAddress, Aft->SourceAddress, ft, Aft->SourceDeviceID);

				}
			}
		}
		else
		{
			if (DeviceID.ID.ID64 == Aft->DestDeviceID.ID.ID64)
			{
				TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
				mb.IDMessage = IM_FRAME_RESPONSE;
				mb.Data.FrameResponseData.FrameID = Aft->FrameID;
				mb.Data.FrameResponseData.FrameType = Aft->FrameType;
				DoMessageOnAllTask(&mb, true, doFORWARD);
			}
		}
	}
	else
	{
		board.Log("Frame receive CRC error..", true, true, tlError);
		if (Aft->FrameType == ftData)
		{
			board.Log("Send rensponse CRC Eroor...", true, true, tlError);
			SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, Aft->DestAddress, Aft->SourceAddress, ftResponseCRCError, Aft->SourceDeviceID);
		}
		else
		{
			board.Log("In response frame is CRC error...", true, true, tlError);
		}
	}
}
// -----------------------------------------------------------------------------------
// Wys�anie messaga do wskazanego zadania w ramce z wska�nikiem na dane ramki lokalnie
// -> Aft - Wska�nik ramki transportowej
void TXB_board::HandleFrameLocal(TFrameTransport *Aft)
{
	if (Aft->FrameType == ftData)
	{
		TTaskDef *TaskDefReceive = GetTaskDefByName(String(Aft->DestTaskName));
		if (TaskDefReceive == NULL)
		{
			TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
			mb.IDMessage = IM_FRAME_RESPONSE;
			mb.Data.FrameResponseData.FrameID = Aft->FrameID;
			mb.Data.FrameResponseData.FrameType = ftThereIsNoSuchTask;
			DoMessageOnAllTask(&mb, true, doFORWARD);
			Log("There is no indicated task in the system to send a frame.", true, true,tlError);
		}
		else
		{
			TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
			mb.IDMessage = IM_FRAME_RECEIVE;
			mb.Data.FrameReceiveData.DataFrame = &Aft->Frame;
			mb.Data.FrameReceiveData.SizeFrame = Aft->LengthFrame;
			mb.Data.FrameReceiveData.TaskDefStream = NULL;
			mb.Data.FrameReceiveData.SourceAddress = Aft->SourceAddress;
			mb.Data.FrameReceiveData.SourceTaskName = Aft->SourceTaskName;
			mb.Data.FrameReceiveData.DestAddress = Aft->DestAddress;

			bool res = DoMessage(&mb,true,NULL,TaskDefReceive);
			if (res)
			{
				SendResponseFrameOnProt(Aft->FrameID, NULL,0,0, ftResponseOK, Aft->SourceDeviceID);
			}
			else
			{
				TFrameType ft;
				switch (mb.Data.FrameReceiveData.FrameReceiveResult)
				{
				case frrOK: ft = ftResponseOK; break;
				case frrError: ft = ftResponseError; break;
				case frrBufferIsFull: ft = ftBufferIsFull; break;
				case frrOKWaitNext: ft = ftOKWaitForNext; break;
				case frrUnrecognizedType: ft = ftUnrecognizedType; break;
				default: ft = ftResponseError;	
				}
				SendResponseFrameOnProt(Aft->FrameID, NULL,0,0, ft, Aft->SourceDeviceID);
			}
		}
	}
	else
	{
		if (DeviceID.ID.ID64 == Aft->SourceDeviceID.ID.ID64)
		{
			TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
			mb.IDMessage = IM_FRAME_RESPONSE;
			mb.Data.FrameResponseData.FrameID = Aft->FrameID;
			mb.Data.FrameResponseData.FrameType = Aft->FrameType;
			DoMessageOnAllTask(&mb, true, doFORWARD);
		}
	}
	
}
// -----------------------------------------------------------------------------------
bool TXB_board::RequestFarDevice(TUniqueID AFarDeviceID, TTaskDef *ATaskDefStream, uint32_t AOnAddress)
{
	TFarDeviceID* fdl = FarDeviceIDList;
	while (fdl != NULL) 
	{
		if (AFarDeviceID.ID.ID64 == fdl->FarDeviceID.ID.ID64)
		{
			if (ATaskDefStream == fdl->RequestTaskStream)
			{
				if (AOnAddress == fdl->FarAddress)
				{
					break;
				}
			}
		}
		fdl = fdl->Next;
	}
	if (fdl == NULL)
	{
		CREATE_STR_ADD_TO_LIST(FarDeviceIDList, TFarDeviceID, fdl);
		if (fdl != NULL)
		{
			fdl->FarDeviceID.ID.ID64 = AFarDeviceID.ID.ID64;
			fdl->RequestTaskStream = ATaskDefStream;
			fdl->FarAddress = AOnAddress;
			SaveCfgFarDevice(fdl);
		}
	}
	if (fdl != NULL)
	{
		fdl->TickLastRequest = SysTickCount;
		fdl->DateTimeLastRequest = DateTimeUnix;
#ifdef XB_GUI
		XB_BOARD_Repaint_TFarDeviceID();
#endif
	}
	else
	{
		Log("Error add new DeviceID to FarDeviceIDList.", true, true, tlError);
		return false;
	}
	return true;
}
// -----------------------------------------------------------------------------------
void TXB_board::EraseAllFarDevices()
{
	TFarDeviceID* fdl = FarDeviceIDList_last;
	while (fdl != NULL)
	{
		DESTROY_STR_DEL_FROM_LIST(FarDeviceIDList, fdl);
		fdl = FarDeviceIDList_last;
	}
}
// -----------------------------------------------------------------------------------
void TXB_board::SaveCfgFarDevice(TFarDeviceID *Afdl)
{
#ifdef XB_PREFERENCES
	String StreamDefTaskName; StreamDefTaskName.reserve(32);
	if (SendMessage_GetTaskNameString(Afdl->RequestTaskStream->Task, &StreamDefTaskName))
	{
		if (board.PREFERENCES_BeginSection("XBBOARD"))
		{
			uint16_t Countfdl = board.PREFERENCES_GetUINT16("fd_count",0);
			String itemname; itemname.reserve(16);
			TUniqueID itemdeviceid;
			String itemstreamdeftaskname;
			uint32_t itemonaddress;
			for (uint16_t i = 0; i < Countfdl; i++)
			{
				itemname = "fditem" + String(i);
				board.PREFERENCES_GetArrayBytes(String(itemname+"devid").c_str(), (void*)&itemdeviceid.ID.ID, 8);
				itemstreamdeftaskname = board.PREFERENCES_GetString(String(itemname + "tsn").c_str(), "");
				itemonaddress = board.PREFERENCES_GetUINT32(String(itemname + "oa").c_str(), 0);

				if (itemdeviceid.ID.ID64 == Afdl->FarDeviceID.ID.ID64)
				{
					if (itemstreamdeftaskname == StreamDefTaskName)
					{
						if (itemonaddress == Afdl->FarAddress)
						{
							board.PREFERENCES_EndSection();
							return;
						}
					}
				}
			}

			Countfdl++;
			board.PREFERENCES_PutUINT16("fd_count", Countfdl);
			itemname = "fditem" + String(Countfdl-1);
			board.PREFERENCES_PutArrayBytes(String(itemname + "devid").c_str(), (void*)&Afdl->FarDeviceID.ID, 8);
			board.PREFERENCES_PutString(String(itemname + "tsn").c_str(), StreamDefTaskName);
			board.PREFERENCES_PutUINT32(String(itemname + "oa").c_str(), Afdl->FarAddress);

			board.PREFERENCES_EndSection();
		}
	}
	else
	{
		board.Log("Error indent stream task in request device id...", true, true, tlError);
	}
#endif
}
// -----------------------------------------------------------------------------------
void TXB_board::ResetCfgFarDevices()
{
#ifdef XB_PREFERENCES
		if (board.PREFERENCES_BeginSection("XBBOARD"))
		{
			uint16_t Countfdl = board.PREFERENCES_GetUINT16("fd_count", 0);
			String itemname; itemname.reserve(16);
			for (uint16_t i = 0; i < Countfdl; i++)
			{
				itemname = "fditem" + String(i);
				board.PREFERENCES_CLEAR(String(itemname + "devid").c_str());
				board.PREFERENCES_CLEAR(String(itemname + "tsn").c_str());
				board.PREFERENCES_CLEAR(String(itemname + "oa").c_str());
			}
			board.PREFERENCES_PutUINT16("fd_count", 0);
			board.PREFERENCES_EndSection();
		}
#endif
}
// -----------------------------------------------------------------------------------
void TXB_board::LoadCfgFarDevices()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("XBBOARD"))
	{
		EraseAllFarDevices();
		uint16_t Countfdl = board.PREFERENCES_GetUINT16("fd_count", 0);
		String itemname; itemname.reserve(16);
		TUniqueID itemdeviceid;
		String itemstreamdeftaskname;
		TTaskDef* itemTaskDefStream = NULL;
		uint32_t itemonaddress;
		
		for (uint16_t i = 0; i < Countfdl; i++)
		{
			itemname = "fditem" + String(i);
			board.PREFERENCES_GetArrayBytes(String(itemname + "devid").c_str(), (void*)&itemdeviceid.ID.ID, 8);
			itemstreamdeftaskname = board.PREFERENCES_GetString(String(itemname + "tsn").c_str(), "");
			itemstreamdeftaskname.trim();
			itemonaddress = board.PREFERENCES_GetUINT32(String(itemname + "oa").c_str(), 0);
			itemTaskDefStream = GetTaskDefByName(itemstreamdeftaskname);

			{
				TFarDeviceID* fdl = NULL;
				CREATE_STR_ADD_TO_LIST(FarDeviceIDList, TFarDeviceID, fdl);
				if (fdl != NULL)
				{
					fdl->FarDeviceID.ID.ID64 = itemdeviceid.ID.ID64;
					fdl->RequestTaskStream = itemTaskDefStream;
					fdl->FarAddress = itemonaddress;
				}
			}
		}

		board.PREFERENCES_EndSection();
	}
#endif
}
// -----------------------------------------------------------------------------------
// Ustalenie wskazanego task streamu jako klawiatury
// -> AStreamDefTask - Task stream kt�ry ma by� klawiatur�
// -> Aaddress - indywidualny adres streamu
void TXB_board::AddStreamAddressAsKeyboard(TTaskDef *AStreamDefTask,uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountGetStreamAddressAsKeyboard == 0)
		{
			t->GetStreamAddressAsKeyboard = (uint32_t *)_malloc(sizeof(uint32_t));
			t->GetStreamAddressAsKeyboard[0] = Aaddress;
			t->CountGetStreamAddressAsKeyboard++;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountGetStreamAddressAsKeyboard; i++)
			{
				if (t->GetStreamAddressAsKeyboard[i] == Aaddress)
				{
					return;
				}
			}
			
			for (uint32_t i = 0; i < t->CountGetStreamAddressAsKeyboard; i++)
			{
				if (t->GetStreamAddressAsKeyboard[i] == 0xffffffff)
				{
					t->GetStreamAddressAsKeyboard[i] = Aaddress;
					return;
				}
			}

			t->GetStreamAddressAsKeyboard = (uint32_t *)___realloc((void *)t->GetStreamAddressAsKeyboard, sizeof(uint32_t)*(t->CountGetStreamAddressAsKeyboard + 1));
			t->CountGetStreamAddressAsKeyboard++;

			t->GetStreamAddressAsKeyboard[t->CountGetStreamAddressAsKeyboard-1] = Aaddress;
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
// Odj�cie adresu streamu wskazanego task streamu jako klawiatury
// -> AStreamDefTask - Task stream z kt�rego usuwany jest adres
// -> Aaddress - indywidualny adres streamu
void TXB_board::SubStreamAddressAsKeyboard(TTaskDef *AStreamDefTask, uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountGetStreamAddressAsKeyboard == 0)
		{
			return;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountGetStreamAddressAsKeyboard; i++)
			{
				if (t->GetStreamAddressAsKeyboard[i] == Aaddress)
				{
					t->GetStreamAddressAsKeyboard[i] = 0xffffffff;
					break;
				}
			}
			
			uint32_t li = 0;
			
			for (uint32_t i = 0; i < t->CountGetStreamAddressAsKeyboard; i++)
			{
				if (t->GetStreamAddressAsKeyboard[i] == 0xffffffff) li++;
			}
			
			if (li == t->CountGetStreamAddressAsKeyboard)
			{
				t->CountGetStreamAddressAsKeyboard = 0;
				free(t->GetStreamAddressAsKeyboard);
				t->GetStreamAddressAsKeyboard = NULL;
			}
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
void TXB_board::AddStreamAddressAsLog(TTaskDef *AStreamDefTask, uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountPutStreamAddressAsLog == 0)
		{
			t->PutStreamAddressAsLog = (uint32_t *)_malloc(sizeof(uint32_t));
			t->PutStreamAddressAsLog[0] = Aaddress;
			t->CountPutStreamAddressAsLog++;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)
			{
				if (t->PutStreamAddressAsLog[i] == Aaddress)
				{
					return;
				}
			}
			
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)
			{
				if (t->PutStreamAddressAsLog[i] == 0xffffffff)
				{
					t->PutStreamAddressAsLog[i] = Aaddress;
					return;
				}
			}

			t->PutStreamAddressAsLog = (uint32_t *)___realloc((void *)t->PutStreamAddressAsLog, sizeof(uint32_t)*(t->CountPutStreamAddressAsLog + 1));
			t->CountPutStreamAddressAsLog++;

			t->PutStreamAddressAsLog[t->CountPutStreamAddressAsLog-1] = Aaddress;
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
void TXB_board::SubStreamAddressAsLog(TTaskDef *AStreamDefTask, uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountPutStreamAddressAsLog == 0)
		{
			return;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)
			{
				if (t->PutStreamAddressAsLog[i] == Aaddress)
				{
					t->PutStreamAddressAsLog[i] = 0xffffffff;
					break;
				}
			}
			
			uint32_t li = 0;
			
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)
			{
				if (t->PutStreamAddressAsLog[i] == 0xffffffff) li++;
			}
			
			if (li == t->CountPutStreamAddressAsLog)
			{
				t->CountPutStreamAddressAsLog = 0;
				free(t->PutStreamAddressAsLog);
				t->PutStreamAddressAsLog = NULL;
			}
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
void TXB_board::AddStreamAddressAsGui(TTaskDef *AStreamDefTask, uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountPutStreamAddressAsGui == 0)
		{
			t->PutStreamAddressAsGui = (uint32_t *)_malloc(sizeof(uint32_t));
			t->PutStreamAddressAsGui[0] = Aaddress;
			t->CountPutStreamAddressAsGui++;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsGui; i++)
			{
				if (t->PutStreamAddressAsGui[i] == Aaddress)
				{
					return;
				}
			}
			
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsGui; i++)
			{
				if (t->PutStreamAddressAsGui[i] == 0xffffffff)
				{
					t->PutStreamAddressAsGui[i] = Aaddress;
					return;
				}
			}

			t->PutStreamAddressAsGui = (uint32_t *)___realloc((void *)t->PutStreamAddressAsGui, sizeof(uint32_t)*(t->CountPutStreamAddressAsGui + 1));
			t->CountPutStreamAddressAsGui++;

			t->PutStreamAddressAsGui[t->CountPutStreamAddressAsGui-1] = Aaddress;
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
void TXB_board::SubStreamAddressAsGui(TTaskDef *AStreamDefTask, uint32_t Aaddress)
{
	TTask *t = AStreamDefTask->Task;
	if (t != NULL)
	{
		if (t->CountPutStreamAddressAsGui == 0)
		{
			return;
		}
		else
		{
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsGui; i++)
			{
				if (t->PutStreamAddressAsGui[i] == Aaddress)
				{
					t->PutStreamAddressAsGui[i] = 0xffffffff;
					break;
				}
			}
			
			uint32_t li = 0;
			
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsGui; i++)
			{
				if (t->PutStreamAddressAsGui[i] == 0xffffffff) li++;
			}
			
			if (li == t->CountPutStreamAddressAsGui)
			{
				t->CountPutStreamAddressAsGui = 0;
				free(t->PutStreamAddressAsGui);
				t->PutStreamAddressAsGui = NULL;
			}
			return;
		}
	}
	return;
}

#pragma endregion
#pragma region FUNKCJE_KOMUNIKATOW


TConsoleScreen::TConsoleScreen()
{
	Buf = NULL;
	ColorBuf = NULL;
	Currsor_X = 0;
	Currsor_Y = 0;
	Color = 0;
	Width = xb_board_ConsoleWidth;
	Height = xb_board_ConsoleHeight;
	Repaint_All = 1;
	CreateConsoleInWindow();
}

TConsoleScreen::~TConsoleScreen()
{
	DestroyConsoleInWindow();
}

void TConsoleScreen::CreateConsoleInWindow()
{
	if (Buf != NULL)
	{
		board.freeandnull((void**)&Buf);
	}
	if (ColorBuf != NULL)
	{
		board.freeandnull((void**)&ColorBuf);
	}

	Size_Buf = Width * Height;
	Buf = (uint8_t*)board._malloc_psram(Size_Buf);
	if (Buf != NULL)
	{
		Size_ColorBuf = ((Width * Height) / 4)+8;
		ColorBuf = (TConsoleColor *)board._malloc_psram(Size_ColorBuf);
#ifdef XB_GUI
		xb_board_winHandle1 = GUI_WindowCreate(&XB_BOARD_DefTask, 1);
#endif
	}
}

void TConsoleScreen::DestroyConsoleInWindow()
{
#ifdef XB_GUI
	if (xb_board_winHandle1 != NULL)
	{
		xb_board_winHandle1->Close();
	}
#endif

	if (Buf != NULL)
	{
		board.freeandnull((void**)&Buf);
		Size_Buf = 0;
	}
	if (ColorBuf != NULL)
	{
		board.freeandnull((void**)&ColorBuf);
		Size_ColorBuf = 0;
	}
}

void TConsoleScreen::Set_ColorBuf(uint32_t Aindx,uint8_t Acolor)
{
	if (ColorBuf != NULL)
	{
		uint32_t indx = Aindx / 4;
		if (indx >= Size_ColorBuf)
		{
			Serial.print("Error in Set ColorBuf, overflow");
			return;

		}
		switch (Aindx % 4)
		{
		case 0: ColorBuf[indx].Color0 = Acolor; break;
		case 1: ColorBuf[indx].Color1 = Acolor; break;
		case 2: ColorBuf[indx].Color2 = Acolor; break;
		case 3: ColorBuf[indx].Color3 = Acolor; break;
		default: break;
		}
	}
}

uint8_t TConsoleScreen::Get_ColorBuf(uint32_t Aindx)
{
	uint8_t _Color = 0;
	if (ColorBuf != NULL)
	{
		uint32_t indx = Aindx / 4;
		if (indx >= Size_ColorBuf)
		{
			Serial.print("Error in Get ColorBuf, overflow");
			return 0;
		}

		switch (Aindx % 4)
		{
		case 0: _Color = ColorBuf[indx].Color0; break;
		case 1: _Color = ColorBuf[indx].Color1; break;
		case 2: _Color = ColorBuf[indx].Color2; break;
		case 3: _Color = ColorBuf[indx].Color3; break;
		default: break;
		}
	}
	return _Color;
}


void TConsoleScreen::ScrollUPConsole()
{
	if (Buf != NULL)
	{
		for (uint32_t y = 0; y < Height - 1; y++)
		{
			for (uint32_t x = 0; x < Width; x++)
			{
				uint32_t indx_s = ((y + 1) * Width) + x;
				uint32_t indx_d = ((y)*Width) + x;
				if (indx_d >= Size_Buf)
				{
					Serial.println("Error in Scroll up,indx_d overflow");
					return;
				}
				if (indx_s >= Size_Buf)
				{
					Serial.println("Error in Scroll up,indx_s overflow");
					return;
				}

				Buf[indx_d] = Buf[indx_s];
				Set_ColorBuf(indx_d, Get_ColorBuf(indx_s));
			}
		}
		for (uint32_t x = 0; x < Width; x++)
		{
			uint32_t indx = ((Height - 1) * Width) + x;
			if (indx >= Size_Buf)
			{
				Serial.println("Error in Scroll up,indx overflow");
				return;
			}
			Buf[indx] = ' ';
			Set_ColorBuf(indx,Color);

		}

		Repaint_All++;
	}
}

void TConsoleScreen::PutCharConsole(uint8_t Ach)
{
	switch (Ach)
	{
	case 10:
	{
		Currsor_X = 0;

		Currsor_Y++;
		if (Currsor_Y >= Height)
		{
			Currsor_Y--;
			ScrollUPConsole();
		}
		break;
	}
	case 13:
	{
		Currsor_X = 0;

		Currsor_Y++;
		if (Currsor_Y >= Height)
		{
			Currsor_Y--;
			ScrollUPConsole();
		}

		break;
	}
	default:
	{
		uint32_t indx = Currsor_X + (Currsor_Y * Width);
		if (Buf != NULL)
		{
			if (indx >= Size_Buf)
			{
				Serial.print("Error in Put Console, overflow");
				return ;
			}
			Buf[indx] = Ach;
			Set_ColorBuf(indx,Color);
		}
		Repaint_All++;

		Currsor_X++;
		if (Currsor_X >= Width)
		{
			Currsor_X = 0;
			Currsor_Y++;
			if (Currsor_Y >= Height)
			{
				Currsor_Y--;
				ScrollUPConsole();
			}
		}
		break;
	}
	}

#ifdef XB_GUI
	if (xb_board_winHandle1 != NULL)
	{
		xb_board_winHandle1->RepaintDataCounter++;
	}
#endif
}

void TConsoleScreen::PutConsole(uint8_t* Adata, uint32_t Alength)
{
	uint32_t i = 0;
	while (Alength > 0) 
	{
		PutCharConsole(Adata[i]);
		i++;
		Alength--;
	}
}


uint8_t TXB_board::SumEnableTXStream()
{
	TTask* t = TaskList;
	uint8_t SumEnable = 0;
	while (t != NULL)
	{
		if (IsTaskStream(t->TaskDef))
		{
			if (!StatusDisableTXStream(t->TaskDef))
			{
				SumEnable++;
			}
		}
		t = t->Next;
	}
	return SumEnable;
}




void TXB_board::AllPutStreamGui(void *Adata, uint32_t Alength)
{
	TTask *t = TaskList;
	while (t != NULL)
	{
		for (uint32_t i = 0; i < t->CountPutStreamAddressAsGui; i++)		
		{
			if (t->PutStreamAddressAsGui[i] != 0xffffffff)
			{
				PutStream(Adata, Alength, t->TaskDef, t->PutStreamAddressAsGui[i]);		
			}
		}
		t = t->Next;
	}
}


void TXB_board::AllPutStreamLog(void *Adata, uint32_t Alength)
{
	if ((xb_board_ConsoleInWindow == true) && (ConsoleScreen != NULL))
	{
		ConsoleScreen->PutConsole((uint8_t *)Adata, Alength);
		if (xb_board_Consoleputtoserial) Serial.write((const char*)Adata, Alength);
	}
	else
	{
		TTask* t = TaskList;
		bool isput = false;
		while (t != NULL)
		{
			for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)
			{
				if (t->PutStreamAddressAsLog[i] != 0xffffffff)
				{
					PutStream(Adata, Alength, t->TaskDef, t->PutStreamAddressAsLog[i]);
					//Serial.print("DUPA");
					//Serial.write((const char*)Adata, Alength);
					isput = true;
				}
			}
			t = t->Next;
		}
		if (!isput)
		{
#ifndef Serial0BoardBuf_BAUD
			Serial.write((const char*)Adata, (size_t)Alength);
#endif
		}
	}
}

int TXB_board::print(String Atext)
{
	if (CurrentTask != NULL)
	{
		int len = Atext.length();	
		AllPutStreamLog((void *)Atext.c_str(), len);
		return len;
	}
	return 0;
}

void TXB_board::Log(char Achr, TTypeLog Atl)
{
	if (CurrentTask != NULL)
	{
		if (Atl == tlInfo)
		{
			if (CurrentTask->ShowLogInfo == false)
			{
				return;
			}
		}
		else if (Atl == tlWarn)
		{
			if (CurrentTask->ShowLogWarn == false)
			{
				return;
			}
		}
		else if (Atl == tlError)
		{
			if (CurrentTask->ShowLogError == false)
			{
				return;
			}
		}
	}
	else return;
	
	if ((xb_board_ConsoleInWindow == true) && (ConsoleScreen != NULL))
	{
		switch (Atl)
		{
		case tlInfo: ConsoleScreen->Color = 0; break;
		case tlWarn: ConsoleScreen->Color = 1; break;
		case tlError: ConsoleScreen->Color = 2; break;
		}
	}

	AllPutStreamLog(&Achr, 1);
	
	if (!xb_board_ConsoleInWindow)
	if (NoTxCounter == 0) TXCounter++;
}

void TXB_board::Log(const char *Atxt, bool puttime, bool showtaskname, TTypeLog Atl)
{
	TTaskDef *NameTaskDef=NULL;
		
	if (showtaskname)
	{
		if (CurrentTask != NULL)
		{
			NameTaskDef = CurrentTask->TaskDef;
		}
		else showtaskname = false;
	}

	if (CurrentTask != NULL)
	{
		if (Atl == tlInfo)
		{
			if (CurrentTask->ShowLogInfo == false)
			{
				return;
			}
		}
		else if (Atl == tlWarn)
		{
			if (CurrentTask->ShowLogWarn == false)
			{
				return;
			}
		}
		else if (Atl == tlError)
		{
			if (CurrentTask->ShowLogError == false)
			{
				return;
			}
		}
	}
	else return;

	if ((xb_board_ConsoleInWindow == true) && (ConsoleScreen != NULL))
	{
		switch (Atl)
		{
		case tlInfo: ConsoleScreen->Color = 0; break;
		case tlWarn: ConsoleScreen->Color = 1; break;
		case tlError: ConsoleScreen->Color = 2; break;
		}
	}


	int len = StringLength((char *)Atxt, 0);
	if (len == 0) return;

	if (puttime)
	{
		String txttime = ""; txttime.reserve(40);
		GetTime(txttime, DateTimeUnix,false,false,true);
		txttime = "\n[" + txttime + "] ";
		AllPutStreamLog((void *)txttime.c_str(), txttime.length());
	}

	if (showtaskname)
	{
		if (NameTaskDef != NULL)
		{
			String taskname = "---";
			SendMessage_GetTaskNameString(NameTaskDef, &taskname);
		
			if (taskname.length() > 0)
			{
				taskname.trim();
				taskname = '[' + taskname + "] ";
				AllPutStreamLog((void *)taskname.c_str(), taskname.length());
			}
		}
	}

	AllPutStreamLog((void *)Atxt, len);

	if (!xb_board_ConsoleInWindow)
		if (NoTxCounter == 0) TXCounter++;
}
#pragma endregion
#pragma region FUNKCJE_PREFERENCES
//-----------------------------------------------------------------------------------------------------------------------
bool TXB_board::PREFERENCES_BeginSection(String ASectionname)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	
	String_OnlyRAM++;
	if (ASectionname == "")
	{
		String sn=" "; sn.reserve(32);
		SendMessage_GetTaskNameString(CurrentTask, &sn);
		ASectionname = sn;
	}
	

	if (ASectionname.length() >= 16)
	{
		String ts = ASectionname;
		ASectionname = ASectionname.substring(0, 15);
	}
	if (xbpreferences == NULL)
	{
		xbpreferences = new Preferences();
	}
	String_OnlyRAM--;
	if (ASectionname.length() == 0)
	{
		return false;
	}
	else
	{
		return xbpreferences->begin(ASectionname.c_str());
	}
#else
	return false;
#endif
#else
	return false;
#endif

}
//-----------------------------------------------------------------------------------------------------------------------
void TXB_board::PREFERENCES_EndSection()
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	preferences_freeEntries = xbpreferences->freeEntries();
	delete(xbpreferences);
	xbpreferences = NULL;
#endif
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
void TXB_board::PREFERENCES_CLEAR()
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return;
	xbpreferences->clear();
#endif
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
void TXB_board::PREFERENCES_CLEAR(String Akey)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return;
	xbpreferences->remove(Akey.c_str());
#endif
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutArrayBytes(const char* key, const void *array,size_t sizearray)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putBytes(key, array,sizearray);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_GetArrayBytes(const char* key, void* array, size_t maxsizearray)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->getBytes(key, array, maxsizearray);
#else
	return 0;
#endif
#else
	return 0;
#endif
}


//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutBool(const char* key, const bool value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putBool(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
bool TXB_board::PREFERENCES_GetBool(const char* key, const bool defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getBool(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutINT8(const char* key, const int8_t value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putChar(key, (char)value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
int8_t TXB_board::PREFERENCES_GetINT8(const char* key, const int8_t defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return (int8_t)xbpreferences->getChar(key,(char) defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_GetString(const char* key, char* value, const size_t maxlen)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->getString(key, value, maxlen);
#else
	return 0;
#endif
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
String TXB_board::PREFERENCES_GetString(const char* key, String defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	String_OnlyRAM++;
	String s= xbpreferences->getString(key, String(defaultvalue.c_str()));
	String_OnlyRAM--;
	return s;
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
uint32_t TXB_board::PREFERENCES_GetUINT32(const char* key, uint32_t defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getULong(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
uint16_t TXB_board::PREFERENCES_GetUINT16(const char* key, uint16_t defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getUShort(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
uint8_t TXB_board::PREFERENCES_GetUINT8(const char* key, uint8_t defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getUChar(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutString(const char* key, const char* value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putString(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutString(const char* key, String value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putString(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutUINT32(const char* key, uint32_t value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putULong(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutUINT16(const char* key, uint16_t value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putUShort(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutUINT8(const char* key, uint8_t value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putUChar(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
int16_t TXB_board::PREFERENCES_GetINT16(const char* key, int16_t defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getShort(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutINT16(const char* key, int16_t value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putShort(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutDouble(const char* key, double value)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return 0;
	return xbpreferences->putDouble(key, value);
#else
	return 0;
#endif
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
double TXB_board::PREFERENCES_GetDouble(const char* key, double defaultvalue)
{
#ifdef ESP32
#ifdef XB_PREFERENCES
	if (xbpreferences == NULL) return defaultvalue;
	return xbpreferences->getDouble(key, defaultvalue);
#else
	return defaultvalue;
#endif
#else
	return defaultvalue;
#endif
}



void TXB_board::LoadConfiguration(TTaskDef* ATaskDef)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_LOAD_CONFIGURATION;
	if (!DoMessage(&mb, true, CurrentTask, ATaskDef))
	{
		String tn;
		if (SendMessage_GetTaskNameString(ATaskDef, &tn));
		{
			Log(String("Task [" + tn + "] not support load configuration.").c_str(), true, true, tlWarn);
		}
	}
}

void TXB_board::LoadConfiguration(TTask* ATask)
{
	if (ATask!=NULL)
		LoadConfiguration(ATask->TaskDef);
}

void TXB_board::LoadConfiguration()
{
	LoadConfiguration(CurrentTask);
}

void TXB_board::SaveConfiguration(TTaskDef* ATaskDef)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_SAVE_CONFIGURATION;
	if (!DoMessage(&mb, true, CurrentTask, ATaskDef))
	{
		String tn;
		if (SendMessage_GetTaskNameString(ATaskDef, &tn));
		{
			Log(String("Task [" + tn + "] not support save configuration.").c_str(), true, true, tlWarn);
		}
	}

}

void TXB_board::SaveConfiguration(TTask* ATask)
{
	if (ATask != NULL)
		SaveConfiguration(ATask->TaskDef);
}

void TXB_board::SaveConfiguration()
{
	SaveConfiguration(CurrentTask);
}

void TXB_board::ResetConfiguration(TTaskDef* ATaskDef)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_RESET_CONFIGURATION;
	if (!DoMessage(&mb, true, CurrentTask, ATaskDef))
	{
		String tn;
		if (SendMessage_GetTaskNameString(ATaskDef,&tn));
		{
			Log(String("Task ["+tn+"] not support reset configuration.").c_str(), true, true, tlWarn);
		}
	}
}

void TXB_board::ResetConfiguration(TTask* ATask)
{
	if (ATask != NULL)
		ResetConfiguration(ATask->TaskDef);
}

void TXB_board::ResetConfiguration()
{
	ResetConfiguration(CurrentTask);
}

void TXB_board::AllSaveConfiguration(void)
{
	TTask* task = TaskList;
	while (task != NULL)
	{
		SaveConfiguration(task);
		task = task->Next;
	}
}

void TXB_board::AllResetConfiguration(void)
{
	TTask* task = TaskList;
	while (task != NULL)
	{
		ResetConfiguration(task);
		task = task->Next;
	}
}

#pragma endregion 

#pragma region FUNKCJE_SETUP_LOOP_MESSAGES
// -------------------------------------
// Procedura inicjuj�ca zadanie g��wnego
void XB_BOARD_Setup(void)
{
	// odnotowanie ilo�ci wolnej pami�ci RAM na starcie
	board.FreeHeapInLoop = board.getFreeHeap();
	board.MaximumFreeHeapInLoop = board.FreeHeapInLoop;
	board.MinimumFreeHeapInLoop = board.FreeHeapInLoop;

	board.LoadConfiguration(&XB_BOARD_DefTask);
	board.AddGPIODrive(BOARD_NUM_DIGITAL_PINS, &XB_BOARD_DefTask, "ESP32");
	board.SetDigitalPinCount(BOARD_NUM_DIGITAL_PINS);

	// Je�li framework zosta� uruchomiony na ESP8266 to inicjacja licznik�w czasowych
#if defined(ESP8266)
	board.SysTickCount_init();
	board.DateTimeSecond_init();
#endif
	// Zerowanie licznik�w czasowych
	DateTimeUnix = 0;
	DateTimeStart = 0;

	// Skonfigurowanie pinu (je�li podano) informuj�cego na zewn�trz �e aplikacja dzia�a
#ifdef BOARD_LED_LIFE_PIN
	board.pinMode(BOARD_LED_LIFE_PIN, OUTPUT);
#endif

	// Skonfigurowanie pinu (je�li podano) informuj�cego na zewn�trz �e nast�pi�a transmisja danych na zewn�trz
#ifdef BOARD_LED_TX_PIN
	board.Tick_TX_BLINK = 0;
	board.pinMode(BOARD_LED_TX_PIN, OUTPUT);
#if defined(BOARD_LED_TX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_TX_PIN, (BOARD_LED_TX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_TX_PIN, LOW);
#endif
#endif

	// Skonfigurowanie pinu (je�li podano) informuj�cego na zewn�trz �e nast�pi�a transmisja danych z zewn�trz
#ifdef BOARD_LED_RX_PIN
	board.Tick_RX_BLINK = 0;
	board.pinMode(BOARD_LED_RX_PIN, OUTPUT);
#if defined(BOARD_LED_RX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_RX_PIN, (BOARD_LED_RX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_RX_PIN, LOW);
#endif
#endif

	// Ustawienie czasu jak d�ugo ma utrzymywa� si� stan 1 na pinach informuj�cych na zew�trz o transmisji danych
#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
#ifdef BOARD_LED_RXTX_BLINK_TICK
	board.TickEnableBlink = BOARD_LED_RXTX_BLINK_TICK;
#else
	board.TickEnableBlink = 250;
#endif
#endif

#ifdef Serial0BoardBuf_BAUD
	board.AddTask(&XB_SERIAL_DefTask);
#endif

	// Je�li interface uruchomiony to dodanie zadania obs�uguj�cego interface GUI
#ifdef XB_GUI
	board.AddTask(&XB_GUI_DefTask);
#endif

	board_log("\n\n----------------------------------------------------");
	board_log("\nStart XB_BOARD system by XBary. (email: xbary@wp.pl)\n");

}
// -----------------------------
// G��wna p�tla zadania g��wnego
// <-      = 0 - Zadanie dzia�a wed�ug harmonogramu priorytet�w
//         > 0 - Zadanie zostanie uruchomione po ilo�ci milisekund zwr�cnej w rezultacie
uint32_t XB_BOARD_DoLoop(void)
{
	static bool FirstLoop = false;
	if (!FirstLoop)
	{
		FirstLoop = true;
		board.LoadCfgFarDevices();


		board_log("\nDevice Name: \""+board.DeviceName+"\"");
		board_log("\nDevice Version: \"" + board.DeviceVersion + "\"");
		board_log("\nProject Name: \"" + board.ProjectName + "\"\n");



	}

	// Zamiganie
	board.handle();

	return 0;
}
// ---------------------------------
#ifdef XB_GUI

void XB_BOARD_Show_TFarDeviceID()
{
	xb_board_winHandle2 = GUI_WindowCreate(&XB_BOARD_DefTask, 2);
}

void XB_BOARD_Close_TFarDeviceID()
{
	if (xb_board_winHandle2 != NULL)
	{
		xb_board_winHandle2->Close();
	}
}

void XB_BOARD_Repaint_TFarDeviceID()
{
	if (xb_board_winHandle2 != NULL)
	{
		if ((board.FarDeviceIDList_count + 3) > xb_board_winHandle2->Height)
		{
			xb_board_winHandle2->SetWindowSize(100, board.FarDeviceIDList_count + 3);
			xb_board_winHandle2->Repaint();
		}
		xb_board_winHandle2->RepaintData();
	}
}

void XB_BOARD_Captions_TFarDeviceID(TWindowClass* Awh, Ty& Ay)
{
	Awh->GoToXY(0, Ay);
	Awh->PutStr("Device ID", 24, ' ', taLeft); Awh->PutChar('|');
	Awh->PutStr("Request Task Stream", 20, ' ', taLeft); Awh->PutChar('|');
	Awh->PutStr("Address", 16, ' ', taLeft); Awh->PutChar('|');
	Awh->PutStr("Date Time Last request", 32, ' ', taLeft); Awh->PutChar('|');
	Ay++;
}

void XB_BOARD_RowDraw_TFarDeviceID(TWindowClass* Awh, TFarDeviceID* Afdi, Ty& Ay)
{
	String str; str.reserve(64);
	Awh->GoToXY(0, Ay);
	if (Afdi != NULL)
	{
		Awh->PutStr(board.DeviceIDtoString(Afdi->FarDeviceID).c_str(), 24, ' ', taLeft); Awh->PutChar('|');

		board.SendMessage_GetTaskNameString(Afdi->RequestTaskStream, &str);
		str.trim();
		Awh->PutStr(str.c_str(), 20, ' ', taLeft); Awh->PutChar('|');

		Awh->PutStr(IPAddress(Afdi->FarAddress).toString().c_str(), 16, ' ', taLeft); Awh->PutChar('|');

		str = "";
		GetTime(str, Afdi->DateTimeLastRequest, true, true, true);
		Awh->PutStr(str.c_str(), 32, ' ', taLeft); Awh->PutChar('|');

	}
	Ay++;
}

#endif
// ---------------------------------
// Obs�uga messag�w zadania g��wnego
// -> Am - Wska�nik na strukture messaga
// <-      = true - messag zosta� obs�u�ony
//         = false - messag nie obs�ugiwany przez zadanie
bool XB_BOARD_DoMessage(TMessageBoard* Am)
{
	bool res = false;
	static uint8_t LastKeyCode = 0;

	switch (Am->IDMessage)
	{
	case IM_GET_TASKNAME_STRING:
	{
		GET_TASKNAME("BOARD");
		res = true;
		break;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		GET_TASKSTATUS_ADDSTR(String("TC:" + String(board.TaskList_count)));
		GET_TASKSTATUS_ADDSTR(String(" FD:" + String(board.FarDeviceIDList_count)));
		res = true;
	}
	case IM_HANDLEPTR:
	{
#ifdef XB_GUI
		HANDLEPTR(xb_board_winHandle0);
		HANDLEPTR(xb_board_winHandle1);
		HANDLEPTR(xb_board_winHandle2);
		HANDLEPTR(xb_board_menuHandle1);
		HANDLEPTR(xb_board_inputdialog0);
#endif
		res = true;
		break;
	}
	case IM_LOAD_CONFIGURATION:
	{
		XB_BOARD_LoadConfiguration();
		res = true;
		break;
	}
	case IM_SAVE_CONFIGURATION:
	{
		XB_BOARD_SaveConfiguration();
		res = true;
		break;
	}
	case IM_RESET_CONFIGURATION:
	{
		XB_BOARD_ResetConfiguration();
		res = true;
		break;
	}
	case IM_GPIO:
	{
		switch (Am->Data.GpioData.GpioAction)
		{
		case gaPinMode:
		{
			if (Am->Data.GpioData.GPIODrive == NULL) break;

			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < board.Digital_Pins_Count))
			{
				pin_Mode(Am->Data.GpioData.NumPin, (WiringPinMode)Am->Data.GpioData.ActionData.Mode);

				if (Am->Data.GpioData.ActionData.Mode == INPUT)
					board.SetPinInfo(Am->Data.GpioData.NumPin, FUNCTIONPIN_GPIO, MODEPIN_INPUT);
				else if (Am->Data.GpioData.ActionData.Mode == OUTPUT)
					board.SetPinInfo(Am->Data.GpioData.NumPin, FUNCTIONPIN_GPIO, MODEPIN_OUTPUT);
#ifdef ESP32
				else if (Am->Data.GpioData.ActionData.Mode == ANALOG)
					board.SetPinInfo(Am->Data.GpioData.NumPin, FUNCTIONPIN_ANALOGIN, MODEPIN_ANALOG);
#endif
				else
					board.SetPinInfo(Am->Data.GpioData.NumPin, FUNCTIONPIN_NOIDENT, MODEPIN_OUTPUT);

				res = true;
			}
			break;
		}
		case gaPinRead:
		{
			if (Am->Data.GpioData.GPIODrive == NULL) break;

			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < board.Digital_Pins_Count))
			{
				Am->Data.GpioData.ActionData.Value = digital_Read(Am->Data.GpioData.NumPin);

				if (board.PinInfoTable != NULL)
				{
					board.PinInfoTable[Am->Data.GpioData.NumPin].value = Am->Data.GpioData.ActionData.Value;
				}
				res = true;
			}
			break;
		}
		case gaPinWrite:
		{
			if (Am->Data.GpioData.GPIODrive == NULL) break;

			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < board.Digital_Pins_Count))
			{
				digital_Write(Am->Data.GpioData.NumPin, Am->Data.GpioData.ActionData.Value);

				if (board.PinInfoTable != NULL)
				{
					board.PinInfoTable[Am->Data.GpioData.NumPin].value = Am->Data.GpioData.ActionData.Value;
				}
				res = true;
			}
			break;
		}
		case gaPinToggle:
		{
			if (Am->Data.GpioData.GPIODrive == NULL) break;

			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < board.Digital_Pins_Count))
			{
				if (board.PinInfoTable != NULL)
				{
					Am->Data.GpioData.ActionData.Value = !board.PinInfoTable[Am->Data.GpioData.NumPin].value;
				}
				else
				{
					Am->Data.GpioData.ActionData.Value = !digitalRead(Am->Data.GpioData.NumPin);
				}

				digital_Write(Am->Data.GpioData.NumPin, Am->Data.GpioData.ActionData.Value);

				if (board.PinInfoTable != NULL)
				{
					board.PinInfoTable[Am->Data.GpioData.NumPin].value = Am->Data.GpioData.ActionData.Value;
				}
				res = true;
			}
			break;
		}
		default:
		{
			break;
		}
		}
		break;
	}

	case IM_KEYBOARD:
	{
		if (Am->Data.KeyboardData.TypeKeyboardAction == tkaKEYPRESS)
		{
			if (Am->Data.KeyboardData.KeyFunction == KF_CODE)
			{
				static TKeyboardFunction KeyboardFunctionDetect = KF_CODE;

				if (board.TerminalFunction == 0)
				{
					switch (Am->Data.KeyboardData.KeyCode)
					{
					case 127:
					{
						board.Tick_ESCKey = 0;
						board.SendMessage_FunctionKeyPress(KF_BACKSPACE, 0,NULL,Am->Data.KeyboardData.FromStreamTask);// , & XB_BOARD_DefTask);
						board.TerminalFunction = 0;
						break;
					}
					case 10:
					{
						if (LastKeyCode != 13)
						{
							board.SendMessage_FunctionKeyPress(KF_ENTER, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
							board.TerminalFunction = 0;
						}
						else
						{
							Am->Data.KeyboardData.KeyCode = 0;
						}
						res = true;
						break;
					}
					case 13:
					{
						if (LastKeyCode != 10)
						{
							board.SendMessage_FunctionKeyPress(KF_ENTER, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
							board.TerminalFunction = 0;
						}
						else
						{
							Am->Data.KeyboardData.KeyCode = 0;
						}
						res = true;
						break;
					}
					case 27:
					{
						board.TerminalFunction = 1;
						board.Tick_ESCKey = SysTickCount;
						break;
					}
					case 7:
					{
						board.Tick_ESCKey = 0;
						board.SendMessage_FunctionKeyPress(KF_ESC, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
						break;
					}
					case 9:
					{
						board.Tick_ESCKey = 0;
						board.SendMessage_FunctionKeyPress(KF_TABNEXT, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
						break;
					}
					case 255:
					case 0:
					{
						board.Tick_ESCKey = 0;
						board.TerminalFunction = 0;
						break;
					}


					default:
					{
						board.Tick_ESCKey = 0;
						board.TerminalFunction = 0;
						break;
					}
					}
				}
				else if (board.TerminalFunction == 1)
				{
					if (Am->Data.KeyboardData.KeyCode == 91) // Nadchodzi funkcyjny klawisz
					{
						board.CTRLKey = false;
						board.Tick_ESCKey = 0;
						board.TerminalFunction = 2;
						Am->Data.KeyboardData.KeyCode = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 79) // Nadchodzi funkcyjny klawisz z CTRL
					{
						board.CTRLKey = true;
						board.Tick_ESCKey = 0;
						board.TerminalFunction = 2;
						Am->Data.KeyboardData.KeyCode = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 10)
					{
						board.TerminalFunction = 0;
					}
					else
					{
						board.Tick_ESCKey = 0;
						Am->Data.KeyboardData.KeyCode = 0;
						board.SendMessage_FunctionKeyPress(KF_ESC, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
					}
				}
				else if (board.TerminalFunction == 2)
				{
					if (Am->Data.KeyboardData.KeyCode == 65) // cursor UP
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORUP, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						else board.SendMessage_FunctionKeyPress(KF_CURSORUP, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 66) // cursor DOWN
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORDOWN, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						else board.SendMessage_FunctionKeyPress(KF_CURSORDOWN, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 68) // cursor LEFT
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORLEFT, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						else board.SendMessage_FunctionKeyPress(KF_CURSORLEFT, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 67) // cursor RIGHT
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORRIGHT, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						else board.SendMessage_FunctionKeyPress(KF_CURSORRIGHT, 0, NULL, Am->Data.KeyboardData.FromStreamTask);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 90) // shift+tab
					{
						Am->Data.KeyboardData.KeyCode = 0;
						board.SendMessage_FunctionKeyPress(KF_TABPREV, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 49) // F1
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F1;
						board.TerminalFunction = 3;
					}
					else if (Am->Data.KeyboardData.KeyCode == 50) // INSERT ... F9?
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_INSERT;
						board.TerminalFunction = 5;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) // DELETE
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_DELETE;
						board.TerminalFunction = 4;
					}
					else
					{
						board.TerminalFunction = 0;
					}

				}
				else if (board.TerminalFunction == 3)
				{
					if (Am->Data.KeyboardData.KeyCode == 49) // F1
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F1;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 50) // F2
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F2;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) // F3
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F3;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 52) // F4
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F4;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 53) // F5
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F5;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 55) // F6
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F6;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 56) // F7
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F7;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 57) // F8
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F8;
						board.TerminalFunction = 4;
					}
					else
					{
						board.TerminalFunction = 0;
					}
				}
				else if (board.TerminalFunction == 5)
				{

					if (Am->Data.KeyboardData.KeyCode == 126) // INSERT , ...
					{
						Am->Data.KeyboardData.KeyCode = 0;
						board.SendMessage_FunctionKeyPress(KeyboardFunctionDetect, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//,NULL &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 48) // F9
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F9;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 49) // F10
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F10;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) // F11
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F11;
						board.TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 52) // F12
					{
						Am->Data.KeyboardData.KeyCode = 0;
						KeyboardFunctionDetect = KF_F12;
						board.TerminalFunction = 4;
					}
					else
					{
						board.TerminalFunction = 0;
					}

				}
				else if (board.TerminalFunction == 4)
				{
					if (Am->Data.KeyboardData.KeyCode == 126)
					{
						Am->Data.KeyboardData.KeyCode = 0;
						board.SendMessage_FunctionKeyPress(KeyboardFunctionDetect, 0, NULL, Am->Data.KeyboardData.FromStreamTask);//,NULL &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
					}
					else
					{
						board.TerminalFunction = 0;
					}
				}
				else
				{
					board.TerminalFunction = 0;
				}

				res = true;

			}
#ifdef XB_GUI
			else if (Am->Data.KeyboardData.KeyFunction == KF_F1)
			{
				board.EnableTXStream(Am->Data.KeyboardData.FromStreamTask);
				xb_board_ConsoleInWindow=xb_board_CFG_ConsoleInWindow;
				xb_board_winHandle0 = GUI_WindowCreate(&XB_BOARD_DefTask, 0);
				GUI_Show();

				res = true;
			}
			else if (Am->Data.KeyboardData.KeyFunction == KF_F12)
			{
				board.DisableTXStream(Am->Data.KeyboardData.FromStreamTask);
				if (board.SumEnableTXStream() == 0)
				{
					GUI_Hide();
				}
				res = true;
			}
			else if (Am->Data.KeyboardData.KeyFunction == KF_ENTER)
			{
				if (xb_board_winHandle0 != NULL)
				{
					if (GUI_FindWindowByActive() == xb_board_winHandle0)
					{
						TTask* task = board.GetTaskByIndex(xb_board_currentselecttask);
						if (task != NULL)
						{
							Am->Data.KeyboardData.KeyFunction = KF_NONE;
							GUIGADGET_OpenMainMenu(task->TaskDef, WINDOW_POS_LAST_RIGHT_ACTIVE, xb_board_winHandle0->Y + xb_board_currentYselecttask + 1);

						}
					}
				}
				res = true;
			}
			else if (Am->Data.KeyboardData.KeyFunction == KF_CURSORDOWN)
			{
				if (xb_board_winHandle0 != NULL)
				{
					if (GUI_FindWindowByActive() == xb_board_winHandle0)
					{
						uint8_t l = xb_board_currentselecttask;
						xb_board_currentselecttask++;
						if (xb_board_currentselecttask >= board.TaskList_count)
						{
							xb_board_currentselecttask = board.TaskList_count - 1;
						}
						else
						{
							xb_board_listtask_repaint = true;
							xb_board_winHandle0->RepaintDataCounter++;
						}
						TTask* t = board.GetTaskByIndex(l);
						if (t != NULL) t->LastSumTaskStatusText = 0;
						t = board.GetTaskByIndex(xb_board_currentselecttask);
						if (t != NULL) t->LastSumTaskStatusText = 0;
					}
				}
				res = true;
			}
			else if (Am->Data.KeyboardData.KeyFunction == KF_CURSORUP)
			{
				if (xb_board_winHandle0 != NULL)
				{
					if (GUI_FindWindowByActive() == xb_board_winHandle0)
					{
						uint8_t l = xb_board_currentselecttask;

						if (xb_board_currentselecttask > 0)
						{
							xb_board_currentselecttask--;
							xb_board_listtask_repaint = true;
							xb_board_winHandle0->RepaintDataCounter++;
						}

						TTask* t = board.GetTaskByIndex(l);
						if (t != NULL) t->LastSumTaskStatusText = 0;
						t = board.GetTaskByIndex(xb_board_currentselecttask);
						if (t != NULL) t->LastSumTaskStatusText = 0;

					}
				}
				res = true;
			}

#endif
			LastKeyCode = Am->Data.KeyboardData.KeyCode;

		}
		break;
	}
#ifdef XB_GUI
	case IM_MENU:
	{
		OPEN_MAINMENU()
		{
			xb_board_menuHandle1 = GUIGADGET_CreateMenu(&XB_BOARD_DefTask, 1, false, X, Y);
		}

		BEGIN_MENU(1, "XBBOARD MENU", WINDOW_POS_X_DEF, WINDOW_POS_Y_DEF, 48, MENU_AUTOCOUNT, 0, true)
		{
			BEGIN_MENUITEM_CHECKED("Show GUI on start", taLeft, xb_board_ShowGuiOnStart)
			{
				CLICK_MENUITEM()
				{
					xb_board_ShowGuiOnStart = !xb_board_ShowGuiOnStart;
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM_CHECKED("CONSOLE IN WINDOW", taLeft, xb_board_ConsoleInWindow)
			{
				CLICK_MENUITEM()
				{
					xb_board_ConsoleInWindow = !xb_board_ConsoleInWindow;
				}
			}
			END_MENUITEM()

			BEGIN_MENUITEM("CONSOLE WIDTH [" + String(xb_board_ConsoleWidth) + "]", taLeft)
			{
				CLICKRIGHT_MENUITEM()
				{
					xb_board_ConsoleWidth = xb_board_ConsoleWidth >= 160 ? 160 : xb_board_ConsoleWidth + 1;
				}
				CLICKLEFT_MENUITEM()
				{
					xb_board_ConsoleWidth = xb_board_ConsoleWidth > 80 ? xb_board_ConsoleWidth - 1 : 80;
				}
			}
			END_MENUITEM()

			BEGIN_MENUITEM("CONSOLE HEIGHT [" + String(xb_board_ConsoleHeight) + "]", taLeft)
			{
				CLICKRIGHT_MENUITEM()
				{
					xb_board_ConsoleHeight = xb_board_ConsoleHeight >= 50 ? 50 : xb_board_ConsoleHeight + 1;
				}
				CLICKLEFT_MENUITEM()
				{
					xb_board_ConsoleHeight = xb_board_ConsoleHeight > 25 ? xb_board_ConsoleHeight - 1 : 25;
				}
			}
			END_MENUITEM()

			BEGIN_MENUITEM_CHECKED("LOG put to Serial.", taLeft, xb_board_Consoleputtoserial)
			{
				CLICK_MENUITEM()
				{
					xb_board_Consoleputtoserial = !xb_board_Consoleputtoserial;
				}
			}
			END_MENUITEM()

			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM_CHECKED("Show list Far Device ID", taLeft, xb_board_ShowListFarDeviceID)
			{
				CLICK_MENUITEM()
				{
					xb_board_ShowListFarDeviceID = !xb_board_ShowListFarDeviceID;
					if (xb_board_ShowListFarDeviceID)
					{
						XB_BOARD_Show_TFarDeviceID();
					}
					else
					{
						XB_BOARD_Close_TFarDeviceID();
					}
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Reset list Far Device ID", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.ResetCfgFarDevices();
					board.EraseAllFarDevices();
					board.LoadCfgFarDevices();
					XB_BOARD_Repaint_TFarDeviceID();
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("List var & value in system...", taLeft)
			{
				CLICK_MENUITEM()
				{
					TTask* t = board.TaskList;
					int count = 0;
					board.Log("- List variable in system -", true, true, tlInfo);
					while (t != NULL)
					{
						count = board.SendMessage_GetVarCount(t);
						if (count > 0)
						{
							String s = "";
							String sv = "";
							board.SendMessage_GetTaskNameString(t, &s);
							board.Log(String("\n\nTask "+s + " vars count "+String(count)).c_str(), false, false, tlWarn);
							for (int i = 0; i < count; i++)
							{	
								board.Log(String("\n"+String(i)+". %").c_str());
								s=board.SendMessage_GetVarName(i, t);
								board.Log(String(s+"% = '").c_str());
								sv = board.SendMessage_GetVarValue(s, t);
								board.Log(String(sv + "'").c_str());
								sv = "";
								sv = board.SendMessage_GetVarDescription(s, t);
								if (sv!="") board.Log(String("  ["+sv + "]").c_str());

								
							}
						}
						t = t->Next;
					}



				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM_CHECKED("Auto check heap Integrity", taLeft, board.AutoCheckHeapIntegrity)
			{
				CLICK_MENUITEM()
				{
					board.AutoCheckHeapIntegrity = !board.AutoCheckHeapIntegrity;
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			BEGIN_MENUITEM("Device Name [" + board.DeviceName + "]", taLeft)
			{
				CLICK_MENUITEM()
				{
					xb_board_inputdialog0 = GUIGADGET_CreateInputDialog(&XB_BOARD_DefTask, 0, true);
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("Project Name [" + board.ProjectName + "]", taLeft)
			{
				CLICK_MENUITEM()
				{
				}
			}
			END_MENUITEM()
				SEPARATOR_MENUITEM()
				CONFIGURATION_MENUITEMS()
				BEGIN_MENUITEM("Save all configuration", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.AllSaveConfiguration();
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM("Save all configuration & Soft RESET", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.AllSaveConfiguration();
					board.SoftResetMCU(true);
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM("Reset all configuration", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.AllResetConfiguration();
				}
			}
			END_MENUITEM()
				BEGIN_MENUITEM("Reset all configuration & Soft RESET", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.AllResetConfiguration();
					board.SoftResetMCU(true);
				}
			}
			END_MENUITEM()
				SEPARATOR_MENUITEM()
				BEGIN_MENUITEM("SOFT RESET MCU", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.SoftResetMCU(true);
					return true;
				}
			}
			END_MENUITEM()

		}
		END_MENU()


			res = true;
		break;
	}
	case IM_INPUTDIALOG:
	{
		BEGIN_DIALOG(0, "Device name", "input device name", tivString, 32, &board.DeviceName)
		{
		}
		END_DIALOG()

			res = true;
		break;
	}
	case IM_WINDOW:
	{
		BEGIN_WINDOW_DEF(2, "List Far DeviceID", 20, 0, 100, board.FarDeviceIDList_count + 3, xb_board_winHandle2)
		{
			static Ty YRow;
			REPAINT_WINDOW()
			{
				YRow = 0;
				WH->BeginDraw();
				WH->SetTextColor(tfcMagenta);
				XB_BOARD_Captions_TFarDeviceID(WH, YRow);
				WH->EndDraw(false);
			}
			REPAINTDATA_WINDOW()
			{
				TFarDeviceID *fdi = board.FarDeviceIDList;
				Ty _Row = YRow;
				WH->BeginDraw();
				WH->SetTextColor(tfcYellow);
				while (fdi != NULL)
				{
					XB_BOARD_RowDraw_TFarDeviceID(WH, fdi, _Row);
					fdi = fdi->Next;
				}
				WH->EndDraw(false);
			}

			DESTROY_WINDOW()
			{
				xb_board_ShowListFarDeviceID = false;
			}

		}
		END_WINDOW_DEF()


		BEGIN_WINDOW_DEF(1, "CONSOLE LOG", 49, 0, xb_board_ConsoleWidth + 2, xb_board_ConsoleHeight + 2, xb_board_winHandle1)
		{
			REPAINT_WINDOW()
			{
			}
			REPAINTDATA_WINDOW()
			{
				if (board.ConsoleScreen != NULL)
				{
					uint8_t ch, c, lc = 5;
					WH->BeginDraw();
					WH->GoToXY(0, 0);
					WH->TextWordWrap = true;
					for (uint32_t i = 0; i < (board.ConsoleScreen->Width * board.ConsoleScreen->Height); i++)
					{
						c = board.ConsoleScreen->Get_ColorBuf(i);
						if (c != lc)
						{
							lc = c;
							switch (c)
							{
							case 0: WH->SetTextColor(tfcWhite); break;
							case 1: WH->SetTextColor(tfcYellow); break;
							case 2: WH->SetTextColor(tfcRed); break;
							default: WH->SetTextColor(tfcWhite); break;
							}
						}
						ch = board.ConsoleScreen->Buf[i];
						if (ch == 0) ch = 32;
						WH->PutChar(ch);
					}

					WH->EndDraw(false);
				}
			}

			DESTROY_WINDOW()
			{
				xb_board_ConsoleInWindow = false;
			}

		}
		END_WINDOW_DEF()

		BEGIN_WINDOW_DEF(0, WINDOW_0_CAPTION, 0, 0, 48, WINDOW_0_HEIGHT, xb_board_winHandle0)
		{
			//--------------------------------
			REPAINT_WINDOW()
			{
				int y = 0;
				String name;

				WH->BeginDraw();
				if (!xb_board_listtask_repaint)
				{
					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("DEVICE NAME: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceName.c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("DEVICE VERSION: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceVersion.c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("DEVICE ID: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceIDtoString(board.DeviceID).c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("TIME FROM RUN:"));
					y++;
					WH->PutStr(0, y, ("FREE HEAP:"));
					WH->PutStr(WH->Width - 18, y, ("MIN HEAP:"));
					y++;
					WH->PutStr(WH->Width - 18, y, ("MAX HEAP:"));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(WH->Width - 9, y, String(board.MaximumFreeHeapInLoop).c_str());

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("MEM USE:"));
					y++;
#ifdef BOARD_HAS_PSRAM
					WH->PutStr(0, y, ("FREEpsram:"));
					WH->PutStr(WH->Width - 18, y, ("MINpsram:"));
					y++;
					WH->PutStr(WH->Width - 18, y, ("MAXpsram:"));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(WH->Width - 9, y, String(board.MaximumFreePSRAMInLoop).c_str());

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, ("MEM USE:"));
					y++;
#endif
#ifdef XB_PREFERENCES
					WH->PutStr(0, y, "PREFERENCES FREE ENTRIES:");
#else
					WH->PutStr(0, y, "PREFERENCES NOT USE");
#endif
					WH->PutStr(32, y, "CPU CLK:");
					y++;

					WH->PutStr(0, y, ("OUR RESERVED BLOCK:"));

#ifdef ARDUINO_ESP32C3_DEV
					WH->PutStr(29, y, "CPU Temp.:---");
#else
					WH->PutStr(29, y, "CPU Temp.:");
#endif
					y++;
					//--------------

					WH->PutStr(0, y, "_", WH->Width, '_');
					y++;
					WH->PutStr(0, y, ("TASK NAME"));
					WH->PutStr(15, y, ("STATUS"));

					y++;
				}
				else
				{
					y += 9;
#ifdef BOARD_HAS_PSRAM
					y += 2;
#endif
				}

				WH->SetTextColor(tfcGreen);
				xb_board_listtask_repaint = false;
				for (int i = 0; i < board.TaskList_count; i++)
				{

					if (xb_board_currentselecttask == i)
					{
						WH->SetTextColor(tfcYellow);
						WH->SetReverseChar();
					}
					else
					{
						WH->SetTextColor(tfcGreen);
						WH->SetNormalChar();
					}

					TTask* t = board.GetTaskByIndex(i);
					if (t != NULL)
					{

						if (board.SendMessage_GetTaskNameString(t->TaskDef, &name))
						{
							WH->PutStr(0, y + i, name.c_str(), 13, ' ');
							WH->PutStr(String(t->TaskDef->Priority).c_str());
							if (GUIGADGET_IsMainMenu(t->TaskDef)) WH->PutChar('>');
							else WH->PutChar(' ');
							name = "";
						}
						t->LastSumTaskStatusText = 0;
						t->LastBoldTaskStatus = false;
					}
				}
				WH->EndDraw();
			}
			//--------------------------------
			REPAINTDATA_WINDOW()
			{
				int y = 3;
				WH->BeginDraw();

				WH->SetBoldChar();
				WH->SetTextColor(tfcYellow);
				{
					String txttime = "";
					GetTimeIndx(txttime, DateTimeUnix - DateTimeStart);
					WH->PutStr(14, y, txttime.c_str());
				}
				y++;
				WH->PutStr(10, y, String(String(board.FreeHeapInLoop) + " ("+String(board.FreeMaxAllocInLoop)+")").c_str());
				WH->PutChar(' ');
				WH->PutStr(WH->Width - 9, y, String(board.MinimumFreeHeapInLoop).c_str());
				WH->PutChar(' ');
				y++;
				WH->PutStr(9, y, String((uint32_t)(100 - (board.FreeHeapInLoop / (board.MaximumFreeHeapInLoop / 100L)))).c_str());
				WH->PutStr(("% "));
				y++;

#ifdef BOARD_HAS_PSRAM
				WH->PutStr(10, y, String(board.FreePSRAMInLoop).c_str());
				WH->PutChar(' ');
				WH->PutStr(WH->Width - 9, y, String(board.MinimumFreePSRAMInLoop).c_str());
				WH->PutChar(' ');
				y++;
				WH->PutStr(9, y, String((uint32_t)(100 - (board.FreePSRAMInLoop / (board.MaximumFreePSRAMInLoop / 100L)))).c_str());
				WH->PutStr(("% "));
				y++;
#endif
#ifdef XB_PREFERENCES
				WH->PutStr(26, y, String(board.preferences_freeEntries).c_str(), 6);
#endif
				WH->PutStr(40, y, String(String(ESP.getCpuFreqMHz()) + "Mhz").c_str());
				y++;


#ifdef XB_BOARD_MEMDEBUG
				WH->PutStr(20, y, String(MemDebugList_count).c_str(), 8);
#else
				WH->PutStr(20, y, String(OurReservedBlock).c_str(), 8);
#endif

#ifdef ARDUINO_ESP32C3_DEV
				y++;
#else
				WH->PutStr(39, y, String(String(temperatureRead(), 2) + "C").c_str());
				y++;
#endif

				String name;
				name.reserve(80);
				y += 2;


				for (int i = 0; i < board.TaskList_count; i++)
				{
					if (xb_board_currentselecttask == i)
					{
						WH->SetTextColor(tfcYellow);
						WH->SetReverseChar();
						xb_board_currentYselecttask = y + i;
					}
					else
					{
						WH->SetNormalChar();

					}
					TTask* t = board.GetTaskByIndex(i);

					if (xb_board_listtask_repaint)
					{
						if (xb_board_currentselecttask == i)
						{
							WH->SetTextColor(tfcYellow);
						}
						else
						{
							WH->SetTextColor(tfcGreen);
						}

						if (t != NULL)
						{

							if (board.SendMessage_GetTaskNameString(t->TaskDef, &name))
							{
								WH->PutStr(0, y + i, name.c_str(), 13, ' ');
								WH->PutStr(String(t->TaskDef->Priority).c_str());
								if (GUIGADGET_IsMainMenu(t->TaskDef)) WH->PutChar('>');
								else WH->PutChar(' ');
								name = "";
							}
							//t->LastSumTaskStatusText = 0;
						}
						WH->SetTextColor(tfcYellow);
					}

					if (t != NULL)
					{
						if (board.SendMessage_GetTaskStatusString(t->TaskDef, &name))
						{
							uint32_t sum = 0;
							uint32_t l = name.length();
							for (uint32_t t = 0; t < l; t++) sum += (uint32_t)name[t];
							if (t->LastSumTaskStatusText != sum)
							{
								t->LastSumTaskStatusText = sum;
								t->LastBoldTaskStatus = true;


								if (xb_board_currentselecttask == i)
								{
									WH->SetTextColor(tfcYellow);
									WH->SetReverseChar();
								}
								else
								{
									WH->SetNormalChar();
								}
								WH->SetBoldChar();
								WH->PutStr(15, y + i, name.c_str(), 33, ' ');
							}
							else
							{
								if (t->LastBoldTaskStatus)
								{
									t->LastBoldTaskStatus = false;
									if (xb_board_currentselecttask == i)
									{
										WH->SetTextColor(tfcYellow);
										WH->SetReverseChar();
									}
									else
									{
										WH->SetNormalChar();
									}
									WH->PutStr(15, y + i, name.c_str(), 33, ' ');
								}
							}
							name = "";
						}
					}
				}
				xb_board_listtask_repaint = false;
				WH->EndDraw();
			}
			//--------------------------------
			DESTROY_WINDOW()
			{
				xb_board_ShowGuiOnStart = false;
			}
		}
		//--------------------------------
		END_WINDOW_DEF()

			res = true;
		break;
	}
#endif
	case IM_VAR:
	{
		VAR("devicename")
		{
			GET_VAR
			{
				VALUE_VAR = board.DeviceName;
				return true;
			}
			GET_DESC
			{
				VALUE_VAR = "Device Name";
				return true;
			}
		}
		EVAR

		return false;
	}
	default:;
	}

	return res;
}

TTaskDef XB_BOARD_DefTask = { 0,&XB_BOARD_Setup,&XB_BOARD_DoLoop,&XB_BOARD_DoMessage };

#pragma endregion
