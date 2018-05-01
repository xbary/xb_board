#include "XB_board.h"
/*
Insert Into WiFiClient Class

void abort();

void WiFiClient::abort()
{
if (_client)
_client->abort();
}
*/

TXB_board board(TASK_COUNT);

TTaskDef XB_BOARD_DefTask = { &XB_BOARD_Setup,&XB_BOARD_DoLoop,&XB_BOARD_DoMessage,0 };

volatile uint32_t DateTime;
volatile uint32_t DateTimeStart;

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#ifdef ARDUINO_ARCH_STM32F1
extern "C" {
#include <string.h>
#include <stdlib.h>

extern void *malloc(size_t size);
extern void free(void *memblock);
extern size_t strlen(const char *str);
extern void *memcpy(void *dest,const void *src,size_t count);
extern int sprintf(char *buffer,const char *format, ...);
}
#endif

#ifdef XB_GUI
#include <xb_GUI.h>
TWindowClass *winHandle0;
#endif
#ifdef XB_GUIGADGET
#include <xb_GUI_Gadget.h>
TGADGETMenu *menuHandle0;
#endif

//------------------------------------------------------------------------------------------------------------

#ifdef ESP8266
Ticker SysTickCount_ticker;
Ticker DateTimeSecond_ticker;
volatile uint32_t SysTickCount;
#endif

uint32_t Tick_ESCKey = 0;
uint8_t TerminalFunction = 0;
String rs;

#ifdef ESP8266
#ifdef wificlient_h
void TCPClientDestroy(WiFiClient **Awificlient)
{
	if (Awificlient != NULL)
	{
		if (*Awificlient != NULL)
		{

			delay(50);
			(*Awificlient)->flush();
			delay(50);
			(*Awificlient)->abort();
			delay(50);
			delete((*Awificlient));
			(*Awificlient) = NULL;
			delay(50);
		}
	}
}
#endif
#endif

#ifdef ESP8266
void SysTickCount_proc(void)
{
	SysTickCount++;
}

void DateTimeSecond_proc(void)
{
	DateTime++;
}
#endif

bool showasc = false;

