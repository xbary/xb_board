#include <xb_board.h>


TXB_board board;

TTaskDef XB_BOARD_DefTask = {0,&XB_BOARD_Setup,&XB_BOARD_DoLoop,&XB_BOARD_DoMessage};

volatile uint32_t DateTimeUnix;
volatile uint32_t DateTimeStart;
volatile uint32_t __SysTickCount;

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif

#ifdef ARDUINO_ARCH_STM32F1
extern "C" {
#include <string.h>
#include <stdlib.h>
}
#endif

#ifdef XB_GUI
#include <xb_GUI.h>
TWindowClass *winHandle0;

#include <xb_GUI_Gadget.h>
TGADGETMenu *menuHandle0;
TGADGETMenu *menuHandle1;
#endif

//------------------------------------------------------------------------------------------------------------
#if defined(ESP8266) 
Ticker SysTickCount_ticker;
Ticker DateTimeSecond_ticker;
#endif

uint32_t Tick_ESCKey = 0;
uint8_t TerminalFunction = 0;

#if defined(ESP8266)

void SysTickCount_proc(void)
{
	__SysTickCount++;
}

void DateTimeSecond_proc(void)
{
	DateTimeUnix++;
}
#endif

bool showasc = false;

bool TXB_board::SetPinInfo(uint16_t Anumpin, uint8_t Afunction, uint8_t Amode,bool Alogwarn)
{
	if ((Anumpin >= 0) && (Anumpin < BOARD_NR_GPIO_PINS))
	{
		if (PinInfoTable != NULL)
		{
			if (PinInfoTable[Anumpin].use==1)
			{
				if ((setup_procedure==true ) || (Alogwarn==true))
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
		Log("ERROR out of num pin",true,true, tlError);
		return false;	
	}
	return true;
}
	
bool XB_BOARD_DoMessage(TMessageBoard *Am)
{
	bool res = false;
	static uint8_t LastKeyCode = 0;
	
	switch (Am->IDMessage)
	{
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		if (Am->Data.FreePTR == winHandle0) winHandle0 = NULL;
		if (Am->Data.FreePTR == menuHandle0) menuHandle0 = NULL;
		if (Am->Data.FreePTR == menuHandle1) menuHandle1 = NULL;
#endif
		res = true;
		break;
	}
	case IM_GPIO:
	{
		switch (Am->Data.GpioData.GpioAction)
		{
		case gaPinMode:
		{
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
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
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
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
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
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
			if ((Am->Data.GpioData.NumPin >= 0) && (Am->Data.GpioData.NumPin < BOARD_NR_GPIO_PINS))
			{
				Am->Data.GpioData.ActionData.Value = !digital_Read(Am->Data.GpioData.NumPin);
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
								board.SendKeyFunctionPress(KF_BACKSPACE, 0, &XB_BOARD_DefTask);
								TerminalFunction = 0;
								break;
							}
						case 10:
							{
								if (LastKeyCode != 13)
								{
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
								TerminalFunction = 1;
								Tick_ESCKey = SysTickCount;
								break;
							}
						case 7:
							{
								Tick_ESCKey = 0;
								board.SendKeyFunctionPress(KF_ESC, 0, &XB_BOARD_DefTask);
								TerminalFunction = 0;
								break;
							}
						case 9:
							{
								Tick_ESCKey = 0;
								board.SendKeyFunctionPress(KF_TABNEXT, 0, &XB_BOARD_DefTask);
								TerminalFunction = 0;
								break;
							}
						case 255:
						case 0:
							{
								Tick_ESCKey = 0;
								TerminalFunction = 0;
								break;
							}


						default:
							{
								Tick_ESCKey = 0;
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
					menuHandle0 = GUIGADGET_CreateMenu(&XB_BOARD_DefTask, 0);
#endif
					res = true;
				}

			
				LastKeyCode = Am->Data.KeyboardData.KeyCode;

			}
			break;
		}
#ifdef XB_GUI
	case IM_MENU:
	{
		switch (Am->Data.MenuData.TypeMenuAction)
		{
		case tmaOPEN_MAINMENU:
		{
			menuHandle1 = GUIGADGET_CreateMenu(&XB_BOARD_DefTask, 1);
			res = true;
			break;
		}
		case tmaGET_INIT_MENU:
		{
			BEGIN_MENUINIT(0);
			{
				DEF_MENUINIT(board.TaskCount, 0, 20,-1,0,true);
				res = true;
			}
			END_MENUINIT();
			BEGIN_MENUINIT(1);
			{
				DEF_MENUINIT(2, 0, 15,-1,0,true);
				res = true;
			}
			END_MENUINIT();
			break;
		}
		case tmaGET_ITEM_MENU_STRING:
		{
			BEGIN_MENUITEMNAME(0);
			{
				String n = "";
				TTask *t = board.GetTaskByIndex(MenuItemIndex);
				if (t != NULL)
				{

					if (board.GetTaskName(t->TaskDef, n))
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
				DEF_MENUITEMNAME(MenuItemIndex, n);
				res = true;
			}
			END_MENUITEMNAME();

			BEGIN_MENUITEMNAME(1);
			{
				DEF_MENUITEMNAME(0, FSS("Restart MCU"));
				DEF_MENUITEMNAME(1, FSS("Save All Config..."));
				res = true;
			}
			END_MENUITEMNAME();
			break;
		}
		case tmaCLICK_ITEM_MENU:
		{
			BEGIN_MENUCLICK(0)
			{
				TTask *t = board.GetTaskByIndex(MenuItemIndex);
				if (t != NULL)
				{
					bool r = GUIGADGET_OpenMainMenu(t->TaskDef);
				}
				res = true;
				break;
			}
			END_MENUCLICK()

			BEGIN_MENUCLICK(1)
			{
				EVENT_MENUCLICK(0)
				{
#if defined(ESP8266) || defined(ESP32)
					ESP.restart();
					delay(5000);
#elif defined(ARDUINO_ARCH_STM32F1)
					nvic_sys_reset();
#else
					board.Log(FSS("\nReset no support!\n"));
#endif
					res = true;
					break;
				}
				EVENT_MENUCLICK(1)
				{
					board.SendSaveConfigToAllTask();
				}
			}
			END_MENUCLICK()
			break;
		}
		case tmaGET_CAPTION_MENU_STRING:
		{
			BEGIN_MENUCAPTION(0)
			{
				DEF_MENUCAPTION("TASK LIST...");
			}
			END_MENUCAPTION()
				else
			BEGIN_MENUCAPTION(1)
			{
				DEF_MENUCAPTION("BOARD MAIN MENU");
			}
			END_MENUCAPTION()
			
			res = true;
			break;
		}
		default: break;
		}
		break;
	}
	case IM_WINDOW:
	{
		switch (Am->Data.WindowData.WindowAction)
		{
		case waCreate:
		{
			if (Am->Data.WindowData.ID == 0)
			{
				Am->Data.WindowData.ActionData.Create.X = -1;
				Am->Data.WindowData.ActionData.Create.Y = 0;
				Am->Data.WindowData.ActionData.Create.Width = 46;
				Am->Data.WindowData.ActionData.Create.Height = board.TaskCount + 14;

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
#ifdef ESP32
#ifdef BOARD_HAS_PSRAM
	
				*(Am->Data.WindowData.ActionData.GetCaption.PointerString) = FSS("BOARD (ESP32 wROVER, 240Mhz)");
#else
				*(Am->Data.WindowData.ActionData.GetCaption.PointerString) = FSS("BOARD (ESP32, 240Mhz)");
#endif
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
					winHandle0->PutStr(board.DeviceName.c_str());

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

					winHandle0->PutStr(0, 5, FSS("DEVICE ID "));
					
					{
						TUniqueID ID = board.GetUniqueID();
						char strid[25];
						xb_memoryfill(strid, 25, 0);
						
						uint8tohexstr(strid,(uint8_t *)&ID, 8,':');

						winHandle0->PutStr(strid);
					}

					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 6, FSS("FREEpsram:"));
					winHandle0->PutStr(18, 6, FSS("MINpsram:"));
					winHandle0->PutStr(18, 7, FSS("MAXpsram:"));
					winHandle0->SetBoldChar();
					winHandle0->SetTextColor(tfcYellow);
#ifdef ESP32					
					winHandle0->PutStr(String(board.MaximumFreePSRAMInLoop).c_str());
#else
					//winHandle0->PutStr("---");
#endif
					winHandle0->SetNormalChar();
					winHandle0->SetTextColor(tfcWhite);
					winHandle0->PutStr(0, 7, FSS("MEM USE:"));

					winHandle0->PutStr(0, 8, FSS("OUR RESERVED BLOCK:"));
					//--------------
					{
						int y;
						String name;
						y = 10;
						winHandle0->PutStr(0, y, FSS("__________________________________"));
						y++;
						winHandle0->PutStr(0, y, FSS("TASK NAME"));
						winHandle0->PutStr(15, y, FSS("STATUS"));
						winHandle0->SetTextColor(tfcGreen);
						y++;
						for (int i = 0; i < board.TaskCount; i++)
						{
							TTask *t = board.GetTaskByIndex(i);
							if (t != NULL)
							{

								if (board.GetTaskName(t->TaskDef, name))
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

#ifdef ESP32
					winHandle0->PutStr(10, 6, String(board.FreePSRAMInLoop).c_str());
					winHandle0->PutChar(' ');
					winHandle0->PutStr(27, 6, String(board.MinimumFreePSRAMInLoop).c_str());
					winHandle0->PutChar(' ');

					winHandle0->PutStr(9, 7, String((uint32_t)(100 - (board.FreePSRAMInLoop / (board.MaximumFreePSRAMInLoop / 100L)))).c_str());
					winHandle0->PutStr(FSS("% "));
#else
				
#endif
					winHandle0->PutStr(20,8,String(board.OurReservedBlock).c_str(),8);
					
					//---------------
					{
						String name;
						int y;

						y = 10;

						y += 2;
						for (int i = 0; i < board.TaskCount; i++)
						{
							TTask *t = board.GetTaskByIndex(i);
							if (t != NULL)
							{

								if (board.GetTaskStatusString(t->TaskDef, name))
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
		*(Am->Data.PointerString) = String("TC:" + String(board.TaskCount) + " / GFPEC:" + String(board.GETFREEPSRAM_ERROR_COUNTER));
		res = true;
	}
	case IM_STREAM:
	{
		switch (Am->Data.StreamData.StreamAction)
		{
		case saGet:
		{
			int av = Serial_available();
			if (av > 0)
			{
				uint8_t ch = 0;
				uint8_t count = 0;
				while (av > 0)
				{
					ch = (uint8_t)Serial_read();
					((uint8_t *)Am->Data.StreamData.Data)[count] = ch;
					count++;
					if (count >= Am->Data.StreamData.Length) break;
					av =  Serial_available();
				}
				Am->Data.StreamData.LengthResult = count;
			}
			else
			{
				Am->Data.StreamData.LengthResult = 0;
			}
			res = true;
			break;
		}
		case saPut:
		{
			uint32_t indx = 0;
			uint32_t count = Am->Data.StreamData.Length;
			Am->Data.StreamData.LengthResult = 0;
			while (count > 0)
			{
				Am->Data.StreamData.LengthResult += Serial_write(((uint8_t *)Am->Data.StreamData.Data)[indx]);
				count--;
				indx++;
				yield();
					
			}
			res = true;
			break;
		}
		}
		break;
	}
	default:;
	}
	
	return res;
}

void XB_BOARD_Setup(void)
{
#ifdef BOARD_HAS_PSRAM
	board.psram2m = board._malloc_psram((1024 * 1024) * 2);
#endif

#if defined(ESP8266)
	board.SysTickCount_init();
	board.DateTimeSecond_init();
#endif

// Wy��czenie komunikat�w od CORE ESPowego
#ifdef Serial_setDebugOutput
	Serial_setDebugOutput(false);
#endif

// Uruchomienie UARTa podstawowego
#if defined(SerialBoard_RX_PIN) && defined(SerialBoard_TX_PIN)
	Serial_begin(SerialBoard_BAUD, SERIAL_8N1, SerialBoard_RX_PIN, SerialBoard_TX_PIN);
	board.SetPinInfo(SerialBoard_RX_PIN,FUNCTIONPIN_UARTRXTX,MODEPIN_OUTPUT);
	board.SetPinInfo(SerialBoard_TX_PIN, FUNCTIONPIN_UARTRXTX, MODEPIN_OUTPUT);
#else
	Serial_begin(SerialBoard_BAUD);
#endif

// procedura inicjuj�ca UART ... 
#if defined(ESP8266) || defined(ESP32)

#ifdef Serial_availableForWrite 
	while (!Serial_availableForWrite())
	{
		delay(1);
	}
#endif
	for (int i = 0; i < 32; i++) {
		Serial_write(0);
		delay(1);
	}
#endif



	
#ifdef BOARD_LED_LIFE_PIN
	board.pinMode(BOARD_LED_LIFE_PIN,OUTPUT);
#endif

#ifdef BOARD_LED_TX_PIN
	board.Tick_TX_BLINK = 0;
	board.pinMode(BOARD_LED_TX_PIN, OUTPUT);
#if defined(BOARD_LED_TX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_TX_PIN, (BOARD_LED_TX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_TX_PIN, LOW);
#endif
#endif

#ifdef BOARD_LED_RX_PIN
	board.Tick_RX_BLINK = 0;
	board.pinMode(BOARD_LED_RX_PIN, OUTPUT);
#if defined(BOARD_LED_RX_STATUS_OFF)
	board.digitalWrite(BOARD_LED_RX_PIN, (BOARD_LED_RX_STATUS_OFF));
#else
	board.digitalWrite(BOARD_LED_RX_PIN, LOW);
#endif
#endif

#if defined(BOARD_LED_RX_PIN) || defined(BOARD_LED_TX_PIN)
	#ifdef TICK_LED_BLINK
	board.TickEnableBlink= TICK_LED_BLINK;
#else
	board.TickEnableBlink = 250;
#endif
#endif

	DateTimeUnix = 0;
	DateTimeStart = 0;

	board.Log(FSS("Start..."),true,true);
	
#ifdef XB_GUI
	board.AddTask(&XB_GUI_DefTask);
#endif
}

uint32_t XB_BOARD_DoLoop(void)
{
	// Sprawdzenie  przy pierwszym  wejsciu do p�tli loop ilo�ci wolnej pami�ci
	if (board.MaximumFreeHeapInLoop == 0)
	{
#ifdef ESP32
		board.FreePSRAMInLoop = board.getFreePSRAM();
		board.MinimumFreePSRAMInLoop = board.FreePSRAMInLoop;
		board.MaximumFreePSRAMInLoop = board.FreePSRAMInLoop;
	
		board.FreeHeapInLoop = board.getFreeHeap();
		board.MaximumFreeHeapInLoop = board.FreeHeapInLoop;
		board.MinimumFreeHeapInLoop = board.FreeHeapInLoop;
#else

		board.MaximumFreeHeapInLoop = board.getFreeHeap();
		board.MinimumFreeHeapInLoop = board.getFreeHeap();

#endif
	}

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
	if (winHandle0 != NULL)
	{
		DEF_WAITMS_VAR(xbl1);
		BEGIN_WAITMS(xbl1, 1000)
		{
			winHandle0->RepaintDataCounter++;
		}
		END_WAITMS(xbl1)
	}
#endif

	return 0;
}

//------------------------------------------------------------------------------------------------------------
TXB_board::TXB_board()
{
	TaskList = NULL;
	TaskCount = 0;
	lastfreepsram = 0;
	OurReservedBlock = 0;
	GETFREEPSRAM_ERROR_COUNTER = 0;
	iteratetask_procedure = false;
	MaximumMallocPSRAM = 0;
	CurrentIterateTask = NULL;
	CurrentTask = NULL;
	NoTxCounter = 0;
	TXCounter = 0;
	doAllInterruptRC = 0;
	Default_StreamTaskDef = &XB_BOARD_DefTask;
	Default_SecStreamTaskDef = NULL;
	Default_ShowLogInfo = true;
	Default_ShowLogWarn = true;
	Default_ShowLogError = true;
	HandleFrameTransportInGetStream = true;
	DeviceName = DEVICE_NAME;
	
	PinInfoTable = (TPinInfo *)_malloc(BOARD_NR_GPIO_PINS);
	
	
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

bool TXB_board::pinMode(uint16_t pin, WiringPinMode mode)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinMode;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Mode = mode;

	
	if (!SendMessageToAllTask(&mb, doFORWARD)) //doONLYINTERESTED
	{
//		Log(FSS("\n[BOARD] GPIO pinMode Error.\n"));
//		return false;
	}

	return true;
}

void TXB_board::digitalWrite(uint16_t pin, uint8_t value)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinWrite;
	mb.Data.GpioData.NumPin = pin;
	mb.Data.GpioData.ActionData.Value = value;


	if (!SendMessageToAllTask(&mb, doFORWARD)) //doONLYINTERESTED
	{
	//	Log(FSS("\n[BOARD] GPIO digitalWrite Error.\n"));
	}
}

uint8_t TXB_board::digitalRead(uint16_t pin)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinRead;
	mb.Data.GpioData.NumPin = pin;

	if (!SendMessageToAllTask(&mb, doFORWARD)) //doONLYINTERESTED
	{
//		Log(FSS("\n[BOARD] GPIO digitalWrite Error.\n"));
//		return 0;
//	}
//	else
//	{
		return mb.Data.GpioData.ActionData.Value;
	}
}

uint8_t TXB_board::digitalToggle(uint16_t pin)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GPIO;
	mb.Data.GpioData.GpioAction = gaPinToggle;
	mb.Data.GpioData.NumPin = pin;

	if (!SendMessageToAllTask(&mb, doFORWARD)) // doONLYINTERESTED
	{
//		Log(FSS("\n[BOARD] GPIO digitalToggle Error.\n"));
//		return 0;
//	}
//	else
//	{
		return mb.Data.GpioData.ActionData.Value;
	}
}

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
	SendMessageToAllTask(&mb, doFORWARD, &XB_BOARD_DefTask);
}

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
	SendMessageToAllTask(&mb, doFORWARD, &XB_BOARD_DefTask);
}

TTask *TXB_board::AddTask(TTaskDef *Ataskdef, uint64_t ADeviceID)
{
	TTask *ta = TaskList;
	while (ta != NULL)
	{
		if (Ataskdef == ta->TaskDef)
		{
			String taskname = "";
			GetTaskName(Ataskdef, taskname);
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
		t->StreamTaskDef = Default_StreamTaskDef;
		t->SecStreamTaskDef = Default_SecStreamTaskDef;
		t->ShowLogInfo = Default_ShowLogInfo;
		t->ShowLogWarn = Default_ShowLogWarn;
		t->ShowLogError = Default_ShowLogError;
		if (ADeviceID != 0)
		{
			t->DeviceID.ID.ID64 = ADeviceID;
		}
		else
		{
			t->DeviceID.ID.ID64 = board.GetUniqueID().ID.ID64;
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

bool TXB_board::DelTask(TTaskDef *Ataskdef)
{
	if (Ataskdef != NULL)
	{
		if (Ataskdef->Task != NULL)
		{
			SendMessageToTask(Ataskdef, IM_DELTASK,true);
			DELETE_FROM_LIST_STR(TaskList, Ataskdef->Task);
			board.free(Ataskdef->Task);
			Ataskdef->Task = NULL;
			TaskCount--;
			return true;
		}
	}
	return false;
}

void TXB_board::ResetInAllTaskDefaultStream()
{
	TTask *t = TaskList;
	while (t != NULL)
	{
		t->StreamTaskDef = Default_StreamTaskDef;
		t->SecStreamTaskDef = Default_SecStreamTaskDef;
		t = t->Next;
	}
}

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

TTaskDef *TXB_board::GetTaskDefByName(String ATaskName)
{
	TTask *t = TaskList;
	String tmpstr;
	tmpstr.reserve(32);
	
	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (GetTaskName(t->TaskDef, tmpstr))
			{
				if (tmpstr == ATaskName) return t->TaskDef;
			}
		}
		t = t->Next;
	}
	return NULL;
}


void TXB_board::IterateTask(void)
{
	TTask *t = TaskList;
	iteratetask_procedure = true;
	{
		// Sprawdzenie czy uruchomi� przerwanie
		if(doAllInterruptRC>0)
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
		// Sprawdzenie czy w kt�rym� zadaniu min�� czas na uruchomienie Loop()
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

		// zapami�tanie ilo�ci wolnej pami�ci ram i minimalnego stanu
		DEF_WAITMS_VAR(GFH);
		BEGIN_WAITMS(GFH, 500);
		{
#ifdef ESP32

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

		// je�li bufor nadawczy jest zape�niony to nie uruchamiaj zada�
		//if (Serial_availableForWrite() < Serial_EmptyTXBufferSize) return;
		// ------------------------------------------------------------

		t = TaskList;
		// Uruchomienie zada� tzw realtime
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
		
		// Uruchomienie zada� z podzia�em na priorytety
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
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKSTATUS_STRING;
	return GetTaskString(&mb, ATaskDef, APointerString);
}

bool TXB_board::GetTaskName(TTaskDef *ATaskDef,String &APointerString)
{
	if (ATaskDef == NULL) return false;
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_GET_TASKNAME_STRING;
	return GetTaskString(&mb, ATaskDef, APointerString);
}

void TXB_board::SendMessageOTAUpdateStarted()
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_OTA_UPDATE_STARTED;
	mb.Data.uData64 = 0;
	SendMessageToAllTask(&mb, doBACKWARD);
}

void TXB_board::SendKeyFunctionPress(TKeyboardFunction Akeyfunction,char Akey)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToAllTask(&mb, doFORWARD); //doONLYINTERESTED
}

void TXB_board::SendKeyFunctionPress(TKeyboardFunction Akeyfunction, char Akey,TTaskDef *Ataskdef,bool Aexcludethistask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = Akeyfunction;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	if (Aexcludethistask)
	{
		SendMessageToAllTask(&mb, doFORWARD,Ataskdef); // doONLYINTERESTED
	}
	else
	{
		SendMessageToAllTask(&mb, doFORWARD); // doONLYINTERESTED
		if (Ataskdef->Task != NULL)
		{
			if (Ataskdef->Task->domessageRC > 0)
			{
				SendMessageToTask(Ataskdef, &mb, true);
			}
		}
	}
}

void TXB_board::SendKeyPress(char Akey)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode=Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToAllTask(&mb, doFORWARD); // doONLYINTERESTED
}

void TXB_board::SendKeyPress(char Akey, TTaskDef *Ataskdef)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = IM_KEYBOARD;
	mb.Data.KeyboardData.KeyCode = Akey;
	mb.Data.KeyboardData.KeyFunction = KF_CODE;
	mb.Data.KeyboardData.TypeKeyboardAction = tkaKEYPRESS;
	SendMessageToTask(Ataskdef,&mb, true);
}

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

bool TXB_board::SendMessageToTask(TTaskDef *ATaskDef, TMessageBoard *mb,bool Arunagain)
{
	return DoMessage(mb, Arunagain, CurrentTask, ATaskDef);
}

bool TXB_board::SendMessageToTask(TTaskDef *ATaskDef, TIDMessage AIDmessage, bool Arunagain)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = AIDmessage;
	mb.Data.uData32 = 0;
	return SendMessageToTask(ATaskDef, &mb, Arunagain);
}

bool TXB_board::SendMessageToTaskByName(String Ataskname, TMessageBoard *mb, bool Arunagain)
{
	TTask *t = TaskList;
	bool res = false;
	String tn;

	while (t != NULL)
	{
		if (t->TaskDef != NULL)
		{
			if (t->TaskDef->domessage)
			{
				tn = "";
				if (GetTaskName(t->TaskDef, tn))
				{
					if (Ataskname == tn)
					{
						return SendMessageToTask(t->TaskDef, mb, Arunagain);
					}
				}
			}
		}
		t = t->Next;
	}
	return false;
}

bool TXB_board::SendMessageToAllTask(TIDMessage AidMessage, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask)
{
	TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
	mb.IDMessage = AidMessage;
	mb.Data.uData32 = 0;
	return SendMessageToAllTask(&mb, ADoMessageDirection, Aexcludetask);
}

bool TXB_board::SendMessageToAllTask(TMessageBoard *mb, TDoMessageDirection ADoMessageDirection, TTaskDef *Aexcludetask)
{
	uint32_t res = 0;
	switch (ADoMessageDirection)
	{
/*		case doONLYINTERESTED:
		{
			uint8_t isinterested = 0;
			TTask *t = TaskList;
			while(t!=NULL)
			{
				if (t->TaskDef != NULL)
				{
					if (Aexcludetask != t->TaskDef)
					{
						if (t->LastIDMessage == mb->IDMessage)
						{
							isinterested++;
							res = DoMessage(mb, true, CurrentTask, t->TaskDef);
							if (!res)
							{
								t->LastIDMessage = IM_IDLE;
								isinterested--;
							}
						}
					}
				}
				t = t->Next;
			}
			if (isinterested == 0)
			{
				t = TaskList;
				while(t!=NULL)
				{
					if (t->TaskDef != NULL)
					{
						if (Aexcludetask != t->TaskDef)
						{
							res = DoMessage(mb, true, CurrentTask, t->TaskDef);
						}
					}
					t = t->Next;
				}
			}
			break;
		}*/
		case doFORWARD:
		{
			TTask *t = TaskList;
			while (t!=NULL)
			{
				if (t->TaskDef != NULL)
				{
					if (Aexcludetask != t->TaskDef)
					{
						res += DoMessage(mb, true, CurrentTask, t->TaskDef);
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
						res += DoMessage(mb, true, CurrentTask, t->TaskDef);
					}
				}
				t = t->Prev;
			}
			break;
		}
		default: break;
	}
	return (res>0);
}

void TXB_board::SendSaveConfigToAllTask(void)
{
	SendMessageToAllTask(IM_CONFIG_SAVE);
}

void TXB_board::setString(char *dst, const char *src, int max_size) 
{
	max_size--;
	if (src == NULL) {
		dst[0] = 0;
		return;
	}

	
	int size = StringLength((char *)src, 0);

	if (size + 1 > max_size)
		size = max_size - 1;
	xb_memorycopy((void *)src, dst, size);
	
	dst[size] = 0;
}

#if defined(ESP32)
uint32_t TXB_board::getFreePSRAM()
{
#ifdef BOARD_HAS_PSRAM
	
	uint32_t freepsram = ESP.getFreePsram();
	uint32_t r=0;
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
	return ESP.getFreeHeap();
#endif
}
#if !defined(_VMICRO_INTELLISENSE)
void *TXB_board::_malloc_psram(size_t Asize)
{
	size_t size = Asize;
	
	MaximumMallocPSRAM = (Asize > MaximumMallocPSRAM) ? Asize : MaximumMallocPSRAM;
	
	size = size >> 4;
	size = size + 1;
	size = size << 4;
#ifdef BOARD_HAS_PSRAM
	//void *ptr= heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT);
	void *ptr = malloc(size);
#else
	void *ptr = malloc(size);
#endif
	if (ptr != NULL)
	{
		if ((size - Asize) > 0)
		{
			xb_memoryfill(&((uint8_t *)ptr)[Asize] , size - Asize, 0xff);
		}
		xb_memoryfill(ptr, Asize, 0);
		OurReservedBlock++;
	}
	return ptr;

}
#endif

#if !defined(_VMICRO_INTELLISENSE)
void *TXB_board::_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr != NULL)
	{
		xb_memoryfill(ptr, size, 0);
		OurReservedBlock++;
	}
	return ptr;

}
#endif

#endif

#ifdef ESP8266
void *TXB_board::_malloc(size_t size)
{
	void *ptr = malloc(size);
	if (ptr != NULL)
	{
		xb_memoryfill(ptr, size, 0);
	}
	return ptr;
}
#endif

void ___free(void *Aptr)
{
	free(Aptr);
}

void TXB_board::free(void *Aptr)
{
	if (Aptr != NULL)
	{
		SendMessageToAllTask_FreePTR(Aptr);
		___free(Aptr);
		OurReservedBlock--;
	}
}

void TXB_board::freeandnull(void **Aptr)
{
	if (*Aptr != NULL)
	{
		board.free(*Aptr);
		*Aptr = NULL;
	}
}

void TXB_board::SendMessageToAllTask_FreePTR(void *Aptr)
{
	if (Aptr != NULL)
	{
		TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
		mb.IDMessage = IM_FREEPTR;
		mb.Data.FreePTR = Aptr;
		SendMessageToAllTask(&mb, doBACKWARD);
	}
}


uint32_t TXB_board::getFreeHeap()
{
#if defined(ESP8266) || defined(ESP32)
	return ESP.getFreeHeap();
#endif

#ifdef ARDUINO_ARCH_STM32F1
/*
	volatile int32_t size = 0; 
	volatile uint32_t ADR = 0;
	volatile uint8_t a = 1;

	noInterrupts();

	ADRESS_STACK = (uint32_t)&a;
	ADRESS_HEAP = (uint32_t)malloc(32);
	board.free((void *)ADRESS_HEAP);

	size = ADRESS_STACK - ADRESS_HEAP;
	interrupts();

	return size;
	*/
	return 20 * 1024;
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

#ifdef ARDUINO_ARCH_STM32F1
#define STM32_UUID ((uint32_t *) 0x1ffff7e8)

	volatile uint32_t idpart[3];
	idpart[0] = STM32_UUID[0];
	idpart[1] = STM32_UUID[1];
	idpart[2] = STM32_UUID[2];
	uint8_t *idpart8=(uint8_t *)idpart;

	V.ID.ID[0] = idpart8[0];
	V.ID.ID[1] = idpart8[1];
	V.ID.ID[2] = idpart8[2] + idpart8[8];
	V.ID.ID[3] = idpart8[3] + idpart8[9];
	V.ID.ID[4] = idpart8[4] + idpart8[10];
	V.ID.ID[5] = idpart8[5] + idpart8[11];
	V.ID.ID[6] = idpart8[6];
	V.ID.ID[7] = idpart8[7];
	
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

void TXB_board::SysTickCount_init(void)
{
#if defined(ESP8266) 
	SysTickCount_ticker.attach_ms(1, SysTickCount_proc);
	
#endif
	LastActiveTelnetClientTick = 0;
	
}

void TXB_board::DateTimeSecond_init(void)
{
#if defined(ESP8266)
	DateTimeSecond_ticker.attach(1, DateTimeSecond_proc);
#endif
}

void TXB_board::HandleKeyPress(char ch)
{
	if (TerminalFunction == 0)
	{
		SendKeyPress(ch);
	}
	else
	{
		SendKeyPress(ch, &XB_BOARD_DefTask);
	}
}

bool TXB_board::SendFrameToDeviceTask(String ADestTaskName, String AONStreamTaskName, void *ADataFrame, uint32_t Alength, uint32_t *AframeID, uint32_t ASourceAddress, uint32_t ADestAddress)
{
	TTaskDef *TaskDefStream = NULL;
	if (AONStreamTaskName != "local")
	{
		TaskDefStream = GetTaskDefByName(AONStreamTaskName);
		if (TaskDefStream == NULL) 
		{
			board.Log("Error: Stream task name not found...", true, true, tlError);
			return false;
		}
	}
	return SendFrameToDeviceTask(ADestTaskName, TaskDefStream, ADataFrame, Alength,AframeID,ASourceAddress,ADestAddress);
}

bool TXB_board::SendFrameToDeviceTask(String ADestTaskName, TTaskDef *ATaskDefStream, void *ADataFrame, uint32_t Alength, uint32_t *AframeID, uint32_t ASourceAddress,uint32_t ADestAddress)
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
	
	if (ATaskDefStream != NULL)
	{
		hdft->ACK.a = FRAME_ACK_A;
		hdft->ACK.b = FRAME_ACK_B;
		hdft->ACK.c = FRAME_ACK_C;
		hdft->ACK.d = FRAME_ACK_D;
	}
	
	*AframeID = SysTickCount;
	hdft->FT.SourceAddress = ASourceAddress;
	hdft->FT.SourceDeviceID = board.GetUniqueID();
	hdft->FT.DestAddress = ADestAddress;
	hdft->FT.DestDeviceID.ID.ID64 = 0;
	hdft->FT.FrameID = *AframeID;
	hdft->FT.FrameType = ftData;
	
	xb_memorycopy(ADataFrame, &hdft->FT.Frame, Alength);
	hdft->FT.LengthFrame = Alength;

	xb_memorycopy((void *)(ADestTaskName.c_str()), &hdft->FT.DestTaskName, ADestTaskName.length());
	
	hdft->FT.size = (((uint32_t)&hdft->FT.Frame) - ((uint32_t)&hdft->FT)) + hdft->FT.LengthFrame;
	uint32_t ltsize = sizeof(THDFT) - (sizeof(TFrameTransport) - hdft->FT.size);
	if (ATaskDefStream != NULL)
	{
		hdft->FT.crc8 = board.crc8((uint8_t *)&hdft->FT, hdft->FT.size);
		reslen=PutStream(hdft, ltsize, ATaskDefStream,ADestAddress);
		if (reslen != ltsize)
		{
			board.Log(String("Stream error - Send: " + String(ltsize) + " Sended:" + String(reslen) + " ...").c_str(), true, true, tlWarn);
			return false;
		}
	}
	else
	{
		HandleFrameLocal(&hdft->FT);
	}
	return true;
}

void TXB_board::SendResponseFrameOnProt(uint32_t AFrameID, TTaskDef *ATaskDefStream, uint32_t ASourceAddress, uint32_t ADestAddress, TFrameType AframeType, TUniqueID ADestDeviceID)
{
	static THDFT *hdft = NULL;
	if (hdft != NULL)
	{
		board.free(hdft);
		hdft = NULL;
	}
	hdft=(THDFT *)board._malloc(sizeof(THDFT));
	if (hdft == NULL)
	{
		board.Log("Memory error in send response frame...", true, true, tlError);
		return;
	}

	if (ATaskDefStream != NULL)
	{
		hdft->ACK.a = FRAME_ACK_A;
		hdft->ACK.b = FRAME_ACK_B;
		hdft->ACK.c = FRAME_ACK_C;
		hdft->ACK.d = FRAME_ACK_D;
	}

	hdft->FT.SourceAddress = ASourceAddress;
	hdft->FT.SourceDeviceID = board.GetUniqueID();
	hdft->FT.DestAddress = ADestAddress;
	hdft->FT.DestDeviceID = ADestDeviceID;
	hdft->FT.LengthFrame = 0;
	hdft->FT.FrameID = AFrameID;
	hdft->FT.FrameType = AframeType;
	
	hdft->FT.size = (((uint32_t)&hdft->FT.Frame) - ((uint32_t)&hdft->FT)) + hdft->FT.LengthFrame;
	uint32_t ltsize = sizeof(THDFT) - (sizeof(TFrameTransport) - hdft->FT.size);
	if (ATaskDefStream != NULL)
	{
		hdft->FT.crc8 = board.crc8((uint8_t *)&hdft->FT, hdft->FT.size);
		PutStream(hdft, ltsize, ATaskDefStream, ADestAddress);
	}
	else
	{
		HandleFrameLocal(&hdft->FT);
	}
	
	return;
}


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
				
				bool res = SendMessageToTask(DestTaskDefReceive, &mb);
				if (res)
				{
					SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, Aft->DestAddress, Aft->SourceAddress, ftResponseOK, Aft->SourceDeviceID);
					//SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, 0,Aft->FromChannelStream, ftResponseOK, Aft->DeviceID);
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
					//SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream,0, Aft->FromChannelStream, ft, Aft->DeviceID);
				}
			}
		}
		else
		{
			if (GetUniqueID().ID.ID64 == Aft->SourceDeviceID.ID.ID64)
			{
				TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
				mb.IDMessage = IM_FRAME_RESPONSE;
				mb.Data.FrameResponseData.FrameID = Aft->FrameID;
				mb.Data.FrameResponseData.FrameType = Aft->FrameType;
				SendMessageToAllTask(&mb, doFORWARD);
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
			//SendResponseFrameOnProt(Aft->FrameID, ATaskDefStream, 0, Aft->FromChannelStream, ftResponseCRCError, Aft->DeviceID);
		}
		else
		{
			board.Log("In response frame is CRC error...", true, true, tlError);
		}
	}
}

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
			SendMessageToAllTask(&mb, doFORWARD);
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
			

			bool res = SendMessageToTask(TaskDefReceive, &mb);

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
		if (GetUniqueID().ID.ID64 == Aft->SourceDeviceID.ID.ID64)
		{
			TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
			mb.IDMessage = IM_FRAME_RESPONSE;
			mb.Data.FrameResponseData.FrameID = Aft->FrameID;
			mb.Data.FrameResponseData.FrameType = Aft->FrameType;
			SendMessageToAllTask(&mb, doFORWARD);
		}
	}
	
}

