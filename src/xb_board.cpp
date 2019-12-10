//#define BOARD_BETA_DEBUG
#pragma region INCLUDES
#include <xb_board.h>

// Dla ESP8266 inkludy z funkcjami jêzyka C
#ifdef ESP8266
extern "C" {
#include "user_interface.h"	
}
#endif

#ifdef ESP32
#include <stddef.h>
#endif


// Dla STM32 inkludy z funkcjami jêzyka C
#ifdef ARDUINO_ARCH_STM32
extern "C" {
#include <string.h>
#include <stdlib.h>
}
#endif

#ifdef XB_PREFERENCES
#ifdef ESP32
#include <Preferences.h>
Preferences xbpreferences;
#else
#error "XB_PREFERENCES not support"
#endif
#endif

#ifdef XB_GUI
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>
#endif

#pragma endregion
#pragma region GLOBAL_VARS
// Podstawowy obiekt tzw. "kernel"
TXB_board board; 

// Struktura nag³ówkowa zadania obiektu podstawowego.
uint32_t XB_BOARD_DoLoop(void);
void XB_BOARD_Setup(void);
bool XB_BOARD_DoMessage(TMessageBoard *Am);
TTaskDef XB_BOARD_DefTask = {0,&XB_BOARD_Setup,&XB_BOARD_DoLoop,&XB_BOARD_DoMessage};

// Data i Czas w formacie Unix
volatile uint32_t DateTimeUnix;
// Sekundy które up³ynê³y od startu urz¹dzenia
volatile uint32_t DateTimeStart;
// SysTick na ESP8266
#if defined(ESP8266) 
volatile uint32_t __SysTickCount;
#endif

// Zmienne i funkcjonalnoœæ GUI do zadania systemowego
#ifdef XB_GUI

#ifdef __riscv64
#define WINDOW_0_CAPTION "BOARD (Sipeed Maix), 400Mhz)";
#endif
#ifdef ESP8266
#define WINDOW_0_CAPTION "BOARD (ESP8266, 160Mhz)";
#endif
#ifdef ESP32
#ifdef BOARD_HAS_PSRAM
#define WINDOW_0_CAPTION "BOARD (ESP32 wROVER, 240Mhz)";
#else
#define WINDOW_0_CAPTION "BOARD (ESP32, 240Mhz)";
#endif
#endif
#ifdef ARDUINO_ARCH_STM32
#define WINDOW_0_CAPTION "BOARD (STM32)";
#endif

#ifdef BOARD_HAS_PSRAM
#define WINDOW_0_HEIGHT board.TaskCount + 13
#else
#define WINDOW_0_HEIGHT board.TaskCount + 11
#endif

TWindowClass * xb_board_winHandle0;
uint8_t xb_board_currentselecttask = 0;
uint8_t xb_board_currentYselecttask;
bool xb_board_listtask_repaint = false;

TGADGETMenu *xb_board_menuHandle1;
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
#pragma region FUNKCJE_SETUP_LOOP_MESSAGES
// -------------------------------------