bool XB_BOARD_DoMessage(TMessageBoard *Am)
{
	
	bool res = false;
	static uint8_t LastKeyCode = 0;

	switch (Am->IDMessage)
	{
	case IM_GPIO:
	{
		switch (Am->Data.GpioData.GpioAction)
		{
		case gaPinMode:
		{
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
			{
				pinMode(Am->Data.GpioData.NumPin,(WiringPinMode)Am->Data.GpioData.ActionData.Mode);
				res = true;
			}
			break;
		}
		case gaPinRead:
		{
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
			{
				Am->Data.GpioData.ActionData.Value = digitalRead(Am->Data.GpioData.NumPin);
				res = true;
			}
			break;
		}
		case gaPinWrite:
		{
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
			{
				digitalWrite(Am->Data.GpioData.NumPin, Am->Data.GpioData.ActionData.Value);
				res = true;
			}
			break;
		}
		case gaPinToggle:
		{
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
			{
				Am->Data.GpioData.ActionData.Value = !digitalRead(Am->Data.GpioData.NumPin);
				digitalWrite(Am->Data.GpioData.NumPin, Am->Data.GpioData.ActionData.Value);
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

#ifdef XB_GUIGADGET
	case IM_MENU:
	{
		switch (Am->Data.MenuData.TypeMenuAction)
		{
		case tmaGET_INIT_MENU:
		{
			switch (Am->Data.MenuData.IDMenu)
			{
			case 0:
			{
				Am->Data.MenuData.ActionData.MenuInitData.ItemCount = 1 + TASK_COUNT;
				Am->Data.MenuData.ActionData.MenuInitData.Width = 64;
				Am->Data.MenuData.ActionData.MenuInitData.CurrentSelect = 0;
				res = true;
			}
			default: break;
			}
			break;
		}
		case tmaGET_ITEM_MENU_STRING:
		{
			switch (Am->Data.MenuData.IDMenu)
			{
			case 0:
			{

				switch (Am->Data.MenuData.ActionData.MenuItemData.ItemIndex)
				{
					DEF_MENUITEMNAME(0, FSS("Restart MCU"));
				default:
				{

					uint8_t i = Am->Data.MenuData.ActionData.MenuItemData.ItemIndex - 1;
					String n = "";
					if (board.TaskDef[i] != NULL)
					{

						if (board.GetTaskName(board.TaskDef[i], n))
						{
							n += FSS(" >>>");
						}
						else
						{
							n = FSS("task no menu!");
						}
					}
					else
					{
						n = FSS("task null!");
					}
					*(Am->Data.MenuData.ActionData.MenuItemData.PointerString) = n.c_str();
					break;
				}
				}
			}
			}
			res = true;
			break;
		}
		case tmaCLICK_ITEM_MENU:
		{
			switch (Am->Data.MenuData.IDMenu)
			{
			case 0:
			{
				switch (Am->Data.MenuData.ActionData.MenuClickData.ItemIndex)
				{
				case 0:
				{
					nvic_sys_reset();
					break;
				}
				default:
				{
					break;
				}
				}
				res = true;
			}
			default: 
			{
				String n = "";
				uint8_t itask = 0;
				for (uint8_t i = 0; i < TASK_COUNT; i++)
				{
					if (board.TaskDef[i] != NULL)
					{
						if (board.GetTaskName(board.TaskDef[i], n))
						{
							if (itask == (Am->Data.MenuData.ActionData.MenuClickData.ItemIndex - 1))
							{
								GUIGADGET_OpenMainMenu(board.TaskDef[i]);
								break;
							}
							else
							{
								itask++;
							}
						}
					}
				}
				res = true;
				break;
			}
			}
			break;
		}
		case tmaGET_CAPTION_MENU_STRING:
		{
			switch (Am->Data.MenuData.IDMenu)
			{
			case 0:
			{

				*(Am->Data.MenuData.ActionData.MenuCaptionData.PointerString) = FSS("BOARD MAIN MENU");
				res = true;
				break;
			}
			}
			break;
		}
		default: break;
		}
		break;
	}
#endif
	case IM_KEYBOARD:
	{
		if (Am->Data.KeyboardData.TypeKeyboardAction == tkaKEYPRESS)
		{
			if (Am->Data.KeyboardData.KeyFunction == KF_CODE)
			{
				static TKeyboardFunction KeyboardFunctionDetect = KF_CODE;

				if (showasc)
				{
					Serial_print((int)Am->Data.KeyboardData.KeyCode);
					Serial_print(' ');
					Serial_println(TerminalFunction);
				}

				if (TerminalFunction == 0)
				{
					switch (Am->Data.KeyboardData.KeyCode)
					{
					case 127:
					{
						Tick_ESCKey = 0;
						rs = "";
						board.SendKeyFunctionPress(KF_BACKSPACE, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
						break;
					}
					case 10:
					{
						if (LastKeyCode != 13)
						{
							rs.toLowerCase();
							rs.trim();
							board.cmdparse(rs);
							rs = "";
							board.SendKeyFunctionPress(KF_ENTER, 0, &XB_BOARD_DefTask);
							TerminalFunction = 0;
							
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
							rs.toLowerCase();
							rs.trim();
							board.cmdparse(rs);
							rs = "";
							board.SendKeyFunctionPress(KF_ENTER, 0, &XB_BOARD_DefTask);
							TerminalFunction = 0;
							
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
						rs = "";
						TerminalFunction = 1;
						Tick_ESCKey = SysTickCount;
						break;
					}
					case 7:
					{
						Tick_ESCKey = 0;
						rs = "";
						board.SendKeyFunctionPress(KF_ESC, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
						break;
					}
					case 9:
					{
						Tick_ESCKey = 0;
						rs = "";
						board.SendKeyFunctionPress(KF_TABNEXT, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
						break;
					}
					case 255:
					case 0:
					{
						Tick_ESCKey = 0;
						rs = "";
						TerminalFunction = 0;
						break;
					}


					default:
					{
						Tick_ESCKey = 0;
						rs += (char)Am->Data.KeyboardData.KeyCode;
						if (rs.length() > 16)
						{
							rs = "";
							board.Log(FSS("KEYBORARD: Cmdparse buffer is full.\n"));
						}

						break;
					}
					}
				}
				else if (TerminalFunction == 1)
				{
					if (Am->Data.KeyboardData.KeyCode == 91) // Nadchodzi funkcyjny klawisz
					{
						Tick_ESCKey = 0;
						TerminalFunction = 2;
						Am->Data.KeyboardData.KeyCode = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 10)
					{
						TerminalFunction = 0;
					}
					else
					{
						rs = "";
						Tick_ESCKey = 0;
						board.SendKeyFunctionPress(KF_ESC, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
					}
				}
				else if (TerminalFunction == 2)
				{
					if (Am->Data.KeyboardData.KeyCode == 65) // cursor UP
					{
						board.SendKeyFunctionPress(KF_CURSORUP, 0, &XB_BOARD_DefTask);
						Am->Data.KeyboardData.KeyCode = 0;
						TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 66) // cursor DOWN
					{
						board.SendKeyFunctionPress(KF_CURSORDOWN, 0, &XB_BOARD_DefTask);
						Am->Data.KeyboardData.KeyCode = 0;
						TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 68) // cursor LEFT
					{
						board.SendKeyFunctionPress(KF_CURSORLEFT, 0, &XB_BOARD_DefTask);
						Am->Data.KeyboardData.KeyCode = 0;
						TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 67) // cursor RIGHT
					{
						board.SendKeyFunctionPress(KF_CURSORRIGHT, 0, &XB_BOARD_DefTask);
						Am->Data.KeyboardData.KeyCode = 0;
						TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 90) // shift+tab
					{
						board.SendKeyFunctionPress(KF_TABPREV, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
					}
					else if (Am->Data.KeyboardData.KeyCode == 49) // F1
					{
						KeyboardFunctionDetect = KF_F1;
						TerminalFunction = 3;
					}
					else if (Am->Data.KeyboardData.KeyCode == 50) // F9
					{
						KeyboardFunctionDetect = KF_F9;
						TerminalFunction = 5;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) 
					{
						KeyboardFunctionDetect = KF_DELETE;
						TerminalFunction = 4;
					}
					else
					{
						TerminalFunction = 0;
					}

				}
				else if (TerminalFunction == 3)
				{
					if (Am->Data.KeyboardData.KeyCode == 49) // F1
					{
						KeyboardFunctionDetect = KF_F1;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 50) // F2
					{
						KeyboardFunctionDetect = KF_F2;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) // F3
					{
						KeyboardFunctionDetect = KF_F3;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 52) // F4
					{
						KeyboardFunctionDetect = KF_F4;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 53) // F5
					{
						KeyboardFunctionDetect = KF_F5;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 55) // F6
					{
						KeyboardFunctionDetect = KF_F6;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 56) // F7
					{
						KeyboardFunctionDetect = KF_F7;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 57) // F8
					{
						KeyboardFunctionDetect = KF_F8;
						TerminalFunction = 4;
					}
					else
					{
						TerminalFunction = 0;
					}
				}
				else if (TerminalFunction == 5)
				{

					if (Am->Data.KeyboardData.KeyCode == 48) // F9
					{
						KeyboardFunctionDetect = KF_F9;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 49) // F10
					{
						KeyboardFunctionDetect = KF_F10;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 51) // F11
					{
						KeyboardFunctionDetect = KF_F11;
						TerminalFunction = 4;
					}
					else if (Am->Data.KeyboardData.KeyCode == 52) // F12
					{
						KeyboardFunctionDetect = KF_F12;
						TerminalFunction = 4;
					}
					else
					{
						TerminalFunction = 0;
					}

				}
				else if (TerminalFunction == 4)
				{
					if (Am->Data.KeyboardData.KeyCode == 126)
					{
						board.SendKeyFunctionPress(KeyboardFunctionDetect, 0, &XB_BOARD_DefTask);
						TerminalFunction = 0;
					}
					else
					{
						TerminalFunction = 0;
					}
				}
				else
				{
					TerminalFunction = 0;
				}

				res = true;

			}
			else if (Am->Data.KeyboardData.KeyFunction == KF_F1)
			{
#ifdef XB_GUI
				GUI_Show();
				winHandle0 = GUI_WindowCreate(&XB_BOARD_DefTask, 0);
#endif
#ifdef XB_GUIGADGET
				menuHandle0 = GUIGADGET_CreateMenu(&XB_BOARD_DefTask, 0);
#endif
				res = true;
			}

			
			LastKeyCode = Am->Data.KeyboardData.KeyCode;

		}
		break;
	}
#ifdef XB_GUI
	case IM_WINDOW:
	{
		switch (Am->Data.WindowData.WindowAction)
		{
		case waCreate:
		{
			if (Am->Data.WindowData.ID == 0)
			{
				Am->Data.WindowData.ActionData.Create.X = 26;
				Am->Data.WindowData.ActionData.Create.Y = 0;
				Am->Data.WindowData.ActionData.Create.Width = 36;
				Am->Data.WindowData.ActionData.Create.Height = board.TaskDefCount + 9;

				res = true;
			} 

			break;
		}
		case waDestroy:
		{
			if (Am->Data.WindowData.ID == 0)
			{
				winHandle0 = NULL;
			}
			res = true;
			break;
		}
		case waGetCaptionWindow:
		{
			if (Am->Data.WindowData.ID == 0)
			{
#ifdef ESP8266
				*(Am->Data.WindowData.ActionData.GetCaption.PointerString) = FSS("BOARD (ESP8266, 160Mhz)");
#endif
#ifdef ARDUINO_ARCH_STM32F1
				*(Am->Data.WindowData.ActionData.GetCaption.PointerString) = FSS("BOARD (STM32F1, 72Mhz)");
#endif
			}
			res = true;
			break;
		}
		case waRepaint:
		{
			if (Am->Data.WindowData.ID == 0)
			{
				if (winHandle0 != NULL)
				{
					winHandle0->BeginDraw();

					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 0, FSS("DEVICE NAME: "));
					winHandle0->SetBoldChar();
					winHandle0->SetTextColor(tfcYellow);
					winHandle0->PutStr(FSS(DEVICE_NAME));

					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 1, FSS("DEVICE VERSION: "));
					winHandle0->SetBoldChar();
					winHandle0->SetTextColor(tfcYellow);
					winHandle0->PutStr(FSS(DEVICE_VERSION));

					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 2, FSS("TIME FROM RUN:"));
					winHandle0->PutStr(0, 3, FSS("FREE HEAP:"));
					winHandle0->PutStr(18, 3, FSS("MIN HEAP:"));
					winHandle0->PutStr(18, 4, FSS("MAX HEAP:"));
					winHandle0->SetBoldChar();
					winHandle0->SetTextColor(tfcYellow);
					winHandle0->PutStr(String(board.MaximumFreeHeapInLoop).c_str());

					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 4, FSS("MEM USE:"));

					//--------------
					{
						int y;
						String name;
						y = 5;
						winHandle0->PutStr(0, y, FSS("__________________________________"));
						y++;
						winHandle0->PutStr(0, y, FSS("TASK NAME"));
						winHandle0->PutStr(15, y, FSS("STATUS"));
						winHandle0->SetTextColor(tfcGreen);
						y++;
						for (int i = 0; i < board.TaskDefCount; i++)
						{
							if (board.TaskDef[i] != NULL)
							{

								if (board.GetTaskName(board.TaskDef[i], name))
								{
									winHandle0->PutStr(0, y + i, name.c_str());
									name = "";
								}
							}
						}
					}
					winHandle0->EndDraw();
				}
			}
			res = true;
			break;
		}
		case waRepaintData:
		{
			if (Am->Data.WindowData.ID == 0)
			{
				if (winHandle0 != NULL)
				{
					winHandle0->BeginDraw();

					winHandle0->SetBoldChar();
					winHandle0->SetTextColor(tfcYellow);
					
					{
						cbufSerial cbuf(32);
						board.PrintTimeFromRun(&cbuf);
						winHandle0->PutStr(14, 2, cbuf.readString().c_str());
					}

					winHandle0->PutStr(10, 3, String(board.FreeHeapInLoop).c_str());
					winHandle0->PutChar(' ');
					winHandle0->PutStr(27, 3, String(board.MinimumFreeHeapInLoop).c_str());
					winHandle0->PutChar(' ');


					winHandle0->PutStr(9, 4, String((uint32_t)(100 - (board.FreeHeapInLoop / (board.MaximumFreeHeapInLoop / 100L)))).c_str());
					winHandle0->PutStr(FSS("% "));

					//---------------
					{
						String name;
						int y;

						y = 5;

						y += 2;
						for (int i = 0; i < board.TaskDefCount; i++)
						{
							if (board.TaskDef[i] != NULL)
							{

								if (board.GetTaskStatusString(board.TaskDef[i], name))
								{
									winHandle0->PutStr(15, y + i, name.c_str());
									name = "";
								}
							}
						}
					}
					winHandle0->EndDraw();
				}
			}
			res = true;
			break;
		}

		default: break;
		}
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
#ifdef XB_IOT1
		String tmp = FSS("PCF8574_1(");
		uint8_t b=expander_1.read();
		for (uint8_t i = 7; i <= 8; i--)
		{
			if (((b >> i) & 1) == 1) tmp = tmp + '0';
			else tmp = tmp + '1';
		}
		tmp = tmp + ')';
		*(Am->Data.PointerString) = tmp;
#else
		*(Am->Data.PointerString) = FSS("...");
#endif
		res = true;
	}
	default:;
	}
	return res;
}

void XB_BOARD_Setup(void)
{
#ifdef ESP8266
	Serial_setDebugOutput(false);
#endif
	Serial_begin(SerialBoard_BAUD);
#ifdef ESP8266
	while (!Serial_availableForWrite())
	{
		yield();
	}
	for (int i = 0; i < 32; i++) {
		Serial_write(0);
		delay(1);
	}
#endif

#ifdef  BOARD_LED_LIFE_PIN
	pinMode(BOARD_LED_LIFE_PIN,OUTPUT);
#endif
#ifdef  BOARD_LED_OKSEND_PIN
	pinMode(BOARD_LED_OKSEND_PIN, OUTPUT);
#endif

	rs.reserve(32);

#ifdef XB_GUI
	ScreenText.Clear();
	ScreenText.PutText(FSS("Start...\n"));
#else
	Serial_print(FSS("\n\nStart...\n"));
#endif
	DateTime = 0;
	DateTimeStart = 0;
}

void XB_BOARD_DoLoop(void)
{

#ifdef XB_WIFI
	// Sprawdzenie czy nie nast¹pi³o roz³¹czenie z punktem WiFi
	if (WIFI_CheckDisconnectWiFi())
	{
		WIFI_HardDisconnect();
		return;
	}
	// Sprawdzenie stanu krytycznego stosu
	else if (board.CheckCriticalFreeHeap())
	{
		WIFI_HardDisconnect();
		return;
	}
#endif
	// Zamiganie
	board.handle();

	if (Tick_ESCKey != 0)
	{
		if (SysTickCount - Tick_ESCKey > 100)
		{
			Tick_ESCKey = 0;
			TerminalFunction = 0;
			board.SendKeyFunctionPress(KF_ESC, 0,&XB_BOARD_DefTask,true);
		}
	}

#ifdef XB_GUI
	DEF_WAITMS_VAR(xbl1);
	BEGIN_WAITMS(xbl1, 1000)
	{
		if (winHandle0 != NULL)
		{
			winHandle0->RepaintDataCounter++;
		}
		RESET_WAITMS(xbl1);
	}
	END_WAITMS(xbl1)
#endif
}

//------------------------------------------------------------------------------------------------------------
TXB_board::TXB_board(uint8_t ATaskDefCount)
{
	iteratetask_procedure = false;
	NoTxCounter = 0;
	TXCounter = 0;
	MaximumFreeHeapInLoop = getFreeHeap();

	TaskDefCount = ATaskDefCount;
	TaskDef = new PTaskDef[ATaskDefCount];

	MinimumFreeHeapInLoop = getFreeHeap();

	for (int i = 0; i < TaskDefCount; i++)
	{
		TaskDef[i] = NULL;
	}
#ifdef ESP8266
	SysTickCount_init();
	DateTimeSecond_init();
#endif
}

TXB_board::~TXB_board()
{
	delete(TaskDef);
	TaskDef = NULL;

}


bool TXB_board::pinMode(uint16_t pin, WiringPinMode mode)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinMode;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Mode = mode;

	
	if (!SendMessageToAllTask(&mb,doONLYINTERESTED))
	{
		Log(FSS("\n[BOARD] GPIO pinMode Error.\n"));
		return false;
	}

	return true;
}

void TXB_board::digitalWrite(uint16_t pin, uint8_t value)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinWrite;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Value = value;


	if (!SendMessageToAllTask(&mb, doONLYINTERESTED))
	{
		Log(FSS("\n[BOARD] GPIO digitalWrite Error.\n"));
	}
}

uint8_t TXB_board::digitalRead(uint16_t pin)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinRead;
	mb.Data.GpioData.NumPin = pin;

	if (!SendMessageToAllTask(&mb, doONLYINTERESTED))
	{
		Log(FSS("\n[BOARD] GPIO digitalWrite Error.\n"));
		return 0;
	}
	else
	{
		return mb.Data.GpioData.ActionData.Value;
	}
}

uint8_t TXB_board::digitalToggle(uint16_t pin)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinToggle;
	mb.Data.GpioData.NumPin = pin;

	if (!SendMessageToAllTask(&mb, doONLYINTERESTED))
	{
		Log(FSS("\n[BOARD] GPIO digitalToggle Error.\n"));
		return 0;
	}
	else
	{
		return mb.Data.GpioData.ActionData.Value;
	}
}


int TXB_board::DefTask(TTaskDef *Ataskdef,uint8_t Aid)
{
	int rez = -1;
	for (int i = 0; i < TaskDefCount; i++)
	{
		if (TaskDef[i] == NULL)
		{
			TaskDef[i] = Ataskdef;
			Ataskdef->IDTask = Aid;
			if (TaskDef[i]->dosetup != NULL)
			{
				if (TaskDef[i]->dosetupRC == 0)
				{
					TaskDef[i]->dosetupRC++;
					{
						TaskDef[i]->dosetup();
					}
					TaskDef[i]->dosetupRC--;
				}
			}
			rez= i;
			break;

		}
	}
	if (rez == -1)
	{
		Log(FSS("Out Of Task Table.\n"));
	}
	return rez;
}

void TXB_board::IterateTask(void)
{
	iteratetask_procedure = true;
	static uint32_t CurrentIndxRunTask = 0;

	// zapamiêtanie iloœci wolnej pamiêci ram i minimalnego stanu
	
	FreeHeapInLoop = getFreeHeap();
	if (FreeHeapInLoop < MinimumFreeHeapInLoop)
	{
		MinimumFreeHeapInLoop = FreeHeapInLoop;
	}

	// Uruchomienie zadañ w tzw realtime
	for (int i = 0; i < TaskDefCount; i++)
	{
		if (TaskDef[i] != NULL)
		{
			if (TaskDef[i]->doloop != NULL)
			{
				if (TaskDef[i]->Priority == 0)
				{
					if (TaskDef[i]->doloopRC == 0)
					{
						TaskDef[i]->doloopRC++;
						{
							TaskDef[i]->doloop();
						}
						TaskDef[i]->doloopRC--;
					}
				}
			}
		}
	}

	// Sprawdzenie czy zdefiniowano zadanie
	if (TaskDef[CurrentIndxRunTask] != NULL)
	{
		if (TaskDef[CurrentIndxRunTask]->doloop != NULL)
		{
			if (TaskDef[CurrentIndxRunTask]->doloopRC == 0)
			{
				if (TaskDef[CurrentIndxRunTask]->Priority > 0)
				{
					TaskDef[CurrentIndxRunTask]->CounterPriority++;
					if (TaskDef[CurrentIndxRunTask]->CounterPriority >= TaskDef[CurrentIndxRunTask]->Priority)
					{
						TaskDef[CurrentIndxRunTask]->CounterPriority = 0;
						TaskDef[CurrentIndxRunTask]->doloopRC++;
						{
							TaskDef[CurrentIndxRunTask]->doloop();
						}
						TaskDef[CurrentIndxRunTask]->doloopRC--;
					}
				}
			}
		}

		CurrentIndxRunTask++;
		if (CurrentIndxRunTask >= TaskDefCount)
		{
			CurrentIndxRunTask = 0;
		}
	}
	else
	{
		CurrentIndxRunTask = 0;
	}

}

bool TXB_board::GetTaskString(TMessageBoard *Amb,TTaskDef *ATaskDef, String &APointerString)
{
	Amb->Data.PointerString = &APointerString;

	if (SendMessageToTask(ATaskDef, Amb,true))
	{
		return true;
	}
	else
	{
		return false;
	}
	
}

bool TXB_board::GetTaskStatusString(TTaskDef *ATaskDef, String &APointerString)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GET_TASKSTATUS_STRING;
	return GetTaskString(&mb, ATaskDef, APointerString);
}

bool TXB_board::GetTaskName(TTaskDef *ATaskDef,String &APointerString)
{
	TMessageBoard mb;
	mb.IDMessage = IM_GET_TASKNAME_STRING;
	return GetTaskString(&mb, ATaskDef, APointerString);
}

void TXB_board::SendKeyFunctionPress(TKeyboardFunction Akeyfunction,char Akey)
{
	TMessageBoard mb;
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToAllTask(&mb, doONLYINTERESTED);
}

void TXB_board::SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey,TTaskDef *Ataskdef,bool Aexcludethistask)
{
	TMessageBoard mb;
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	if (Aexcludethistask)
	{
		SendMessageToAllTask(&mb, doONLYINTERESTED,Ataskdef);
	}
	else
	{
		SendMessageToAllTask(&mb, doONLYINTERESTED);
		if (Ataskdef->domessageRC > 0)
		{
			SendMessageToTask(Ataskdef, &mb, true);
		}
	}
}