void TXB_board::handle(void)
{
	DEF_WAITMS_VAR(LOOPW);
	BEGIN_WAITMS(LOOPW, 1000)
	{
#ifdef  BOARD_LED_LIFE_PIN
		digitalToggle(BOARD_LED_LIFE_PIN);
#endif
#ifdef ARDUINO_ARCH_STM32F1
		DateTimeUnix++;
#endif
#ifdef ESP32
		DateTimeUnix++;
#endif
#ifdef XB_GUI
		if (winHandle0 != NULL) winHandle0->RepaintDataCounter++;
#endif
		CheckOld_HandleDataFrameTransport();
	}
	END_WAITMS(LOOPW);

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

	{
		if (CurrentTask != NULL)
		{
			uint8_t bufkey[1];

			if (GetStream(bufkey, 1, CurrentTask->StreamTaskDef) > 0)
			{
				HandleKeyPress((char)bufkey[0]);
			}
			else if (CurrentTask->SecStreamTaskDef != NULL)
			{
				if (GetStream(bufkey, 1, CurrentTask->SecStreamTaskDef) > 0)
				{
					HandleKeyPress((char)bufkey[0]);
				}
			}
		}
	}
}

bool TXB_board::HandleDataFrameTransport(TMessageBoard *mb, THandleDataFrameTransport *AHandleDataFrameTransport,TTaskDef *ATaskDefStream)
{
	int32_t indx = 0;
	uint32_t indxstartinterpret = 0xffff;
	uint8_t v =0;
	bool isininterpret = false;
	
	while (indx < mb->Data.StreamData.LengthResult)
	{
		v = ((uint8_t *)mb->Data.StreamData.Data)[indx];

		// Sprawdzenie czy nadchodz�cy bajt jest pierwszym od ACK
		if ((AHandleDataFrameTransport->indx_interpret == 0) && (v == FRAME_ACK_A))
		{
			// Uruchominie interpretowania 
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			AHandleDataFrameTransport->isdataframe_interpret = true;

			isininterpret = true;
			indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz�cy bajt jest drugim od ACK
		else if ((AHandleDataFrameTransport->indx_interpret == 1) && (v == FRAME_ACK_B))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)
				if (indxstartinterpret==0xffff) 
					indxstartinterpret = indx;
		}
		// Sprawdzenie czy nadchodz�cy bajt jest trzecim od ACK
		else if ((AHandleDataFrameTransport->indx_interpret == 2) && (v == FRAME_ACK_C))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)					
				if (indxstartinterpret == 0xffff)
				indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz�cy bajt jest czwartym od ACK
		else if ((AHandleDataFrameTransport->indx_interpret == 3) && (v == FRAME_ACK_D))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)	
				if (indxstartinterpret == 0xffff)
					indxstartinterpret = indx; 
		}
		// Sprawdzenie czy nadchodz�cy bajt to 4 pozycja i czy ma warto�� oczekiwan� SIZE
		else if ((AHandleDataFrameTransport->indx_interpret == 4) && (v <= sizeof(TFrameTransport)) && (v >= offsetof(TFrameTransport, LengthFrame)))
		{
			AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			if (!isininterpret)				
				if (indxstartinterpret == 0xffff)
				indxstartinterpret = indx;
		}
		// Sprawdzenie czy kolejny
		else if (AHandleDataFrameTransport->indx_interpret > 4)
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
			if (isininterpret)
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
				if (AHandleDataFrameTransport->isdataframe_interpret)
				{
					//AHandleDataFrameTransport->Data.buf[AHandleDataFrameTransport->indx_interpret++] = v;
			
					uint32_t c = AHandleDataFrameTransport->indx_interpret-indx;

					// sprawdzenie czy zmie�ci si� do tego bufora streamu
					if ((mb->Data.StreamData.Length - mb->Data.StreamData.LengthResult) >= c)
					{
						int32_t indx_s = mb->Data.StreamData.LengthResult - 1;
						int32_t indx_d = mb->Data.StreamData.LengthResult - 1 + c;

						// Mo�na przesun�� to co zosta�o zapami�tane
						while (indx_s >= 0)
						{
							((uint8_t *)mb->Data.StreamData.Data)[indx_d] = ((uint8_t *)mb->Data.StreamData.Data)[indx_s];
							indx_d--;
							indx_s--;
						}

						mb->Data.StreamData.LengthResult += c;
						
						indx_s = 0;

						while (indx_s<AHandleDataFrameTransport->indx_interpret)
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
							GetTaskName(ATaskDefStream, tmps);
							Log(tmps.c_str(),false,false, tlError);
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
	if (indxstartinterpret != 0xffff)
	{
		mb->Data.StreamData.LengthResult = indxstartinterpret;
	}
	return true;
}

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
					xb_memoryfill(AHandleDataFrameTransport, sizeof(THandleDataFrameTransport),0);
					break;
				}
			}
			mb->Data.StreamData.LengthResult = indx;
			return true;
		}
	}
	return false;
}