// -------------------------------------
// Procedura inicjuj¹ca zadanie g³ównego
void XB_BOARD_Setup(void)
{
	board.LoadConfiguration(&XB_BOARD_DefTask);
	board.AddGPIODrive(BOARD_NUM_DIGITAL_PINS,&XB_BOARD_DefTask,"ESP32");
	board.SetDigitalPinCount(BOARD_NUM_DIGITAL_PINS);

	// Jeœli framework zosta³ uruchomiony na ESP32 z dodatkow¹ pamiêci¹ RAM SPI , rezerwacja pierwszych 2 megabajtów w celu unikniêcia b³ednej wspó³pracy ESP32 z PSRAM
#ifdef ESP32
#ifdef PSRAM_BUG
#ifdef BOARD_HAS_PSRAM
			board.psram2m = heap_caps_malloc((1024 * 1024) * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
#endif
#endif
#endif

	// Jeœli framework zosta³ uruchomiony na ESP8266 to inicjacja liczników czasowych
#if defined(ESP8266)
	board.SysTickCount_init();
	board.DateTimeSecond_init();
#endif
	// Zerowanie liczników czasowych
	DateTimeUnix = 0;
	DateTimeStart = 0;

	// Skonfigurowanie pinu (jeœli podano) informuj¹cego na zewn¹trz ¿e aplikacja dzia³a
#ifdef BOARD_LED_LIFE_PIN
	board.pinMode(BOARD_LED_LIFE_PIN, OUTPUT);
#endif

	// Skonfigurowanie pinu (jeœli podano) informuj¹cego na zewn¹trz ¿e nast¹pi³a transmisja danych na zewn¹trz
#ifdef BOARD_LED_TX_PIN
	board.Tick_TX_BLINK = 0;
	board.pinMode(BOARD_LED_TX_PIN, OUTPUT);
#if defined(BOARD_LED_TX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_TX_PIN, (BOARD_LED_TX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_TX_PIN, LOW);
#endif
#endif

	// Skonfigurowanie pinu (jeœli podano) informuj¹cego na zewn¹trz ¿e nast¹pi³a transmisja danych z zewn¹trz
#ifdef BOARD_LED_RX_PIN
	board.Tick_RX_BLINK = 0;
	board.pinMode(BOARD_LED_RX_PIN, OUTPUT);
#if defined(BOARD_LED_RX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_RX_PIN, (BOARD_LED_RX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_RX_PIN, LOW);
#endif
#endif

	// Ustawienie czasu jak d³ugo ma utrzymywaæ siê stan 1 na pinach informuj¹cych na zew¹trz o transmisji danych
#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
#ifdef BOARD_LED_RXTX_BLINK_TICK
	board.TickEnableBlink = BOARD_LED_RXTX_BLINK_TICK;
#else
	board.TickEnableBlink = 250;
#endif
#endif

	// Jeœli interface uruchomiony to dodanie zadania obs³uguj¹cego interface GUI
#ifdef XB_GUI
	board.AddTask(&XB_GUI_DefTask);
#endif
}
// -----------------------------
// G³ówna pêtla zadania g³ównego
// <-      = 0 - Zadanie dzia³a wed³ug harmonogramu priorytetów
//         > 0 - Zadanie zostanie uruchomione po iloœci milisekund zwrócnej w rezultacie
uint32_t XB_BOARD_DoLoop(void)
{
	// Zamiganie
	board.handle();
	return 0;
}
// ---------------------------------
// Obs³uga messagów zadania g³ównego
// -> Am - WskaŸnik na strukture messaga
// <-      = true - messag zosta³ obs³u¿ony
//         = false - messag nie obs³ugiwany przez zadanie
bool XB_BOARD_DoMessage(TMessageBoard *Am)
{
	bool res = false;
	static uint8_t LastKeyCode = 0;
	
	switch (Am->IDMessage)
	{
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		FREEPTR(xb_board_winHandle0);
		FREEPTR(xb_board_menuHandle1);
#endif
		res = true;
		break;
	}
	case IM_LOAD_CONFIGURATION:
	{
		res = true;
		break;
	}
	case IM_SAVE_CONFIGURATION:
	{
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
						board.SendMessage_FunctionKeyPress(KF_BACKSPACE, 0);// , & XB_BOARD_DefTask);
						board.TerminalFunction = 0;
						break;
					}
					case 10:
					{
						if (LastKeyCode != 13)
						{
							board.SendMessage_FunctionKeyPress(KF_ENTER, 0);//, &XB_BOARD_DefTask);
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
							board.SendMessage_FunctionKeyPress(KF_ENTER, 0);//, &XB_BOARD_DefTask);
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
						board.SendMessage_FunctionKeyPress(KF_ESC, 0);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
						break;
					}
					case 9:
					{
						board.Tick_ESCKey = 0;
						board.SendMessage_FunctionKeyPress(KF_TABNEXT, 0);//, &XB_BOARD_DefTask);
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
						board.SendMessage_FunctionKeyPress(KF_ESC, 0);//, &XB_BOARD_DefTask);
						board.TerminalFunction = 0;
					}
				}
				else if (board.TerminalFunction == 2)
				{
					if (Am->Data.KeyboardData.KeyCode == 65) // cursor UP
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORUP, 0);
						else board.SendMessage_FunctionKeyPress(KF_CURSORUP, 0);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 66) // cursor DOWN
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORDOWN, 0);
						else board.SendMessage_FunctionKeyPress(KF_CURSORDOWN, 0);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 68) // cursor LEFT
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORLEFT, 0);
						else board.SendMessage_FunctionKeyPress(KF_CURSORLEFT, 0);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 67) // cursor RIGHT
					{
						if (board.CTRLKey)  board.SendMessage_FunctionKeyPress(KF_CTRL_CURSORRIGHT, 0);
						else board.SendMessage_FunctionKeyPress(KF_CURSORRIGHT, 0);
						Am->Data.KeyboardData.KeyCode = 0;
						board.TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 90) // shift+tab
					{
						Am->Data.KeyboardData.KeyCode = 0;
						board.SendMessage_FunctionKeyPress(KF_TABPREV, 0);//, &XB_BOARD_DefTask);
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
						board.SendMessage_FunctionKeyPress(KeyboardFunctionDetect, 0);//,NULL &XB_BOARD_DefTask);
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
						board.SendMessage_FunctionKeyPress(KeyboardFunctionDetect, 0);//,NULL &XB_BOARD_DefTask);
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
				GUI_Show();
				xb_board_winHandle0 = GUI_WindowCreate(&XB_BOARD_DefTask, 0);
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
							GUIGADGET_OpenMainMenu(task->TaskDef, WINDOW_POS_LAST_RIGHT_ACTIVE, xb_board_winHandle0->Y+xb_board_currentYselecttask + 1);

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
						xb_board_currentselecttask++;
						if (xb_board_currentselecttask >= board.TaskCount)
						{
							xb_board_currentselecttask = board.TaskCount - 1;
						}
						else
						{
							xb_board_listtask_repaint = true;
							xb_board_winHandle0->RepaintDataCounter++;
						}
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
						if (xb_board_currentselecttask > 0)
						{
							xb_board_currentselecttask--;
							xb_board_listtask_repaint = true;
							xb_board_winHandle0->RepaintDataCounter++;
						}
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
			xb_board_menuHandle1 = GUIGADGET_CreateMenu(&XB_BOARD_DefTask, 1, false, X,Y);
		}

		BEGIN_MENU(1, "XBBOARD MENU", WINDOW_POS_X_DEF, WINDOW_POS_Y_DEF, 32, MENU_AUTOCOUNT, 0, true)
		{
			BEGIN_MENUITEM("SOFT RESET MCU", taLeft)
			{
				CLICK_MENUITEM()
				{
#if defined(ESP8266) || defined(ESP32)
					ESP.restart();
					delay(5000);
#elif defined(ARDUINO_ARCH_STM32)
					board.Log("Reset no support!", true, true, tlError);
#else
					board.Log("Reset no support!", true, true, tlError);
#endif
					return true;
				}
			}
			END_MENUITEM()
			BEGIN_MENUITEM("SAVE ALL CONFIGURATION", taLeft)
			{
				CLICK_MENUITEM()
				{
					board.SendMessage_ConfigSave();
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			CONFIGURATION_MENUITEMS()
		}
		END_MENU()


		res = true;
		break;
	}
	case IM_WINDOW:
	{
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
					WH->PutStr(0, y, FSS("DEVICE NAME: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceName.c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("DEVICE VERSION: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceVersion.c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("DEVICE ID: "));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(board.DeviceIDtoString(board.DeviceID).c_str());
					y++;

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("TIME FROM RUN:"));
					y++;
					WH->PutStr(0, y, FSS("FREE HEAP:"));
					WH->PutStr(WH->Width - 18, y, FSS("MIN HEAP:"));
					y++;
					WH->PutStr(WH->Width - 18, y, FSS("MAX HEAP:"));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(WH->Width - 9, y, String(board.MaximumFreeHeapInLoop).c_str());

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("MEM USE:"));
					y++;
#ifdef BOARD_HAS_PSRAM
					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("FREEpsram:"));
					WH->PutStr(WH->Width - 18, y, FSS("MINpsram:"));
					y++;
					WH->PutStr(WH->Width - 18, y, FSS("MAXpsram:"));
					WH->SetBoldChar();
					WH->SetTextColor(tfcYellow);
					WH->PutStr(WH->Width - 9, y, String(board.MaximumFreePSRAMInLoop).c_str());

					WH->SetNormalChar();
					WH->SetTextColor(tfcWhite);
					WH->PutStr(0, y, FSS("MEM USE:"));
					y++;
#endif
					WH->PutStr(0, y, FSS("OUR RESERVED BLOCK:"));
					y++;
					//--------------

					WH->PutStr(0, y, "_", WH->Width, '_');
					y++;
					WH->PutStr(0, y, FSS("TASK NAME"));
					WH->PutStr(15, y, FSS("STATUS"));

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
				for (int i = 0; i < board.TaskCount; i++)
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

						if (board.SendMessage_GetTaskNameString(t->TaskDef, name))
						{
							WH->PutStr(0, y + i, name.c_str(), 14, ' ');
							if (GUIGADGET_IsMainMenu(t->TaskDef)) WH->PutChar('>');
							else WH->PutChar(' ');
							name = "";
						}
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
					cbufSerial cbuf(32);
					board.PrintTimeFromRun(&cbuf);
					WH->PutStr(14, y, cbuf.readString().c_str());
				}
				y++;
				WH->PutStr(10, y, String(board.FreeHeapInLoop).c_str());
				WH->PutChar(' ');
				WH->PutStr(WH->Width - 9, y, String(board.MinimumFreeHeapInLoop).c_str());
				WH->PutChar(' ');
				y++;
				WH->PutStr(9, y, String((uint32_t)(100 - (board.FreeHeapInLoop / (board.MaximumFreeHeapInLoop / 100L)))).c_str());
				WH->PutStr(FSS("% "));
				y++;