void TXB_board::SendKeyPress(char Akey)
{
	TMessageBoard mb;
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode=Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToAllTask(&mb, doONLYINTERESTED);
}

void TXB_board::SendKeyPress(char Akey, TTaskDef *Ataskdef)
{
	TMessageBoard mb;
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToTask(Ataskdef,&mb, true);
}

bool TXB_board::SendMessageToTask(TTaskDef *ATaskDef, TMessageBoard *mb,bool Arunagain)
{
	bool res = false;
	if (ATaskDef != NULL)
	{
		if (ATaskDef->domessage != NULL)
		{
			
			if ((ATaskDef->domessageRC == 0) || (Arunagain==true))
			{
				ATaskDef->domessageRC++;
				{
					res = ATaskDef->domessage(mb);
				}
				ATaskDef->domessageRC--;
			}
		}
	}
	return res;
}

bool TXB_board::SendMessageToTaskByID(uint8_t Aidtask, TMessageBoard *mb, bool Arunagain)
{
	bool res = false;

	for (int i = 0; i < TaskDefCount; i++)
	{
		if (TaskDef[i] != NULL)
		{
			if (TaskDef[i]->IDTask == Aidtask)
			{
				res = SendMessageToTask(TaskDef[i], mb, true);
				if (res) break;
			}
		}
	}
	return res;
}
	
