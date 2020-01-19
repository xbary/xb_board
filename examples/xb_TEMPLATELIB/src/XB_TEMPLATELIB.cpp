#include <xb_board.h>
#include "XB_TEMPLATELIB.h"


//--------------------------------------------------------------------------------------------------------------------
// nazwane kroki funkcji Loop tego zadania
typedef enum {ggIdle} TTEMPLATELIB_LoopStep;
TTEMPLATELIB_LoopStep XB_TEMPLATELIB_LoopStep = ggIdle;

//--------------------------------------------------------------------------------------------------------------------
// Procedura uruchamiana podczas dodawania zadania 
// board.AddTask(&XB_TEMPLATELIB_DefTask);
void XB_TEMPLATELIB_Setup(void)
{
	board.Log("Init.",true,true);
	board.Log("..OK");
}
//-------------------------------------------------------------------------------------------------------------
// Procedura o najwy¿szym priorytecie , uruchamiana tyle razy ile razy zostanie wyzolone przerwanie dla tego zadania
// board.TriggerInterrupt(&XB_TEMPLATELIB_DefTask);
void XB_TEMPLATELIB_DoInterrupt(void)
{
	board.Log("Interrupt", true, true);
}
//-------------------------------------------------------------------------------------------------------------
// procedura g³ówna tego zadania
// <- jest to iloœæ milisekund po których ponownie zostanie uruchomiona procedura pêtli
uint32_t XB_TEMPLATELIB_DoLoop()
{
	switch (XB_TEMPLATELIB_LoopStep)
	{
	case ggIdle:
	{
		break;
	}
	default: break;
	}
	return 0;
}
//--------------------------------------------------------------------------------------------------------------------
// Procedura odbierania messagów przez zadanie
bool XB_TEMPLATELIB_DoMessage(TMessageBoard* Am)
{
	switch (Am->IDMessage)
	{
	case IM_GET_TASKNAME_STRING: GET_TASKNAME("TEMPLATELIB"); break;
	case IM_GET_TASKSTATUS_STRING:
	{
		switch (XB_TEMPLATELIB_LoopStep)
		{
			GET_TASKSTATUS(ggIdle, 2);
		}
		break;
	}
	default: return false;
	}
	return true;
}
//--------------------------------------------------------------------------------------------------------------------
TTaskDef XB_TEMPLATELIB_DefTask = { 2, &XB_TEMPLATELIB_Setup,&XB_TEMPLATELIB_DoLoop,&XB_TEMPLATELIB_DoMessage };