#ifdef BOARD_HAS_PSRAM
				WH->PutStr(10, y, String(board.FreePSRAMInLoop).c_str());
				WH->PutChar(' ');
				WH->PutStr(WH->Width - 9, y, String(board.MinimumFreePSRAMInLoop).c_str());
				WH->PutChar(' ');
				y++;
				WH->PutStr(9, y, String((uint32_t)(100 - (board.FreePSRAMInLoop / (board.MaximumFreePSRAMInLoop / 100L)))).c_str());
				WH->PutStr(FSS("% "));
				y++;
#endif
				WH->PutStr(20, y, String(board.OurReservedBlock).c_str(), 8);
				y++;

				String name;
				y += 2;


				for (int i = 0; i < board.TaskCount; i++)
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

							if (board.SendMessage_GetTaskNameString(t->TaskDef, name))
							{
								WH->PutStr(0, y + i, name.c_str(), 14, ' ');
								if (GUIGADGET_IsMainMenu(t->TaskDef)) WH->PutChar('>');
								else WH->PutChar(' ');
								name = "";
							}
						}
						WH->SetTextColor(tfcYellow);
					}

					if (t != NULL)
					{

						if (board.SendMessage_GetTaskStatusString(t->TaskDef, name))
						{
							WH->PutStr(15, y + i, name.c_str(), 33, ' ');
							name = "";
						}
					}
				}
				xb_board_listtask_repaint = false;
				WH->EndDraw();
			}
		}
		//--------------------------------
		END_WINDOW_DEF()

		res = true;
		break;
	}
#endif
	case IM_GET_TASKNAME_STRING:
	{
		*(Am->Data.PointerString) = FSS("BOARD");
		res = true;
		break;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		*(Am->Data.PointerString) = String("TC:" + String(board.TaskCount));
#ifdef PSRAM_BUG
		*(Am->Data.PointerString) += String(" / GFPEC:" + String(board.GETFREEPSRAM_ERROR_COUNTER));
#endif
		res = true;
	}
	case IM_GET_VAR_VALUE:
	{
		if (*Am->Data.VarValueData.VarName == "devicename")
		{
			*Am->Data.VarValueData.VarValue = board.DeviceName;
			res = true;
		}
		break;
	}
	default:;
	}
	
	return res;
}
#pragma endregion
#pragma region FUNKCJE_INICJUJACE_TXB_BOARD
TXB_board::TXB_board()
{
	// Reset zmiennych
	Tick_ESCKey = 0;
	board.TerminalFunction = 0;
	TaskList = NULL;
	TaskCount = 0;
#ifdef PSRAM_BUG
	lastfreepsram = 0;
	GETFREEPSRAM_ERROR_COUNTER = 0;
	MaximumMallocPSRAM = 0;
#endif
	OurReservedBlock = 0;
	iteratetask_procedure = false;
	setup_procedure = false;
	CurrentIterateTask = NULL;
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
	HDFT_ResponseItemList = NULL;
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
		crc = pgm_read_byte(dscrc_table + (crc ^ *addr++));
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
						mb.IDMessage = IM_GET_VAR_VALUE;
						mb.fromTask = NULL;
						mb.Data.VarValueData.VarName = &varname;
						mb.Data.VarValueData.VarValue = &varvalue;
						result = DoMessageOnAllTask(&mb, true, doFORWARD);
						if (result)
						{
								// Wstaw zawartoœæ
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
						 // Anuluj nazwe zmiennej bo koniec Ÿród³a
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
String TXB_board::DeviceIDtoString(TUniqueID Adevid)
{
	char strid[25];
	xb_memoryfill(strid, 25, 0);
	uint8tohexstr(strid, (uint8_t *)&Adevid, 8, ':');

	return String(strid);
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
// Ustalenie iloœci pinów w systemie
// -> Acount nowa iloœæ pinów
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
// Jeœli funkcja zostanie wywo³ana w funkcji setup() zadania to zostan¹ wyœwietlone komunikaty o kolizji u¿ytecznoœci pinu 
// -> Anumpin    - Numer pinu
// -> Afunction  - Numer funkcji jak¹ spe³nia pin
// -> Amode      - Tryb w jakim pin funkcjonuje
// -> Alogwarn   = true - Wyœwietla ostrze¿enia kolizji
//               = false - Nie wyœwietla ostrze¿enia kolizji
// <-            = true - wykonane z powodzeniem, ale kolizja mo¿e zaistnieæ
//               = false - b³¹d, pin nie obs³ugiwany
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
// -> value - wartoœæ stanu
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
// Wys³anie messaga informuj¹cego ¿e urz¹dzenie odczyta³o dane z zewn¹trz
// ! jeœli zdefiniowano BOARD_LED_RX_PIN to ustawienie tego pinu w stan wysoki na okreœlony czas, ma to zadanie poinformowaæ u¿ytkownika migaj¹cym ledem ¿e nastêpuje transmisja
// -> AuserId - jest to numer zdefiniowany przez aplikacje , który to bêdzie przekazywany w messagu do ka¿dego zadania
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
// Wys³anie messaga informuj¹cego ¿e urz¹dzenie wys³a³o dane na zewn¹trz
// ! jeœli zdefiniowano BOARD_LED_TX_PIN to ustawienie tego pinu w stan wysoki na okreœlony czas, ma to zadanie poinformowaæ u¿ytkownika migaj¹cym ledem ¿e nastêpuje transmisja
// -> AuserId - jest to numer zdefiniowany przez aplikacje , który to bêdzie przekazywany w messagu do ka¿dego zadania
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
// Wys³anie messaga informuj¹cego ¿e urz¹dzenie ¿e urz¹dzenie ¿yje
// ! jeœli zdefiniowano BOARD_LED_LIFE_PIN to nastêpuje togglowanie stanu 
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
// Procedura wywo³ywana przez g³ówne zadanie frameworka
// ! W tej procedurze nastêpuje odliczanie sekundowe czasu, blinkowanie LIFE ledem (jeœli zdefiniowany),
// ! wygaszanie ledów od blink RX blink TX (jeœli zdefiniowane), 
void TXB_board::handle(void)
{
	// Sprawdzenie  przy pierwszym  wejsciu do pêtli loop iloœci wolnej pamiêci
	if(MaximumFreeHeapInLoop == 0)
	{
		FreePSRAMInLoop = getFreePSRAM();
		MinimumFreePSRAMInLoop = FreePSRAMInLoop;
		MaximumFreePSRAMInLoop = FreePSRAMInLoop;

		FreeHeapInLoop = getFreeHeap();
		MaximumFreeHeapInLoop = FreeHeapInLoop;
		MinimumFreeHeapInLoop = FreeHeapInLoop;
	}
	
	DEF_WAITMS_VAR(LOOPW);
	BEGIN_WAITMS(LOOPW, 1000)
	{
		Blink_Life();

#if defined(ARDUINO_ARCH_STM32) || defined(ESP32) || defined(__riscv64)
		DateTimeUnix++;
#endif
		
#ifdef XB_GUI
		if (xb_board_winHandle0 != NULL) xb_board_winHandle0->RepaintDataCounter++;
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
	DEF_WAITMS_VAR(LOOPW2);
	BEGIN_WAITMS(LOOPW2, 5)
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
							SendMessage_KeyPress((char)bufkey[i]);
						}
						break;
					}
				}
				if (res > 0) break;
			}
			t = t->Next;
		}
	}
	END_WAITMS(LOOPW2)
		
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
// -> Ataskdef - wskaŸnik definicji zadania
// -> DeviceID - 8 bajtowy numer ID urz¹dzenia na którym zadanie zosta³o uruchomione
// <- WskaŸnik utworzonej struktury opisuj¹cej zadanie
TTask *TXB_board::AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID)
{
	TTask *ta = TaskList;
	while (ta != NULL)
	{
		if (Ataskdef == ta->TaskDef)
		{
			String taskname = "";
			SendMessage_GetTaskNameString(Ataskdef, taskname);
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


		TaskCount++;

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
// Usuniêcie z frameworka wskazanego zadania
// -> Ataskdef - wskaŸnik definicji usuwanego zadania
// <- true OK
//    false B³¹d
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
			TaskCount--;
			return true;
		}
	}
	return false;
}
// ---------------------------------------------------------------------------------------------------------------
// G³ówna procedura uruchamiaj¹ca zadania z podzia³em na priorytety, zg³oszeñ z przerwañ i czekaj¹cych zadany czas
void TXB_board::IterateTask()
{
	TTask *t = TaskList;
	iteratetask_procedure = true;
	{
		// Sprawdzenie czy uruchomiæ przerwanie
		if(doAllInterruptRC > 0)
		{
			doAllInterruptRC--;
			bool isint = false;
			while (t != NULL)
			{
				if (t->TaskDef->dointerrupt != NULL)
				{
					if (t->dointerruptRC > 0)
					{
						iteratetask_procedure = false;
						CurrentTask = t;
						DoInterrupt(t->TaskDef);
						CurrentTask = XB_BOARD_DefTask.Task;
						isint = true;
						iteratetask_procedure = true;
					}
				}
				t = t->Next;
			}

			if (isint) 
			{
				iteratetask_procedure = false;
			}
		}
		//--------------------------------------
		t = TaskList;
		// Sprawdzenie czy w którymœ zadaniu min¹³ czas na uruchomienie Loop()
		{
			while (t != NULL)
			{
				if (t->TaskDef->doloop != NULL)
				{
					if (t->TickWaitLoop != 0)
					{
						if (SysTickCount - t->TickReturn >= t->TickWaitLoop)
						{
							CurrentTask = t;
							t->doloopRC++;
							t->TickWaitLoop = t->TaskDef->doloop();
							t->TickReturn = SysTickCount;
							CurrentTask  = XB_BOARD_DefTask.Task;
							t->doloopRC--;
						}
					}
				}
				t = t->Next;
			}

		}
		//--------------------------------------

		// zapamiêtanie iloœci wolnej pamiêci ram i minimalnego stanu
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
		// ----------------------------------------------------------

		// jeœli bufor nadawczy jest zape³niony to nie uruchamiaj zadañ
		//if (Serial_availableForWrite() < Serial_EmptyTXBufferSize) return;
		// ------------------------------------------------------------

		t = TaskList;
		// Uruchomienie zadañ tzw realtime
		{
			while (t != NULL)
			{
				if (t->TaskDef->doloop != NULL)
				{
					if (t->TaskDef->Priority == 0)
					{
						if (t->TickWaitLoop == 0)
						{
							CurrentTask = t;
							t->doloopRC++;
							t->TickWaitLoop = t->TaskDef->doloop();
							t->TickReturn = SysTickCount;
							CurrentTask = XB_BOARD_DefTask.Task;
							t->doloopRC--;
						}
					}
				}
				t = t->Next;
			}
		}
		//--------------------------------------

		t = CurrentIterateTask;
		if (t == NULL) t = TaskList;
		
		// Uruchomienie zadañ z podzia³em na priorytety
		{
			while (t != NULL)
			{
				if (t->TaskDef->doloop != NULL)
				{
					if (t->TaskDef->Priority > 0)
					{
						if (t->TickWaitLoop == 0)
						{
							t->CounterPriority++;
							if (t->CounterPriority >= t->TaskDef->Priority)
							{
								t->CounterPriority = 0;
								CurrentTask = t;
								t->doloopRC++;
								t->TickWaitLoop = t->TaskDef->doloop();
								t->TickReturn = SysTickCount;
								CurrentTask = XB_BOARD_DefTask.Task;
								t->doloopRC--;
								t = t->Next;
								CurrentIterateTask = t;
								break;
							}
						}
					}
				}
				t = t->Next;
				CurrentIterateTask = t;
			}
		}
		//--------------------------------------
	}
	iteratetask_procedure = false;
}