bool TXB_board::SendMessageToAllTask(TIDMessage AidMessage, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask)
{
	TMessageBoard mb;
	mb.IDMessage = AidMessage;
	mb.Data.uData32 = 0;
	return SendMessageToAllTask(&mb, ADoMessageDirection, Aexcludetask);
}

bool TXB_board::SendMessageToAllTask(TMessageBoard *mb, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask)
{
	bool res = false;
	switch (ADoMessageDirection)
	{
		case doONLYINTERESTED:
		{
			uint8_t isinterested = 0;
			for (int i = 0; i < TaskDefCount; i++)
			{
				if (TaskDef[i] != NULL)
				{
					if (Aexcludetask != TaskDef[i])
					{
						if (TaskDef[i]->domessage != NULL)
						{
							if (TaskDef[i]->domessageRC == 0)
							{
								if (TaskDef[i]->LastIDMessage == mb->IDMessage)
								{
									isinterested++;

									TaskDef[i]->domessageRC++;
									{
										res = TaskDef[i]->domessage(mb);
									}
									TaskDef[i]->domessageRC--;

									if (!res)
									{
										TaskDef[i]->LastIDMessage = IM_IDLE;
										isinterested--;
									}

								}
							}
						}
					}
				}
			}
			if (isinterested == 0)
			{
				for (int i = 0; i < TaskDefCount; i++)
				{
					if (TaskDef[i] != NULL)
					{
						if (Aexcludetask != TaskDef[i])
						{
							if (TaskDef[i]->domessage != NULL)
							{
								if (TaskDef[i]->domessageRC == 0)
								{

									//if (TaskDef[i]->LastIDMessage == mb->IDMessage)
									{

										TaskDef[i]->domessageRC++;
										{
											res = TaskDef[i]->domessage(mb);
										}
										TaskDef[i]->domessageRC--;

										if (res)
										{
											TaskDef[i]->LastIDMessage = mb->IDMessage;
											break;
										}

									}
								}
							}
						}
					}
				}
			}
			break;
		}
		case doFORWARD:
		{
			for (int i = 0; i < TaskDefCount; i++)
			{
				if (TaskDef[i] != NULL)
				{
					if (Aexcludetask != TaskDef[i])
					{

						if (TaskDef[i]->domessage != NULL)
						{
							if (TaskDef[i]->domessageRC == 0)
							{
								TaskDef[i]->domessageRC++;
								{
									res = TaskDef[i]->domessage(mb);
								}
								TaskDef[i]->domessageRC--;
								if (res) break;
							}
						}
					}
				}
			}
			break;
		}

		case doBACKWARD:
		{
			for (int i = TaskDefCount - 1; i > -1; i--)
			{
				if (TaskDef[i] != NULL)
				{
					if (Aexcludetask != TaskDef[i])
					{

						if (TaskDef[i]->domessage != NULL)
						{
							if (TaskDef[i]->domessageRC == 0)
							{
								TaskDef[i]->domessageRC++;
								{
									res = TaskDef[i]->domessage(mb);
								}
								TaskDef[i]->domessageRC--;
								if (res) break;
							}
						}
					}
				}
			}
			break;
		}
		default: break;
	}
	return res;
}


