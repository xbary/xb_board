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
	IM_CONFIG_SAVE,
#ifdef XB_GUI
	IM_MENU,
	IM_INPUTDIALOG,
	IM_WINDOW,
#endif
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
typedef enum
{
	frrOK = 0,
	frrError,
	frrBufferIsFull,
	frrOKWaitNext,
	frrUnrecognizedType
} TFrameReceiveResult;
typedef struct {
	void *DataFrame;
	uint32_t SizeFrame;
	TTaskDef *TaskDefStream;
	TFrameReceiveResult FrameReceiveResult;
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
	tivNoDef =0,
	tivString,
	tivDynArrayChar1, 
	// Wszystkie znaki
   tivDynArrayChar2, 
	// Bez spacji
   tivDynArrayChar3, 
	// same litery i cyfry
   tivInt8,
	tivIP,
	tivInt16,
	tivInt32,
	tivInt64,
	tivUInt8,
	tivUInt16,
	tivUInt32,
	tivUInt64,

} TTypeInputVar;

#ifdef XB_GUI

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
	tmaOPEN_MAINMENU, 
	tmaCLOSE_MAINMENU, 
	tmaGET_INIT_MENU, 
	tmaGET_CAPTION_MENU_STRING, 
	tmaGET_ITEM_MENU_STRING, 
	tmaCLICK_ITEM_MENU,
	tmaESCAPE_MENU,
	tmaDEL_ITEM_MENU
} TTypeMenuAction;

typedef struct
{
	uint8_t ItemCount;
	uint8_t Width;
	uint8_t CurrentSelect;
	Tx X;
	Ty Y;
	bool EscapeClose;
} TMenuInitData;

typedef struct
{
	String *PointerString;
} TMenuCaptionData;

typedef enum { taLeft, taCentre, taRight } TTextAlignment;

typedef struct
{
	String *PointerString;
	uint8_t ItemIndex;
	TTextAlignment TextAlignment;
} TMenuItemData;

typedef struct
{
	uint8_t ItemIndex;
	bool Close;
} TMenuClickData;

typedef struct
{
	uint8_t ItemIndex;
	bool ReInit;
} TMenuDelData;
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
		TMenuDelData MenuDelData;
	} ActionData;
} TMenuData;
//-----------------------------------------------------------------------

typedef enum {
	ida_INIT_INPUTDIALOG,
	ida_GET_CAPTION_STRING,
	ida_GET_DESCRIPTION_STRING,
	ida_ENTER_DIALOG,
	ida_ESCAPE_DIALOG,
} TTypeInputDialogAction;

typedef struct
{
	String *PointerString;
} TInputDialogCaptionData;

typedef struct
{
	String *PointerString;
} TInputDialogDescriptionData;

typedef struct
{
	uint32_t Min;
	uint32_t Max;
} Tuint32MinMax;

typedef struct
{
	TTypeInputVar TypeInputVar;
	uint8_t MaxLength;
	void *DataPointer;
	union 
	{
		Tuint32MinMax uint32MinMax;
	} MinMax;
	
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
#endif
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
#ifdef XB_GUI
		TMenuData MenuData;
		TInputDialogData InputDialogData;
		TWindowData WindowData;
#endif
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