// -----------------------------------------------------------
// Pobranie adresu struktury opisuj¹cej zadanie wed³ug indeksu
// -> Aindex - indeks zadania weg³ug kolejnoœci dodania do frameworka
// <- wskaŸnik zadania
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
// Pobranie adresu struktury opisuj¹cej zadanie wed³ug nazwy
// -> ATaskName - Nazwa zadania 
// <- wskaŸnik definicji zadania
TTaskDef *TXB_board::GetTaskDefByName(String ATaskName)
{
	TTask *t = TaskList;
	String tmpstr;
	tmpstr.reserve(32);
	
	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (SendMessage_GetTaskNameString(t->TaskDef, tmpstr))
			{
				if (tmpstr == ATaskName) return t->TaskDef;
			}
		}
		t = t->Next;
	}
	return NULL;
}
// ------------------------------------------------------------------------------------
// Wyzwolenie przy nastêpnej iteracji procedury z definicji zadania do obs³ugi przerwañ
// -> WskaŸnik definicji zadania którego ma zostaæ uruchomiona procedura interrupt
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
// Wykonanie procedure obs³ugi przerwania wskazanego zadania
// -> WskaŸnik definicji zadania którego ma zostaæ uruchomiona procedura interrupt
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
#pragma endregion
#pragma region FUNKCJE_MESSAGOW
// -----------------------------------------------------
// Wykonanie procedury obs³ugi messaga wybranego zadania
// -> mb - WskaŸnik struktury opisuj¹cej messaga
// -> Arunagain  - Jeœli true to wywo³anie procedury messaga z mo¿liwoœci¹ wykonania ponownie
//               - Jeœli false nie wykona siê procedura messaga odwo³a siê poraz kolejny 
//                 Argument s³u¿y do nikniêcia ewentualnego zapêtlenia odwo³añ messagów.
// -> Afromtask  - Zadania od którego wysy³any jest messag
// -> Atotaskdef - Definicja zadania do którego messag jest wysy³any
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
// Wykonanie procedury obs³ugi messaga dla wszystkich zadañ
// -> mb - WskaŸnik struktury opisuj¹cej messaga
// -> Arunagain    - Jeœli true to wywo³anie procedury messaga z mo¿liwoœci¹ wykonania ponownie
//                 - Jeœli false nie wykona siê procedura messaga odwo³a siê poraz kolejny 
//                   Argument s³u¿y do nikniêcia ewentualnego zapêtlenia odwo³añ messagów.
// -> Afromtask    - Zadania od którego wysy³any jest messag
// -> Aexcludetask - WskaŸnik definicji zadania które ma zostaæ wykluczone z obœ³ugi messaga
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
// Wykonanie procedury obs³ugi messaga zadania o wskazanej nazwie
// -> Ataskname    - Nazwa zadania
// -> mb           - WskaŸnik struktury opisuj¹cej messaga
// -> Arunagain    - Jeœli true to wywo³anie procedury messaga z mo¿liwoœci¹ wykonania ponownie
//                 - Jeœli false nie wykona siê procedura messaga odwo³a siê poraz kolejny 
//                   Argument s³u¿y do nikniêcia ewentualnego zapêtlenia odwo³añ messagów.
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
				if (SendMessage_GetTaskNameString(t->TaskDef, tn))
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
// Wys³anie messaga do zadania w celu uzyskania stringu statusowego
// -> ATaskDef       - Definicja zadania od którego uzyskamy string status
// -> APointerString - Referencja klasy String w którym znajdzie siê status
// <- True           - wykonano poprawnie procedure pobrania status stringu
//    False          - Zadanie nie obs³uguje messaga 
bool TXB_board::SendMessage_GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKSTATUS_STRING;
	mb.Data.PointerString = &APointerString;
	bool res = DoMessage(&mb, true, CurrentTask, ATaskDef);
	*mb.Data.PointerString += "                   ";
	return res;
}
// --------------------------------------------------------------------
// Wys³anie messaga do zadania w celu uzyskania stringu z nazw¹ zadania
// -> ATaskDef       - Definicja zadania od którego uzyskamy string z nazw¹
// -> APointerString - Referencja klasy String w którym znajdzie siê nazwa
// <- True           - wykonano poprawnie procedure pobrania nazwy zadania
//    False          - Zadanie nie obs³uguje messaga 
bool TXB_board::SendMessage_GetTaskNameString(TTaskDef *ATaskDef,String &APointerString)
{
	if (ATaskDef == NULL) return false;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKNAME_STRING;
	mb.Data.PointerString = &APointerString;
	bool res = DoMessage(&mb, true, CurrentTask, ATaskDef);
	if (APointerString.length() > BOARD_TASKNAME_MAXLENGTH)
	{
		APointerString = APointerString.substring(0, BOARD_TASKNAME_MAXLENGTH);
	}
	return res;
}
// -----------------------------------------------------------------------------
// Wys³anie messaga informuj¹cego zadanie ¿e rozpocznie siê procedura OTA Update
void TXB_board::SendMessage_OTAUpdateStarted()
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_OTA_UPDATE_STARTED;
	mb.Data.uData64 = 0;
	DoMessageOnAllTask(&mb, true, doBACKWARD); 
}