void TXB_board::setString(char *dst, const char *src, int max_size) 
{
	max_size--;
	if (src == NULL) {
		dst[0] = 0;
		return;
	}

	int size = strlen(src);

	if (size + 1 > max_size)
		size = max_size - 1;

	memcpy(dst, src, size);
	dst[size] = 0;
}

uint32_t TXB_board::getFreeHeap()
{
#ifdef ESP8266
	return ESP.getFreeHeap();
#endif
#ifdef ARDUINO_ARCH_STM32F1
	uint32_t size = 0; 
	uint32_t ADR = 0;
	uint8_t a = 1;

	ADRESS_STACK = (uint32_t)&a;
	
	ADRESS_HEAP = (uint32_t)malloc(1);
	free((void *)ADRESS_HEAP);

	size = ADRESS_STACK - ADRESS_HEAP;

	while (size > 0)
	{
		ADR = (uint32_t)malloc(size - 32);
		if (ADR != 0)
		{
			free((void *)ADR);
			return size - 32;
		}
		if (size > 32) size -= 32;
		else size = 0;
	}
	return  0;
#endif


}

bool TXB_board::CheckCriticalFreeHeap(void)
{
	if (getFreeHeap() < (BOARD_CRITICALFREEHEAP))
	{
		PrintTimeFromRun();
		Serial_println(FSS(" CRITICAL FREE STACK...set disconnect."));
		return true;
	}
	else
	{
		return false;
	}

	return false;
}

