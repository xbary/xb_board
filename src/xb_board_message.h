#ifndef XB_BOARD_MESSAGES_H
#define XB_BOARD_MESSAGES_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif


#define ADD_TO_LIST(Alist,AClass) {\
AClass *lm = Alist;\
if (lm == NULL)\
{\
	Alist = this;\
	Next = NULL;\
	Prev = NULL;\
}\
else\
{\
	while (lm != NULL) { \
		if (lm->Next == NULL) \
		{ \
			lm->Next = this; \
			Prev = lm; \
			Next = NULL; \
			break; \
		} \
		else \
		{ \
			lm = lm->Next; \
		} \
	} \
} \
}

#define DELETE_FROM_LIST(Alist) \
{ \
if (Prev == NULL) \
{ \
	Alist = Next; \
	if (Next != NULL) \
	{ \
		Next->Prev = NULL; \
	} \
} \
else \
{ \
	Prev->Next = Next; \
	if (Next != NULL) \
	{ \
		Next->Prev = Prev; \
	} \
} \
}

typedef enum { doFORWARD, doBACKWARD, doONLYINTERESTED } TDoMessageDirection;

typedef enum {
	IM_IDLE = 0,
	IM_GPIO,
	IM_WIFI_DISCONNECT,
	IM_WIFI_CONNECT,
	IM_INTERNET_DISCONNECT,
	IM_INTERNET_CONNECT,
	IM_GET_TASKNAME_STRING,
	IM_GET_TASKSTATUS_STRING,
	IM_KEYBOARD,
	IM_MENU,
	IM_INPUTDIALOG,
	IM_CONFIG,
	IM_WINDOW

} TIDMessage;

//-----------------------------------------------------------------------
typedef enum {
	gaPinMode, gaPinWrite, gaPinRead
} TGpioAction;

typedef struct {
	TGpioAction GpioAction;
	uint16_t NumPin;
	union {
		uint8_t Value;
		uint8_t Mode;
	}
	ActionData;
	
} TGpioData;

//-----------------------------------------------------------------------
typedef uint8_t Tx;
typedef uint8_t Ty;

typedef enum {
	waRepaint,waRepaintData,waCreate,waDestroy,waShow,waHide,waGetCaptionWindow
} TWindowAction;

typedef struct {
	Tx X;
	Ty Y;
	Tx Width;
	Ty Height;
} TWindowDataCreate;

typedef struct {
	uint8_t unused;
	String *PointerString;
} TWindowDataGetCaption;

typedef struct {
	TWindowAction WindowAction;
	int8_t ID;
	union
	{
		TWindowDataCreate Create;
		TWindowDataGetCaption GetCaption;
	} ActionData;


} TWindowData;

//-----------------------------------------------------------------------
typedef enum {
	tcaWRITE_VAR, tcaREAD_VAR, tcaREADED_VAR
} TTypeConfigAction;

typedef enum { tcvNULL = 255, tcvArrayChar = 0, tcvString = 1, tcvInt32 = 2, tcvStringIP = 3 } TTypeConfigVar;

typedef struct
{
	uint8_t IDTask:4;
	uint8_t TypeVar:2;
	uint16_t IDVar:10;
} TIDConfig;

typedef struct
{
	void *PointerData;
	uint8_t Size;
	TIDConfig ID;
	TTypeConfigAction TypeConfigAction;
} TConfigData;
//-----------------------------------------------------------------------

#define DEF_MENUITEMNAME(iditem,nameitemstr) case iditem: *(Am->Data.MenuData.ActionData.MenuItemData.PointerString) = (nameitemstr); break;

typedef enum {
	tmaGET_INIT_MENU, tmaGET_CAPTION_MENU_STRING, tmaGET_ITEM_MENU_STRING, tmaCLICK_ITEM_MENU
} TTypeMenuAction;

typedef struct
{
	uint8_t ItemCount;
	uint8_t Width;
	uint8_t CurrentSelect;
} TMenuInitData;

typedef struct
{
	String *PointerString;
} TMenuCaptionData;

typedef struct
{
	String *PointerString;
	uint8_t ItemIndex;
} TMenuItemData;

typedef struct
{
	uint8_t ItemIndex;
} TMenuClickData;

typedef struct
{
	TTypeMenuAction TypeMenuAction;
	int8_t IDMenu;
	union
	{
		TMenuInitData MenuInitData;
		TMenuItemData MenuItemData;
		TMenuClickData MenuClickData;
		TMenuCaptionData MenuCaptionData;
	} ActionData;
} TMenuData;
//-----------------------------------------------------------------------
typedef enum {
	ida_INIT_INPUTDIALOG,
	ida_GET_CAPTION_STRING,
	ida_GET_DESCRIPTION_STRING
} TTypeInputDialogAction;

typedef struct
{
	String *PointerString;
} TInputDialogCaptionData;

typedef struct
{
	String *PointerString;
} TInputDialogDescriptionData;



typedef enum {
	tivString,
	tivDynArrayChar1, // Wszystkie znaki
	tivDynArrayChar2, // Bez spacji
	tivDynArrayChar3, // same litery i cyfry
	tivInt8,
	tivInt16,
	tivInt32,
	tivInt64,
	tivUInt8,
	tivUInt16,
	tivUInt32,
	tivUInt64,

} TTypeInputVar;
typedef struct
{
	TTypeInputVar TypeInputVar;
	uint8_t MaxLength;
	void *DataPointer;
	
} TInputDialogInitData;

typedef struct
{
	TTypeInputDialogAction TypeInputDialogAction;
	int8_t IDInputDialog;
	union
	{
		TInputDialogCaptionData InputDialogCaptionData;
		TInputDialogInitData InputDialogInitData;
		TInputDialogDescriptionData InputDialogDescriptionData;
	} ActionData;
} TInputDialogData;

//-----------------------------------------------------------------------

typedef enum {
	KF_CODE,
	KF_NONE,
	KF_MENU,
	KF_ESC,
	KF_ENTER,
	KF_BACKSPACE,
	KF_DELETE,
	KF_TABNEXT,
	KF_TABPREV,
	KF_CURSORUP,
	KF_CURSORDOWN,
	KF_CURSORLEFT,
	KF_CURSORRIGHT,
	KF_F1,
	KF_F2,
	KF_F3,
	KF_F4,
	KF_F5,
	KF_F6,
	KF_F7,
	KF_F8,
	KF_F9,
	KF_F10,
	KF_F11,
	KF_F12
} TKeyboardFunction;
typedef enum { tkaKEYDOWN, tkaKEYPRESS, tkaKEYUP } TTypeKeyboardAction;

typedef struct
{
	TTypeKeyboardAction TypeKeyboardAction;
	uint8_t KeyCode;
	TKeyboardFunction KeyFunction;
} TKeyboardData;

//-----------------------------------------------------------------------

typedef struct
{
	TIDMessage IDMessage;
	union
	{
		TConfigData ConfigData;
		TKeyboardData KeyboardData;
		TMenuData MenuData;
		TInputDialogData InputDialogData;
		TWindowData WindowData;
		TGpioData GpioData;
		void *PointerData;
		String *PointerString;
		uint64_t uData64;
		uint32_t uData32;
		uint16_t uData16;
		uint8_t uData8;
	} Data;
} TMessageBoard;

#endif