void TXB_board::SendMessage_FunctionKeyPress(TKeyboardFunction Akeyfunction, char Akey, TTaskDef *Aexcludetask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	DoMessageOnAllTask(&mb, true, doFORWARD, NULL, Aexcludetask);  
} 

void TXB_board::SendMessage_KeyPress(char Akey,TTaskDef *Aexcludetask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	DoMessageOnAllTask(&mb, true, doFORWARD, NULL, Aexcludetask);  
}

void TXB_board::SendMessage_ConfigSave(void)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_CONFIG_SAVE;
	DoMessageOnAllTask(&mb, true, doFORWARD);  
}

void TXB_board::SendMessage_FreePTR(void *Aptr)
{
	if (Aptr != NULL)
	{
		TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
		mb.IDMessage = IM_FREEPTR;
		mb.Data.FreePTR = Aptr;
		DoMessageOnAllTask(&mb, true, doBACKWARD);
	}
}


#pragma endregion
#pragma region FUNKCJE_OBSLUGI_RAM
// -------------------------------------
// Zwrócenie iloœci wolnej pamiêci PSRAM
// <- Iloœæ bajtów
#ifdef ARDUINO_ARCH_STM32
extern "C" void * _sbrk(int);
extern uint32_t _estack;
#endif

uint32_t TXB_board::getFreePSRAM()
{
#if defined(BOARD_HAS_PSRAM) && defined(ESP32)
	
	uint32_t freepsram = ESP.getMaxAllocPsram();
#ifdef PSRAM_BUG
	uint32_t r = 0;
	if (lastfreepsram == 0)
	{
		lastfreepsram = freepsram;
	}
	else
	{
		r = (lastfreepsram > freepsram) ? (lastfreepsram - freepsram) : (freepsram - lastfreepsram);	
	}

	if (r > (100 * MaximumMallocPSRAM))
	{
		asm("memw");
		
		GETFREEPSRAM_ERROR_COUNTER++;
	}
	else
	{
		lastfreepsram = freepsram;
	}
	return lastfreepsram;
#else
	return freepsram;
#endif
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
// Zwrócenie iloœci wolnej pamiêci RAM
// <- Iloœæ bajtów
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
//#if !defined(_VMICRO_INTELLISENSE)
//------------------------------------------------------------------------------------------------------------------------------
// Rezerwacja pamiêci SPI RAM, jeœli p³ytka nie udostêpnia takiego rodzaju pamiêci to nast¹pi przydzielenie z podstawowej sterty
// -> Asize - Iloœæ rezerwowanej pamiêci PSRAM
// <- WskaŸnik zarezerwowanego bloku pamiêci PSRAM
void *TXB_board::_malloc_psram(size_t Asize)
{
#ifdef PSRAM_BUG
	size_t size = Asize;
	MaximumMallocPSRAM = (Asize > MaximumMallocPSRAM) ? Asize : MaximumMallocPSRAM;
#endif
	
	
#ifdef BOARD_HAS_PSRAM
#ifdef PSRAM_BUG
	size = size >> 4;
	size = size + 1;
	size = size << 4;
	size += 4;
	void *ptr = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
#ifdef LOG_MALLOC_FREE
	board.Log(String("inbug _malloc_psram, size: [" + String(size) + "] adress:" + String((uint32_t)ptr, HEX)).c_str(), true, true);
#endif
#else
	void *ptr = heap_caps_malloc(Asize, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
#ifdef LOG_MALLOC_FREE
	board.Log(String("_malloc_psram, size: [" + String(size) + "] adress:" + String((uint32_t)ptr, HEX)).c_str(), true, true);
#endif
#endif 
#else
	void *ptr = malloc(Asize);
#ifdef LOG_MALLOC_FREE
	board.Log(String("nopsram _malloc_psram, size: [" + String(size) + "] adress:" + String((uint32_t)ptr, HEX)).c_str(), true, true);
#endif
#endif

	if (ptr != NULL)
	{
#ifdef BOARD_HAS_PSRAM
#ifdef PSRAM_BUG
		if ((size - Asize) > 0)
		{
			xb_memoryfill(&((uint8_t *)ptr)[Asize], size - Asize, 0xff);
		}
#endif
#endif
		xb_memoryfill(ptr, Asize, 0);
		OurReservedBlock++;
	}
	else
	{
		board.Log("Out of memory.", true, true, tlError);
	}
	
	return ptr;
}
//#endif
//#if !defined(_VMICRO_INTELLISENSE)
//---------------------------------------
// Rezerwacja pamiêci RAM mikrokontrolera
// -> Asize - Iloœæ rezerwowanej pamiêci 
// <- WskaŸnik zarezerwowanego bloku pamiêci 
void *TXB_board::_malloc(size_t size)
{
	void *ptr = malloc(size);
#ifdef LOG_MALLOC_FREE
	board.Log(String("_malloc, size: [" + String(size) + "] adress:"+String((uint32_t)ptr, HEX)).c_str(), true, true);
#endif
	if (ptr != NULL)
	{
		xb_memoryfill(ptr, size, 0);
		OurReservedBlock++;
	}
	else
	{
		board.Log("Out of memory.", true, true, tlError);
	}
	return ptr;
}
//#endif
void ___free(void *Aptr)
{
	free(Aptr);
}
// ------------------
// Zwolnienie pamiêci
// -> Aptr wskaŸnik na blok pamiêci zwalnianej
// ! przed zwolnieniem pamiêci zostaje wys³any messag do wszystkich zadañ informuj¹cy ¿e wskazany blok pamiêci zostaje zwolniony
void TXB_board::free(void *Aptr)
{
	if (Aptr != NULL)
	{
		SendMessage_FreePTR(Aptr);
		___free(Aptr);
		OurReservedBlock--;
#ifdef LOG_MALLOC_FREE
		board.Log(String("free adress:" + String((uint32_t)Aptr,HEX)).c_str(), true, true);
#endif
	}
}
// --------------------------------------------------------------------------------------------------------------------
// Zwolnienie pamiêci z pod wskazanego (wskaŸniniem) wskaŸnika i nastêpnie nadanie wartoœæi NULL wskaŸnikowi wskazanemu
//  -> WskaŸnik wskaŸnika zwalnianego bloku pamiêci
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
// Odczyt ze wskazanego streamu okreœlon¹ iloœæ bajtów
// -> Adata          - wskaŸnik na blok pamiêci gdzie maj¹ zostaæ zapisane odczytane bajty
// -> Amaxlength     - wielkoœæ maksymalna bloku pamiêci i równoczeœnie maksymalna iloœæ bajtów do odczytu
// -> AStreamtaskdef - wskaŸnik zadania obs³uguj¹cego stream
// -> Afromaaddress	 - adres / kana³ z którego bêdziê czytany stream
//                   ! Jeœli zero to stream powinien udostêpniæ dane pierwsze w kolejce do odczytu z dowolnego adresu
// <- iloœæ bajtów odczytanych
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
// Wys³anie bloku pamiêci wskazanym streamem pod wskazany adres
// -> Adata          - wskaŸnik na blok pamiêci do wys³ania
// -> Alength        - wielkoœæ w bajtach bloku pamiêci do wys³ania
// -> AStreamtaskdef - wskaŸnik zadania obs³uguj¹cego stream
// -> AToAddress     - Adres pod który zostanie wys³any wskazany blok pamiêci
// <- Iloœæ bajtów ile zdo³ano wys³aæ wskazanym streamem
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
// Poinformowanie zadania obs³uguj¹cego stream, ¿e cykliczny odczyt streamu zostanie przejêty przez inne zadanie
// -> AStreamtaskdef - wskaŸnik zadania obs³uguj¹cego stream
// -> AToAddress     - Adres/Kana³/Numer urz¹dzenia którego bêdzie obs³ugiwany odczyt streamu
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
// Poinformowanie zadania obs³uguj¹cego stream, ¿e cykliczny odczyt streamu ma zostaæ 
// przekazany z powrotem do zadania obœ³ugij¹cego stream
// -> AStreamtaskdef - wskaŸnik zadania obs³uguj¹cego stream
// -> AToAddress     - Adres/Kana³/Numer urz¹dzenia którego oddana ma zostaæ obs³uga zadania od straemu
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
// Zapytanie zadania streamu o udostêpnienie lokalnego adresu
// -> *AStreamtaskdef - wskaŸnik zadania obs³uguj¹cego stream
// -> *Alocaladdress -  wskaŸnik na wartoœæ zwrócon¹ czyli lokalny adres
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
// Funkcja wywo³ywana w GetStream, s³u¿y do wyci¹gania ze streamu ramki która zaczyna siê od 4 bajtowego ACK
// -> mb			            - WskaŸnik struktury messaga GetStream 
// -> AHandleDataFrameTransport - WskaŸnik na strukture opisuj¹c¹ bufor odczytanej ramki, s³u¿y do skompletowania fragmentów streamu z jednego Ÿród³a
// -> ATaskDefStream			- WskaŸnik definicji zadania streamu z którego nadchodz¹ dane
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

		// Sprawdzenie czy nadchodz¹cy bajt jest pierwszym od ACK
		if((AHandleDataFrameTransport->indx_interpret == 0) && (v == FRAME_ACK_A))
		{
			// Uruchominie interpretowania 
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			AHandleDataFrameTransport->isdataframe_interpret = true;

			isininterpret = true;
			indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz¹cy bajt jest drugim od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 1) && (v == FRAME_ACK_B))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)
				if (indxstartinterpret == 0xffff) 
					indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz¹cy bajt jest trzecim od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 2) && (v == FRAME_ACK_C))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)					
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz¹cy bajt jest czwartym od ACK
		else if((AHandleDataFrameTransport->indx_interpret == 3) && (v == FRAME_ACK_D))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)	
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz¹cy bajt to 4 pozycja i czy ma wartoœæ oczekiwan¹ SIZE
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
		// Bajty siê nie zgadzaj¹ do wzoru nadchodz¹cej ramki
		else
		{
			// Sprawdzenie czy by³o teraz rozpoczête interpretowanie
			if(isininterpret)
			{
				// jeœli by³o to leæ dalej z interpretacj¹
				AHandleDataFrameTransport->indx_interpret = 0;
				AHandleDataFrameTransport->isdataframe_interpret = false;
				isininterpret = false;
				indxstartinterpret = 0xffff;
			}
			else
			{
				// jeœli teraz nie by³o rozpoczêcia to wstawiæ co zbuforowane do streamu
				if(AHandleDataFrameTransport->isdataframe_interpret)
				{
					//AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			
					uint32_t c = AHandleDataFrameTransport->indx_interpret - indx;

					// sprawdzenie czy zmieœci siê do tego bufora streamu
					if((mb->Data.StreamData.Length - mb->Data.StreamData.LengthResult) >= c)
					{
						int32_t indx_s = mb->Data.StreamData.LengthResult - 1;
						int32_t indx_d = mb->Data.StreamData.LengthResult - 1 + c;

						// Mo¿na przesun¹æ to co zosta³o zapamiêtane
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
							SendMessage_GetTaskNameString(ATaskDefStream, tmps);
							Log(tmps.c_str(), false, false, tlError);
						}
						Log("]. The read buffer is too little.", false, false, tlError);
					}

					// leæ dalej z interpretacj¹
					AHandleDataFrameTransport->indx_interpret = 0;
					AHandleDataFrameTransport->isdataframe_interpret = false;
					isininterpret = false;
					indxstartinterpret = 0xffff;
				}
			}
		}
		indx++;
	}
	// Sprawdzenie czy odnotowano jakieœ interpretowanie
	if(indxstartinterpret != 0xffff)
	{
		mb->Data.StreamData.LengthResult = indxstartinterpret;
	}
	return true;
}
// --------------------------------------------------------------------------------------
// Jeœli ramka zosta³a b³êdnie zinterpretowana to nastêpuje oddanie z powrotem do streamu 
// -> mb			            - WskaŸnik struktury messaga GetStream 
// -> AHandleDataFrameTransport - WskaŸnik na strukture opisuj¹c¹ bufor odczytanej ramki, s³u¿y do skompletowania fragmentów streamu z jednego Ÿród³a
// <- true - Nast¹pi³o oddanie danych do bufora wskazanego w streamie
//    false - Musi nast¹piæ kolejne wywo³anie messaga streamu w celu odczytu danych
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
// Dodanie struktury opisuj¹cej bufor odczytywanej ramki, struktura jest przypisana do zadania osb³usguj¹cego stream z którego nadchodz¹ dane
// -> AStreamtaskdef - WskaŸnik na definicje zadania streamu z którego bêd¹ interpretowane ramki
// -> Afromaddress   - Adres z którego nadchodz¹ dane do intepretowania ramki
// <- WskaŸnik na strukture opisuj¹c¹ bufor odczytanej ramki
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
// Sprawdzenie oraz usuniêcie bufora odczytywanej ramki jeœli na 10 sekund zatrzyma³a siê transmisja lub jeœli nie by³ potrzebny
// Atask -> WskaŸnik zadania które ma zostaæ sprawdzone
//          ! Jeœli podane zostanie NULL to sprawdzenie bêdzie siê odnoœciæ do wszystkich zadañ
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
// Wys³anie ramki danych okreœlonym streamem do wskazanego zadania
// -> ASourceTaskName   - Nazwa zadania z którego ramka jest wysy³ana 
// -> ASourceAddress    - Adres z którego nastêpuje wys³anie ramki potrzebny do odpowiedzi
// -> AONStreamTaskName - Nazwa zadania streamu którym bêdzie wysy³ana ramka
//                        ! Jeœli podamy nazwê streamu "local", to ramka trafi lokalnie do zadania wskazanego
// -> ADestAddress		- Adres pod który zostanie wys³ana ramka
// -> ADestTaskName     - Nazwa zadania do którego ramka danych ma trafiæ
//                        ! Jeœli podamy NULL, to ramka trafi lokalnie do zadania wskazanego
// -> ADataFrame        - WskaŸnik na zawartoœæ ramki do wys³ania
// -> Alength			- Wielkoœæ w bajtach ramki do wys³ania
// -> AframeID          - WskaŸnik pod którym zostanie zapisany unikalny numer nadawczy ramki
//                        ! Wartoœæ jest potrzebna do idendyfikacji potwiedzenia odebrania ramki
// <- true - Pomyœlnie wys³ano ramkê
//    false - Wyst¹pi³ problem z wys³aniem ramki
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
// Wys³anie ramki danych okreœlonym streamem do wskazanego zadania
// -> ASourceTaskName   - Nazwa zadania z którego ramka jest wysy³ana 
// -> ASourceAddress    - Adres z którego nastêpuje wys³anie ramki potrzebny do odpowiedzi
// -> ATaskDefStream    - WskaŸnik definicji zadania streamu którym bêdzie wysy³ana ramka
// -> ADestAddress		- Adres pod który zostanie wys³ana ramka
// -> ADestTaskName     - Nazwa zadania do którego ramka danych ma trafiæ
//                        ! Jeœli podamy NULL, to ramka trafi lokalnie do zadania wskazanego
// -> ADataFrame        - WskaŸnik na zawartoœæ ramki do wys³ania
// -> Alength			- Wielkoœæ w bajtach ramki do wys³ania
// -> AframeID          - WskaŸnik pod którym zostanie zapisany unikalny numer nadawczy ramki
//                        ! Wartoœæ jest potrzebna do idendyfikacji potwiedzenia odebrania ramki
// <- true - Pomyœlnie wys³ano ramkê
//    false - Wyst¹pi³ problem z wys³aniem ramki
bool TXB_board::SendFrameToDeviceTask(String ASourceTaskName, uint32_t ASourceAddress, TTaskDef *ATaskDefStream, uint32_t ADestAddress, String ADestTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID)
{	
	uint32_t reslen = 0;
	static THDFT *hdft = NULL;
	if (hdft != NULL)
	{
		board.free(hdft);
		hdft = NULL;
	}
	hdft = (THDFT *)board._malloc(sizeof(THDFT));
	
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
#ifdef BOARD_BETA_DEBUG
			board.Log(String("Stream error - Send: " + String(ltsize) + " Sended: " + String(reslen) + " ...").c_str(), true, true, tlWarn);
#endif
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
// Wys³anie ramki potwierdzaj¹cej
// -> AFrameID - Numer ID ramki potwierdzanej
// -> ATaskDefStream - WskaŸnik zadania streamu którym zostanie wys³ana ramka odpowiadaj¹ca
//                     ! Jeœli NULL to ramka potwierdzaj¹ca zostanie rozg³oszona lokalnie
// -> ASourceAddress - Adres z podktórego wysy³ane jest potwierdzaj¹ca ramka
// -> ADestAddress   - Adres pod który potwierdzaj¹ca ramka ma trafiæ
// -> AframeType	 - Okreœlenie typu ramki, w przypadku wysy³ania potwierdzenie jest to rezultat
// -> ADestDeviceID  - ID urz¹dzenia do którego ramka potwierdzaj¹ca ma trafiæ. Potrzebny w celu wstêpnej segragacji przez framework
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

#ifdef BOARD_BETA_DEBUG
void PrintFrameTransport(TFrameTransport *Aft)
{
	String s;
	s.reserve(512);
	s += "\nAft->DestAddress   = " + IPAddress(Aft->DestAddress).toString();
	s += "\nAft->DestDeviceID = " + board.DeviceIDtoString(Aft->DestDeviceID);
	s += "\nAft->SourceAddress = " + IPAddress(Aft->SourceAddress).toString();
	s += "\nAft->SourceDeviceID = " + board.DeviceIDtoString(Aft->SourceDeviceID);
	s += "\n";
	board.Log(s.c_str());
	
}
#endif
// -----------------------------------------------------------------------------------------------------------------------
// Sprawdzenie sumy kontrolnej wskazanej ramki, wys³anie messaga do wskazanego zadania w ramce z wskaŸnikiem na dane ramki
// -> Aft - WskaŸnik ramki transportowej
// -> wskaŸnik definicji zadania streamu którym ramka dotar³a do urz¹dzenia
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

#ifdef BOARD_BETA_DEBUG
				board.Log(String("Receive frame, HANDLESEND: " + String(Aft->FrameID,HEX)+" res:"+String(res)).c_str(), true, true);
#endif

				if (res)
				{
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
// Wys³anie messaga do wskazanego zadania w ramce z wskaŸnikiem na dane ramki lokalnie
// -> Aft - WskaŸnik ramki transportowej
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
// Ustalenie wskazanego task streamu jako klawiatury
// -> AStreamDefTask - Task stream który ma byæ klawiatur¹
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

			t->GetStreamAddressAsKeyboard = (uint32_t *)realloc((void *)t->GetStreamAddressAsKeyboard, sizeof(uint32_t)*(t->CountGetStreamAddressAsKeyboard + 1));
			t->CountGetStreamAddressAsKeyboard++;

			t->GetStreamAddressAsKeyboard[t->CountGetStreamAddressAsKeyboard-1] = Aaddress;
			return;
		}
	}
	return;
}
// -----------------------------------------------------------------------------------
// Odjêcie adresu streamu wskazanego task streamu jako klawiatury
// -> AStreamDefTask - Task stream z którego usuwany jest adres
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

			t->PutStreamAddressAsLog = (uint32_t *)realloc((void *)t->PutStreamAddressAsLog, sizeof(uint32_t)*(t->CountPutStreamAddressAsLog + 1));
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

			t->PutStreamAddressAsGui = (uint32_t *)realloc((void *)t->PutStreamAddressAsGui, sizeof(uint32_t)*(t->CountPutStreamAddressAsGui + 1));
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
	TTask *t = TaskList;
	while (t != NULL)
	{
		for (uint32_t i = 0; i < t->CountPutStreamAddressAsLog; i++)		
		{
			if (t->PutStreamAddressAsLog[i] != 0xffffffff)
			{
				PutStream(Adata, Alength, t->TaskDef, t->PutStreamAddressAsLog[i]);		
			}
		}
		t = t->Next;
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
	
	AllPutStreamLog(&Achr, 1);

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
	
	int len = StringLength((char *)Atxt, 0);
	if (len == 0) return;

	if (puttime)
	{
		String txttime = "";
		GetTimeIndx(txttime, DateTimeUnix - DateTimeStart);
		txttime = "\n[" + txttime + "] ";
		AllPutStreamLog((void *)txttime.c_str(), txttime.length());
	}

	if (showtaskname)
	{
		if (NameTaskDef != NULL)
		{
			String taskname = "---";
			SendMessage_GetTaskNameString(NameTaskDef, taskname);
		
			if (taskname.length() > 0)
			{
				taskname.trim();
				taskname = '[' + taskname + "] ";
				AllPutStreamLog((void *)taskname.c_str(), taskname.length());
			}
		}
	}

	AllPutStreamLog((void *)Atxt, len);

	if (NoTxCounter == 0) TXCounter++;
}