void TXB_board::SysTickCount_init(void)
{
#ifdef ESP8266
	SysTickCount_ticker.attach_ms(1, SysTickCount_proc);
#endif
	LastActiveTelnetClientTick = 0;
}

void TXB_board::DateTimeSecond_init(void)
{
#ifdef ESP8266
	DateTimeSecond_ticker.attach(1, DateTimeSecond_proc);
#endif
	
}

void TXB_board::handle(void)
{
	DEF_WAITMS_VAR(LOOPW);
	BEGIN_WAITMS_PREC(LOOPW, 1000)
	{
#ifdef  BOARD_LED_LIFE_PIN
		digitalToggle(BOARD_LED_LIFE_PIN);
#endif
#ifdef  BOARD_LED_OKSEND_PIN
		digitalWrite(BOARD_LED_OKSEND_PIN, LOW);
#endif
#ifdef ARDUINO_ARCH_STM32F1
		DateTime++;
#endif
#ifdef XB_GUI
		if (winHandle0 != NULL)
		{
			winHandle0->RepaintDataCounter++;
		}
#endif
	}
	END_WAITMS_PREC(LOOPW);

	if (Serial_available())
	{
		if (TerminalFunction == 0)
		{
			SendKeyPress((char)Serial_read());
		}
		else
		{
			
			SendKeyPress((char)Serial_read(),&XB_BOARD_DefTask);
		}
	}
}

