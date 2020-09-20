#include "DEV_SOLAR.h"
#include <xb_board.h>
#include <xb_GUI.h>
#include <xb_GUI_Gadget.h>

// Global VAR
TDS_FunctionStep DS_FunctionStep;
#ifdef XB_GUI
TGADGETMenu* DS_MainMenuHandle;
#endif

// Konfiguracja
bool DS_CFG_Start = false;

bool DS_LoadConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("DEV_SOLAR"))
	{
		DS_CFG_Start = board.PREFERENCES_GetBool("START", DS_CFG_Start);
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false
#endif
}

bool DS_SaveConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("DEV_SOLAR"))
	{
		board.PREFERENCES_PutBool("START", DS_CFG_Start);
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false
#endif
}

bool DS_ResetConfig()
{
#ifdef XB_PREFERENCES
	if (board.PREFERENCES_BeginSection("DEV_SOLAR"))
	{
		board.PREFERENCES_CLEAR();
		DS_CFG_Start = false;
		board.PREFERENCES_EndSection();
	}
	else
	{
		return false;
	}
	return true;
#else
	return false
#endif
}

// --------------------------------------------------------------------------------------------
void DS_Setup()
{
	board.Log("Init...", true, true);
	DS_LoadConfig();
	DS_FunctionStep = DSFS_IDLE;
	board.Log("OK");
}

uint32_t DS_DoLoop()
{
	switch (DS_FunctionStep)
	{
	case DSFS_IDLE: //------------------------------------------------------------------------
	{
		if (DS_CFG_Start)
		{
			DS_FunctionStep = DSFS_Start;
			return 1000;
		}
		break;
	}
	case DSFS_Start: //------------------------------------------------------------------------
	{
		if (!DS_CFG_Start)
		{
			DS_FunctionStep = DSFS_IDLE;
			return 0;
		}
		board.Log("Start.", true, true);


		DS_FunctionStep = DSFS_Engine;
		board.Log("..OK");
		break;
	}
	case DSFS_Stop: //------------------------------------------------------------------------
	{
		if (DS_CFG_Start)
		{
			DS_FunctionStep = DSFS_Engine;
			return 0;
		}
		board.Log("Stop.", true, true);


		DS_FunctionStep = DSFS_IDLE;
		board.Log("..OK");
		break;
	}
	case DSFS_Engine: //------------------------------------------------------------------------
	{
		if (!DS_CFG_Start)
		{
			DS_FunctionStep = DSFS_Stop;
			return 0;
		}

		// Algorytm

		break;
	}
	}
	return 0;
}

bool DS_DoMessage(TMessageBoard* Am)
{
	switch (Am->IDMessage)
	{
	case IM_GET_TASKNAME_STRING:
	{
		GET_TASKNAME("DEV_SOLAR");
		return true;
	}
	case IM_GET_TASKSTATUS_STRING:
	{
		switch (DS_FunctionStep)
		{
			GET_TASKSTATUS(DSFS_IDLE, 5);
			GET_TASKSTATUS(DSFS_Start, 5);
			GET_TASKSTATUS(DSFS_Stop, 5);
			GET_TASKSTATUS(DSFS_Engine, 5);
		}
		return true;
	}
	case IM_FREEPTR:
	{
#ifdef XB_GUI
		FREEPTR(DS_MainMenuHandle);
#endif
		return true;
	}
	case IM_LOAD_CONFIGURATION:
	{
		if (!DS_LoadConfig())
		{
			board.Log("Error load configuration.",true,true,tlError);
		}
		return true;
	}
	case IM_SAVE_CONFIGURATION:
	{
		if (!DS_SaveConfig())
		{
			board.Log("Error save configuration.", true, true, tlError);
		}
		return true;
	}
	case IM_RESET_CONFIGURATION:
	{
		if (!DS_ResetConfig())
		{
			board.Log("Error reset configuration.", true, true, tlError);
		}
		return true;
	}

#ifdef XB_GUI
	case IM_WINDOW:
	{
		break;
	}
	case IM_MENU:
	{
		OPEN_MAINMENU()
		{
			DS_MainMenuHandle = GUIGADGET_CreateMenu(&DS_DefTask, 0, false, X, Y);
		}

		BEGIN_MENU(0, "Main menu", WINDOW_POS_X_DEF, WINDOW_POS_Y_DEF, 64, MENU_AUTOCOUNT, 0, true)
		{
			BEGIN_MENUITEM_CHECKED("Start engine", taLeft, DS_CFG_Start)
			{
				CLICK_MENUITEM()
				{
					DS_CFG_Start = !DS_CFG_Start;
				}
			}
			END_MENUITEM()
			SEPARATOR_MENUITEM()
			CONFIGURATION_MENUITEMS()
		}
		END_MENU()

		return true;
	}
#endif
	}
	return false;
}

TTaskDef DS_DefTask = { 0,&DS_Setup,&DS_DoLoop,&DS_DoMessage };