THandleDataFrameTransport *TXB_board::AddToTask_HandleDataFrameTransport(TTaskDef *AStreamtaskdef,uint32_t Afromchannel)
{
	if (AStreamtaskdef == NULL) return NULL;
	if (AStreamtaskdef->Task == NULL) return NULL;
	
	THandleDataFrameTransport *hdft = AStreamtaskdef->Task->HandleDataFrameTransportList;
	while (hdft != NULL)
	{
		if (Afromchannel == hdft->FromChannel)
		{
			hdft->TickCreate = SysTickCount;
			return hdft;
		}
		hdft = hdft->Next;
	}
	hdft = (THandleDataFrameTransport *)board._malloc(sizeof(THandleDataFrameTransport));
	if (hdft == NULL) return NULL;
	ADD_TO_LIST_STR(AStreamtaskdef->Task->HandleDataFrameTransportList, THandleDataFrameTransport, hdft);
	hdft->FromChannel = Afromchannel;
	hdft->TickCreate = SysTickCount;
	
	return hdft;
}

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
				DELETE_FROM_LIST_STR(t->HandleDataFrameTransportList,hdft);
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

uint32_t TXB_board::GetStream(void *Adata, uint32_t Amaxlength, TTaskDef *AStreamtaskdef, uint32_t AChannel)
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
	mb.Data.StreamData.Channel = AChannel;
	
	
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
	
	if (SendMessageToTask(AStreamtaskdef, &mb, true))
	{
		hdft = AddToTask_HandleDataFrameTransport(AStreamtaskdef, mb.Data.StreamData.FromChannel);
		if (hdft != NULL) 
		{
			HandleDataFrameTransport(&mb, hdft, AStreamtaskdef);
		}
		return mb.Data.StreamData.LengthResult;
	}
	
	CheckOld_HandleDataFrameTransport(AStreamtaskdef->Task);
	
	return 0;
}