void TXB_board::cmdparse(String Ars)
{
	if (Ars == FSS("asc"))
	{
		//GUI_ClearScreen();
		for (int i = 32; i < 256; i++)
		{
			Log((char)i);
		}
	}
	else if (Ars == FSS("echo"))
	{
		PrintTimeFromRun();
		Log(FSS("ECHO\r\n>"));
	}
	/*
#ifdef ESP8266
	else if ((Ars == FSS("printdiag")) || (Ars == FSS("pd")))
	{
		PrintDiag();
		Log(FSS(">"));

	}
	else if ((Ars == FSS("help")) || (Ars == FSS("h")))
	{
		PrintHelp();
		Log(FSS(">"));
	}
	else if ((Ars == FSS("restart")))
	{
		ESP.restart();
		delay(5000);
	}
	else if ((Ars == FSS("r0_on")))
	{
		RELAY_SET(1, true);
#if defined(SUPLADEVICE_SUPPORT)
		SUPLA_UpdateValue();
#endif
		Log(FSS(">"));
	}
	else if ((Ars == FSS("r0_off")))
	{
		RELAY_SET(1, false);
#if defined(SUPLADEVICE_SUPPORT)
		SUPLA_UpdateValue();
#endif
		Log(FSS(">"));
	}
	else if ((Ars == FSS("r1_on")))
	{
		RELAY_SET(2, true);
#if defined(SUPLADEVICE_SUPPORT)
		SUPLA_UpdateValue();
#endif
		Log(FSS(">"));
	}
	else if ((Ars == FSS("r1_off")))
	{
		RELAY_SET(2, false);
#if defined(SUPLADEVICE_SUPPORT)
		SUPLA_UpdateValue();
#endif
		Log(FSS(">"));
	}
#if defined(GUI_SUPPORT)
	else if ((Ars == FSS("showpanel")) || (Ars == FSS("sp")))
	{
		GUI_Show();
		Log(FSS(">"));
	}
	else if ((Ars == FSS("hidepanel")) || (Ars == FSS("hp")))
	{
		GUI_Hide();
		Log(FSS(">"));
	}
	else if ((Ars == FSS("clearscreen")) || (Ars == FSS("cs")))
	{
		//GUI_ClearScreen();
		Log(FSS(">"));
	}
#endif
	else if ((Ars == FSS("disconnectinternet")) || (Ars == FSS("di")))
	{
		WIFI_SetDisconnectInternet();
		Log(FSS(">"));
	}
	else if ((Ars == FSS("disconnectwifi")) || (Ars == FSS("dw")))
	{
		WIFI_HardDisconnect();
		Log(FSS(">"));
	}
#endif
	*/
}

