#ifndef XB_BOARD_MESSAGES_H
#define XB_BOARD_MESSAGES_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

// Makra zwi¹zane z listami struktur, u¿ywane wewn¹trz klas przewa¿nie w kontruktorach i destruktorach
#define ADD_TO_LIST(Alist,AClass) \
{ \
AClass *lm = Alist;\
Next = NULL;\
Prev = NULL;\
if (lm == NULL)\
{\
	Alist = this;\
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

// Makra zwi¹zane z listami struktur, u¿ywane na zewn¹trz klas z mo¿liwoœci¹ wskazania 
#define ADD_TO_LIST_STR(Alist,AClass,_this_) \
{ \
AClass *lm = Alist;\
_this_->Next = NULL;\
_this_->Prev = NULL;\
if (lm == NULL)\
{\
	Alist = _this_;\
}\
else\
{\
	while (lm != NULL) { \
		if (lm->Next == NULL) \
		{ \
			lm->Next = _this_; \
			_this_->Prev = lm; \
			_this_->Next = NULL; \
			break; \
		} \
		else \
		{ \
			lm = lm->Next; \
		} \
	} \
} \
}
#define DELETE_FROM_LIST_STR(Alist,_this_) \
{ \
if (_this_->Prev == NULL) \
{ \
	Alist = _this_->Next; \
	if (_this_->Next != NULL) \
	{ \
		_this_->Next->Prev = NULL; \
	} \
} \
else \
{ \
	_this_->Prev->Next = _this_->Next; \
	if (_this_->Next != NULL) \
	{ \
		_this_->Next->Prev = _this_->Prev; \
	} \
} \
}

struct TTask;
struct TTaskDef;

//doONLYINTERESTED

typedef enum { doFORWARD, doBACKWARD } TDoMessageDirection;

typedef enum {
	IM_IDLE = 0,
	IM_GPIO,
	IM_DELTASK,
	IM_FREEPTR,
	IM_RX_BLINK,
	IM_TX_BLINK,
	IM_WIFI_DISCONNECT,
	IM_WIFI_CONNECT,
	IM_INTERNET_DISCONNECT,
	IM_INTERNET_CONNECT,
	IM_OTA_UPDATE_STARTED,
	IM_GET_TASKNAME_STRING,
	IM_GET_TASKSTATUS_STRING,
	IM_FRAME_RECEIVE,
	IM_FRAME_RESPONSE,
	IM_STREAM,
	IM_KEYBOARD,
	IM_MENU,
	IM_INPUTDIALOG,
	IM_CONFIG_SAVE,
	IM_WINDOW,
	IM_SD_INIT,
	IM_SD_DEINIT,
	IM_SD_EJECT,

} TIDMessage;

//-----------------------------------------------------------------------
typedef enum {
	gaPinMode, gaPinWrite, gaPinRead , gaPinToggle
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
typedef enum {
	saGet, saPut
} TStreamAction;

typedef struct {
	TStreamAction StreamAction;
	void *Data;
	uint32_t Length;
	uint32_t LengthResult;

} TStreamData;

//-----------------------------------------------------------------------
typedef struct {
	int8_t UserID;
} TBlinkData;
//-----------------------------------------------------------------------
typedef struct {
	void *DataFrame;
	uint32_t SizeFrame;
	TTaskDef *TaskDefStream;
} TFrameReceiveData;
//-----------------------------------------------------------------------
typedef struct {
	uint32_t FrameID;
	TFrameType FrameType;
} TFrameResponseData;
//-----------------------------------------------------------------------
typedef int16_t Tx;
typedef int16_t Ty;

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
#define BEGIN_MENUITEMNAME(idmenu) if (Am->Data.MenuData.IDMenu==idmenu) \
{ \
uint8_t MenuItemIndex = Am->Data.MenuData.ActionData.MenuItemData.ItemIndex; 


#define DEF_MENUITEMNAME(iditem, nameitemstr) if (MenuItemIndex == iditem) { *(Am->Data.MenuData.ActionData.MenuItemData.PointerString) = String(nameitemstr); }
#define DEF_MENUITEMNAME_CHECKED(iditem,nameitemstr,boolvalue) if (MenuItemIndex == iditem) { *(Am->Data.MenuData.ActionData.MenuItemData.PointerString) = "[" + String(boolvalue == true ? "*" : " ") + "] "+ String(nameitemstr); }

#define END_MENUITEMNAME() \
}


#define DEF_MENUCAPTION(idmenu,caption) {if (Am->Data.MenuData.IDMenu==idmenu) {*(Am->Data.MenuData.ActionData.MenuCaptionData.PointerString) = caption;}}





#define BEGIN_MENUINIT(idmenu) if (Am->Data.MenuData.IDMenu==idmenu) \
{ 

#define DEF_MENUINIT(itemcount,currentselect,width) \
{ \
Am->Data.MenuData.ActionData.MenuInitData.ItemCount = itemcount;  \
Am->Data.MenuData.ActionData.MenuInitData.CurrentSelect = currentselect; \
Am->Data.MenuData.ActionData.MenuInitData.Width = width; \
}


#define END_MENUINIT() \
}

#define BEGIN_MENUCLICK(idmenu) if (Am->Data.MenuData.IDMenu==idmenu) \
{ \
uint8_t MenuItemIndex = Am->Data.MenuData.ActionData.MenuClickData.ItemIndex; 

#define EVENT_MENUCLICK(itemindex) if (MenuItemIndex == itemindex) 

#define END_MENUCLICK() \
}


typedef enum {
	tmaOPEN_MAINMENU, tmaCLOSE_MAINMENU, tmaGET_INIT_MENU, tmaGET_CAPTION_MENU_STRING, tmaGET_ITEM_MENU_STRING, tmaCLICK_ITEM_MENU
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

#define BEGIN_INPUTDIALOGINIT(iddialog) if (Am->Data.InputDialogData.IDInputDialog==iddialog) \
{ 

#define DEF_INPUTDIALOGINIT(ATypeInputVar,AMaxLength,ADataPointer) \
{ \
Am->Data.InputDialogData.ActionData.InputDialogInitData.TypeInputVar = ATypeInputVar; \
Am->Data.InputDialogData.ActionData.InputDialogInitData.MaxLength = AMaxLength; \
Am->Data.InputDialogData.ActionData.InputDialogInitData.DataPointer = ADataPointer; \
}

#define END_INPUTDIALOGINIT() \
}

#define DEF_INPUTDIALOGCAPTION(idinputdialog,caption) {if (Am->Data.InputDialogData.IDInputDialog==idinputdialog) {*(Am->Data.InputDialogData.ActionData.InputDialogCaptionData.PointerString) = caption;}}

#define DEF_INPUTDIALOGDESCRIPTION(idinputdialog,description) {if (Am->Data.InputDialogData.IDInputDialog==idinputdialog) {*(Am->Data.InputDialogData.ActionData.InputDialogDescriptionData.PointerString) = description;}}


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
struct TMessageBoard
{
	TIDMessage IDMessage;
	TTask *fromTask;
	union
	{
		//TConfigData ConfigData;
		TKeyboardData KeyboardData;
		TMenuData MenuData;
		TInputDialogData InputDialogData;
		TWindowData WindowData;
		TGpioData GpioData;
		TStreamData StreamData;
		TBlinkData BlinkData;
		TFrameReceiveData FrameReceiveData;
		TFrameResponseData FrameResponseData;
		void *PointerData;
		void *FreePTR;
		String *PointerString;
		uint64_t uData64;
		uint32_t uData32;
		uint16_t uData16;
		uint8_t uData8;
	} Data;
};

#endif