uint32_t TXB_board::PutStream(void *Adata, uint32_t Alength, TTaskDef *AStreamtaskdef, uint32_t AChannel)
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
	mb.Data.StreamData.Channel = AChannel;
	
	if (SendMessageToTask(AStreamtaskdef, &mb, true))
	{
		return mb.Data.StreamData.LengthResult;
	}
	return 0;
}

int TXB_board::print(String Atext)
{
	if (CurrentTask != NULL)
	{
		int len = Atext.length();	
		uint32_t rlen1 = PutStream((void *)Atext.c_str(), len, CurrentTask->StreamTaskDef);
		uint32_t rlen2 = PutStream((void *)Atext.c_str(), len, CurrentTask->SecStreamTaskDef);
		return rlen1 == 0 ? rlen2 : rlen1;
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
	PutStream(&Achr, 1, CurrentTask->StreamTaskDef);
	PutStream(&Achr, 1, CurrentTask->SecStreamTaskDef);
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
		PutStream((void *)txttime.c_str(), txttime.length(), CurrentTask->StreamTaskDef);
		PutStream((void *)txttime.c_str(), txttime.length(), CurrentTask->SecStreamTaskDef);
	}

	if (showtaskname)
	{
		if (NameTaskDef != NULL)
		{
			String taskname = "---";
			GetTaskName(NameTaskDef, taskname);
		
			if (taskname.length() > 0)
			{
				taskname.trim();
				taskname = '[' + taskname + "] ";
				PutStream((void *)taskname.c_str(), taskname.length(), CurrentTask->StreamTaskDef);
				PutStream((void *)taskname.c_str(), taskname.length(), CurrentTask->SecStreamTaskDef);
			}
		}

	}

	PutStream((void *)Atxt, len, CurrentTask->StreamTaskDef);
	PutStream((void *)Atxt, len, CurrentTask->SecStreamTaskDef);

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


typedef enum
{
	sfNormalChar,sfGetVarName,sfCancelGetVarName
} TStepFilter;
void TXB_board::FilterString(const char *Asourcestring, String &Adestinationstring)
{
	uint32_t lensource = StringLength(Asourcestring,0);
	uint32_t indx_s = 0;
	Adestinationstring = "";
	Adestinationstring.reserve(lensource * 2);
	char ch = 0;
	String varname = "";
	uint32_t indx_startvarname;
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
					indx_startvarname = indx_s;
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
				{ // Koniec nazwy zmiennej
					if(varname.length() > 0)					
					{
						TMessageBoard mb; xb_memoryfill(&mb, sizeof(TMessageBoard), 0);
						mb.IDMessage = IM_GET_VAR_VALUE;
						mb.fromTask = NULL;
						mb.Data.VarValueData.VarName = &varname;
						mb.Data.VarValueData.VarValue = &varvalue;
						result = SendMessageToAllTask(&mb);
						if (result)
						{	// Wstaw zawarto��
							Adestinationstring += varvalue;
							varname = "";
							varvalue = "";
							indx_s++;
							sf = sfNormalChar;
						}
						else
						{  // wstaw nazwe zmiennej bo niema w systemie takiej

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
				{ // Kolejny znak nazwy zmienne;
					varname += ch;
					indx_s++;
					
					if (indx_s >= lensource)
					{ // Anuluj nazwe zmiennej bo koniec �r�d�a
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
/*void TXB_board::PrintDiag(void)
{
#ifdef ESP8266
	String t;
	cbufSerial tmp(1024);
	
	tmp.print(FSS("\n\rPrint Diag:\n\r"));
	tmp.print(FSS("\n\r-----------"));
	
	t = ""; GetTimeIndx(t, DateTimeUnix - DateTimeStart);
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
*/