void TXB_board::Serial_WriteChar(char Achr)
{
	while(1)
	{
		if (Serial_availableForWrite()>=1)
		{
			Serial_print(Achr);
			break;
		}
		
		if (!iteratetask_procedure)
		{
			break;
		}
		else
		{
			delay(10);
		}

	}
}

void TXB_board::Log(char Achr)
{
	if (NoTxCounter==0) TXCounter++;
#ifdef TELNET_SUPPORT
	TELNET_writechar(Achr);
#endif
	Serial_WriteChar(Achr);
}

void TXB_board::Log(const char *Atxt,bool puttime)
{
	int len = strlen(Atxt);
	if (NoTxCounter==0) TXCounter += len;
	if (len == 0) return;

	String txttime = "";
	if (puttime)
	{
		GetTimeIndx(txttime, DateTime - DateTimeStart);
		txttime = "[" + txttime + "] ";
	}

#ifdef TELNET_SUPPORT
	if (puttime) TELNET_writestr((const uint8_t *)txttime.c_str());
	

	TELNET_writestr((const uint8_t *)Atxt);
#endif
	const char *p;
	
	if (puttime)
	{
		p = txttime.c_str();
		while (*p)
		{
			Serial_WriteChar(*p);
			p++;
		}
	}

	p = Atxt;
	while (*p) 
	{
		Serial_WriteChar(*p);
		p++;
	}
}

void TXB_board::Log(cbufSerial *Acbufserial)
{
#ifdef TELNET_SUPPORT
	uint8_t tmpch;
	while (Acbufserial->available()>0)
	{
		tmpch = Acbufserial->read();
		TELNET_writechar(tmpch);
		Serial_WriteChar(tmpch);
		if (NoTxCounter==0) TXCounter++;
	}
#else
	while (Acbufserial->available()>0)
	{
		Serial_WriteChar(Acbufserial->read());
		if (NoTxCounter==0) TXCounter++;
	}
#endif
}

void TXB_board::Log_TimeStamp()
{
	PrintTimeFromRun();
}

void TXB_board::PrintTimeFromRun(cbufSerial *Astream)
{
	Astream->print('[');
	GetTimeIndx(Astream, DateTime - DateTimeStart);
	Astream->print(FSS("] "));
}

void TXB_board::PrintTimeFromRun(void)
{
	cbufSerial Astream(32);
	Astream.print(FSS("["));
	GetTimeIndx(&Astream, DateTime - DateTimeStart);
	Astream.print(FSS("] "));
	Log(&Astream);
}

void TXB_board::PrintDiag(void)
{
#ifdef ESP8266
	String t;
	cbufSerial tmp(1024);
	
	tmp.print(FSS("\n\rPrint Diag:\n\r"));
	tmp.print(FSS("\n\r-----------"));
	
	t = ""; GetTimeIndx(t, DateTime - DateTimeStart);
	tmp.printf(FSS("\n\rTime running:   (%s)"), t.c_str());

	tmp.printf(FSS("\n\rIP:                %s"), WiFi.localIP().toString().c_str());
	tmp.printf(FSS("\n\rRSSI:              %d"), WiFi.RSSI());
	tmp.printf(FSS("\n\rFree heap:         %d"), FreeHeapInLoop);
	tmp.printf(FSS("\n\rMinimum free heap: %d"), MinimumFreeHeapInLoop);
	tmp.printf(FSS("\n\r\n\rReset Info:        \n\r%s"), ESP.getResetInfo().c_str());


	tmp.print(FSS("\n\r--------\n\rESPdiag:\n\r\n\r"));
	WiFi.printDiag(tmp);
	tmp.print(FSS("\n\r"));

	Log(&tmp);
#endif
}