void TXB_board::Log(cbufSerial *Acbufserial, TTypeLog Atl)
{
	if (CurrentTask != NULL)
	{
		if (Atl == tlInfo)
		{
			if (CurrentTask->ShowLogInfo == false)
			{
				clearbuf(Acbufserial);
				return;
			}
		}
		else if (Atl == tlWarn)
		{
			if (CurrentTask->ShowLogWarn == false)
			{
				clearbuf(Acbufserial);
				return;
			}
		}
		else if (Atl == tlError)
		{
			if (CurrentTask->ShowLogError == false)
			{
				clearbuf(Acbufserial);
				return;
			}
		}
	}
	else 
	{
		clearbuf(Acbufserial);
		return;
	}
		

	while (Acbufserial->available()>0)
	{
		
		Log((char)(Acbufserial->read()), Atl); 
		if (NoTxCounter==0) TXCounter++;
	}
}

void TXB_board::Log_TimeStamp()
{
	PrintTimeFromRun();
}

void TXB_board::PrintTimeFromRun(cbufSerial *Astream)
{
	Astream->print('[');
	GetTimeIndx(Astream, DateTimeUnix - DateTimeStart);
	Astream->print(FSS("] "));
}

void TXB_board::PrintTimeFromRun(void)
{
	cbufSerial Astream(32);
	Astream.print(FSS("["));
	GetTimeIndx(&Astream, DateTimeUnix - DateTimeStart);
	Astream.print(FSS("] "));
	Log(&Astream);
}
#pragma endregion
#pragma region PREFERENCES
#ifdef XB_PREFERENCES
//-----------------------------------------------------------------------------------------------------------------------
bool TXB_board::PREFERENCES_BeginSection(String ASectionname)
{
#ifdef ESP32
	if (ASectionname.length() >= 16)
	{
		String ts = ASectionname;
		ASectionname = ASectionname.substring(0, 15);
	}


	return xbpreferences.begin(ASectionname.c_str());
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
void TXB_board::PREFERENCES_EndSection()
{
#ifdef ESP32
	xbpreferences.end();
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutArrayBytes(const char* key, const void *array,size_t sizearray)
{
#ifdef ESP32
	return xbpreferences.putBytes(key, array,sizearray);
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_GetArrayBytes(const char* key, void* array, size_t maxsizearray)
{
#ifdef ESP32
	return xbpreferences.getBytes(key, array, maxsizearray);
#else
	return 0;
#endif
}


//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutBool(const char* key, const bool value)
{
#ifdef ESP32
	return xbpreferences.putBool(key, value);
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
bool TXB_board::PREFERENCES_GetBool(const char* key, const bool defaultvalue)
{
#ifdef ESP32
	return xbpreferences.getBool(key, defaultvalue);
#else
	return defaultvalue;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_GetString(const char* key, char* value, const size_t maxlen)
{
#ifdef ESP32
	return xbpreferences.getString(key, value, maxlen);
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
String TXB_board::PREFERENCES_GetString(const char* key, String defaultvalue)
{
#ifdef ESP32
	return xbpreferences.getString(key, defaultvalue);
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------------------------------------------------
uint32_t TXB_board::PREFERENCES_GetUINT32(const char* key, uint32_t defaultvalue)
{
#ifdef ESP32
	return xbpreferences.getULong(key, defaultvalue);
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
uint8_t TXB_board::PREFERENCES_GetUINT8(const char* key, uint8_t defaultvalue)
{
#ifdef ESP32
	return xbpreferences.getUChar(key, defaultvalue);
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutString(const char* key, const char* value)
{
#ifdef ESP32
	return xbpreferences.putString(key, value);
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutString(const char* key, String value)
{
#ifdef ESP32
	return xbpreferences.putString(key, value);
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutUINT32(const char* key, uint32_t value)
{
#ifdef ESP32
	return xbpreferences.putULong(key, value);
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------------------------------------------------------
size_t TXB_board::PREFERENCES_PutUINT8(const char* key, uint8_t value)
{
#ifdef ESP32
	return xbpreferences.putUChar(key, value);
#else
	return 0;
#endif
}
#endif

void TXB_board::LoadConfiguration(TTaskDef* ATaskDef)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_LOAD_CONFIGURATION;
	DoMessage(&mb, true, CurrentTask, ATaskDef);
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
	DoMessage(&mb, true, CurrentTask, ATaskDef);
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



#pragma endregion 
