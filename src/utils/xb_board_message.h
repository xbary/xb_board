#ifndef XB_BOARD_MESSAGES_H
#define XB_BOARD_MESSAGES_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#endif

// Makro definiuj¹ce zmienne listy struktur klas
#define DEFLIST_VAR(AClass,Alist) \
AClass *Alist=NULL; \
AClass *Alist##_last=NULL;\
uint32_t Alist##_count=0;


// Makra zwi¹zane z listami struktur, u¿ywane wewn¹trz klas przewa¿nie w kontruktorach i destruktorach
#define ADD_TO_LIST(Alist,AClass) \
{ \
this->Next = NULL;\
this->Prev = NULL;\
if (Alist == NULL)\
{\
	Alist = this; \
	Alist##_last= this; \
	Alist##_count = 1; \
} \
else \
{ \
	Alist##_last->Next = this; \
	this->Prev = Alist##_last; \
	Alist##_last = this; \
	Alist##_count++; \
} \
}

#define DELETE_FROM_LIST(Alist) \
{ \
	Alist##_count--; \
	if (this == Alist##_last) \
	{ \
		Alist##_last = this->Prev; \
	} \
	if (this == Alist) \
	{ \
		Alist = this->Next; \
		if (this->Next != NULL) \
		{ \
			this->Next->Prev = NULL; \
		} \
	} \
	else \
	{ \
		if (this->Prev!=NULL) \
		{ \
			this->Prev->Next = this->Next; \
			if (this->Next != NULL) \
			{ \
				this->Next->Prev = this->Prev; \
			} \
		} \
	} \
}

// Makra zwi¹zane z listami struktur, u¿ywane na zewn¹trz klas z mo¿liwoœci¹ wskazania 
#define ADD_TO_LIST_STR(Alist,AClass,_this_) \
{ \
_this_->Next = NULL;\
_this_->Prev = NULL;\
if (Alist == NULL)\
{\
	Alist = _this_; \
	Alist##_last= _this_; \
	Alist##_count = 1; \
} \
else \
{ \
	Alist##_last->Next = _this_; \
	_this_->Prev = Alist##_last; \
	Alist##_last = _this_; \
	Alist##_count++; \
} \
}

#define INSERT_TO_LIST_STR(Alist,AClass,_tothis_,_this_) \
{ \
	_this_->Next = NULL;\
	_this_->Prev = NULL;\
	if (_tothis_->Prev == NULL)\
	{\
		_tothis_->Prev=_this_; \
		Alist = _this_; \
		_this_->Next=_tothis_; \
		Alist##_count++; \
	} \
	else \
	{ \
		_this_->Prev=_tothis_->Prev; \
		_this_->Next=_tothis_; \
		_tothis_->Prev->Next=_this_; \
		_tothis_->Prev = _this_; \
		Alist##_count++; \
	} \
}


#define DELETE_FROM_LIST_STR(Alist,_this_) \
{ \
	Alist##_count--; \
	if (_this_ == Alist##_last) \
	{ \
		Alist##_last = _this_->Prev; \
	} \
	if (_this_ == Alist) \
	{ \
		Alist = _this_->Next; \
		if (_this_->Next != NULL) \
		{ \
			_this_->Next->Prev = NULL; \
		} \
	} \
	else \
	{ \
		if (_this_->Prev!=NULL) \
		{ \
			_this_->Prev->Next = _this_->Next; \
			if (_this_->Next != NULL) \
			{ \
				_this_->Next->Prev = _this_->Prev; \
			} \
		} \
	} \
}

//#define MOVE_STR_TO(Alist,_this_,_tothis_) \
//{ \
//		if (_this_->Next!=NULL) \
//		{ \
//			_this_->Next->Prev = _this_->Next; \
//			if (_this_->Prev!=NULL) \
//			{ \
//				_this_->Prev->Next = _this_->Next; \
//				_this_->Next->Prev = _this_->Prev; \
//			} \
//			else \
//			{ \
//				Alist = _this_->Next; \
//				_this_->Next->Prev=NULL; \
//			} \
//		} \
//		else \
//		{ \
//			if (_this_->Prev!=NULL) \
//			{ \
//				_this_->Prev->Next=NULL; \
//				Alist##_last = _this_->Prev; \
//			} \
//			else \
//			{ \
//				Alist = NULL; \
//				Alist##_last = NULL; \
//			} \
//		} \
//		_this_->Next = NULL; \
//		_this_->Prev = NULL; \
//		\
//		if (_tothis_->Prev != NULL) \
//		{ \
//			_this_->Next = _tothis_; \
//			_tothis_->Prev =_this_; \
//			_this_->Prev = _tothis_->Prev; \
//			_tothis_->Prev->Next=_this_; \
//		} \
//		else \
//		{ \
//			_this_->Next= _tothis_; \
//			_tothis_->Prev = _this_; \
//			Alist = _this_; \
//		} \
//}

#define MOVE_STR_TO_ENDLIST(Alist,AClass,_this_) \
{ \
if (Alist!=NULL) \
{ \
if (_this_->Next!=NULL) \
{ \
DELETE_FROM_LIST_STR(Alist,_this_); \
ADD_TO_LIST_STR(Alist,AClass,_this_); \
} \
} \
}

#define MOVE_STR_TO_BEGINLIST(Alist,AClass,_this_) \
{ \
if (Alist!=NULL) \
{ \
if (_this_->Prev!=NULL) \
{ \
DELETE_FROM_LIST_STR(Alist,_this_); \
AClass *l=Alist; \
if (l==NULL) \
{ \
	Alist=_this_;\
} \
else \
{ \
	Alist=_this_; \
	_this_->Next=l; \
	l->Prev=_this_; \
} \
} \
} \
}

#define CREATE_STR_ADD_TO_LIST(Alist,Astr,Aptr) \
{ \
	Aptr=(Astr *)board._malloc_psram(sizeof(Astr)); \
	if (Aptr!=NULL) \
	{ \
		ADD_TO_LIST_STR(Alist,Astr,Aptr); \
	} \
}

#define DESTROY_STR_DEL_FROM_LIST(Alist,Aptr) \
{ \
	if (Aptr!=NULL) \
	{ \
		DELETE_FROM_LIST_STR(Alist,Aptr); \
		board.freeandnull((void **)&Aptr); \
	} \
} 

#define CREATE_CLASS_ADD_TO_LIST(Alist,Aclass,Aptr) \
{ \
	Aptr= new Aclass; \
	if (Aptr!=NULL) \
	{ \
		ADD_TO_LIST_STR(Alist,Astr,Aptr); \
	} \
}

#define DESTROY_CLASS_DEL_FROM_LIST(Alist,Aptr) \
{ \
	if (Aptr!=NULL) \
	{ \
		delete(Aptr); \
		DELETE_FROM_LIST_STR(Alist,Aptr); \
		Aptr=NULL; \
	} \
} 

#define BEGIN_FOREACH_LIST(item,list) \
{ \
item = list; \
while(item!=NULL) \
{

#define END_FOREACH_LIST(item) \
item=item->Next; \
} \
} 

#define BEGIN_FOREACH_BACK_LIST(item,list) \
{ \
item = list##_last; \
while(item!=NULL) \
{

#define END_FOREACH_BACK_LIST(item) \
item=item->Prev; \
} \
} 


struct TTask;
struct TTaskDef;
struct TGPIODrive;
struct TSocket;

#define GET_TASKSTATUS(enumstatus,cutbeginch) case enumstatus: {*(Am->Data.PointerString) = String(#enumstatus); Am->Data.PointerString->remove(0,cutbeginch); break;}
#define GET_TASKSTATUS_OTHER(enumstatus,cutbeginch,other) case enumstatus: {*(Am->Data.PointerString) = String(#enumstatus)+other; Am->Data.PointerString->remove(0,cutbeginch); break;}
#define GET_TASKSTATUS_ADDSTR(Astr) {*(Am->Data.PointerString) += String(Astr); }
#define GET_TASKNAME(name) *(Am->Data.PointerString) = (name);
#define GET_ENUMSTRING(enumstatus,cutbeginch) case enumstatus: {String s=#enumstatus; s.remove(0,cutbeginch); s.replace('_',' '); return s;}
#define GET_DEFAULTSTRING(str) default: { return str;}

typedef enum { doFORWARD, doBACKWARD } TDoMessageDirection;

typedef enum {
	IM_IDLE = 0,
	IM_GPIO,
	IM_DELTASK,
	IM_HANDLEPTR,
	IM_RX_BLINK,
	IM_TX_BLINK,
	IM_LIVE_BLINK,
	IM_NET_DISCONNECT,
	IM_NET_CONNECT,
	IM_INTERNET_DISCONNECT,
	IM_INTERNET_CONNECT,
	IM_OTA_UPDATE_STARTED,
	IM_GET_TASKNAME_STRING,
	IM_GET_TASKSTATUS_STRING,
	IM_FRAME_RECEIVE,
	IM_FRAME_RESPONSE,
	IM_STREAM,
	IM_KEYBOARD,
	IM_LOAD_CONFIGURATION,
	IM_SAVE_CONFIGURATION,
	IM_RESET_CONFIGURATION,
	IM_VAR,
#ifdef XB_GUI
	IM_MENU,
	IM_INPUTDIALOG,
	IM_WINDOW,
#endif
	IM_SD_INIT,
	IM_SD_DEINIT,
	IM_SD_EJECT,
	IM_SENSOR,
	IM_BEFORE_RESET,
	IM_RTCSYNC,
	IM_SOCKET
} TIDMessage;


//-----------------------------------------------------------------------
typedef enum {
	saSetValue,saGetValue
} TSensorAction;
class TSensorClass;
typedef struct {
	TSensorAction SensorAction;
	TSensorClass* Sensor;
	void* ValueData;

} TSensorData;
//-----------------------------------------------------------------------
typedef enum {
	gaPinMode, gaPinWrite, gaPinRead , gaPinToggle,
	gaPinModeEvent, gaPinWriteEvent, gaPinReadEvent, gaPinToggleEvent
} TGpioAction;

typedef struct {
	TGpioAction GpioAction;
	uint16_t NumPin;
	union {
		uint8_t Value;
		uint8_t Mode;
	}
	ActionData;
	TGPIODrive* GPIODrive;

} TGpioData;

//-----------------------------------------------------------------------
typedef enum {
	saGet, saPut, saBeginUseGet, saEndUseGet, saGetLocalAddress, saDisableTX, saEnableTX, saStatusDisableTX, saTaskStream
} TStreamAction;

typedef struct {
	TStreamAction StreamAction;
	void *Data;
	uint32_t Length;
	uint32_t LengthResult;
	uint32_t FromAddress;
	uint32_t ToAddress;

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
	uint32_t SourceAddress;
	char *SourceTaskName;
	uint32_t DestAddress;
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
	tivIP_U32,
	tivInt16,
	tivInt32,
	tivInt64,
	tivUInt8,
	tivUInt8_HEX,
	tivUInt16,
	tivUInt32,
	tivUInt64,
	tiv_double,
	tiv_udouble,

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
	tmaIS_MAINMENU,
	tmaGET_INIT_MENU, 
	tmaGET_CAPTION_MENU_STRING, 
	tmaGET_ITEM_MENU_STRING, 
	tmaCLICK_ITEM_MENU,
	tmaCLICKLEFT_ITEM_MENU,
	tmaCLICKRIGHT_ITEM_MENU,
	tmaESCAPE_MENU,
	tmaDEL_ITEM_MENU,
	tmaINSERT_MENUITEM,
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
	bool NMenu;
} TMenuItemData;

typedef struct
{
	uint8_t ItemIndex;
	bool Close;
	bool Repaint;
} TMenuClickData;

typedef struct
{
	uint8_t ItemIndex;
} TMenuClickLeftRightData;

typedef struct
{
	uint8_t ItemIndex;
	bool ReInit;
} TMenuDelData;

typedef struct
{
	int16_t X;
	int16_t Y;
} TMenuOpenMainData;

typedef struct
{
	bool is;
} TMenuIsMainData;

typedef struct
{
	TTypeMenuAction TypeMenuAction;
	int8_t IDMenu;
	union
	{
		TMenuInitData MenuInitData;
		TMenuItemData MenuItemData;
		TMenuClickData MenuClickData;
		TMenuClickLeftRightData MenuClickLeftRightData;
		TMenuCaptionData MenuCaptionData;
		TMenuDelData MenuDelData;
		TMenuOpenMainData MenuOpenMainData;
		TMenuIsMainData MenuIsMainData;
	} ActionData;
} TMenuData;
//-----------------------------------------------------------------------

typedef enum {
	ida_INIT_INPUTDIALOG,
	ida_GET_CAPTION_STRING,
	ida_GET_DESCRIPTION_STRING,
	ida_ENTER_DIALOG,
	ida_ESCAPE_DIALOG,
	ida_CHANGE_VALUE
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
	String* Value;
} TInputDialogChangeValue;

typedef struct
{
	uint32_t Min;
	uint32_t Max;
} Tuint32MinMax;

typedef struct
{
	uint16_t Min;
	uint16_t Max;
} Tuint16MinMax;

typedef struct
{
	uint8_t Min;
	uint8_t Max;
} Tuint8MinMax;

typedef struct
{
	int16_t Min;
	int16_t Max;
} Tint16MinMax;

typedef struct
{
	TTypeInputVar TypeInputVar;
	uint8_t MaxLength;
	void *DataPointer;
	union 
	{
		Tuint32MinMax uint32MinMax;
		Tuint16MinMax uint16MinMax;
		Tint16MinMax int16MinMax;
		Tuint8MinMax uint8MinMax;
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
		TInputDialogChangeValue InputDialogChangeValue;
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
	KF_INSERT,
	KF_TABNEXT,
	KF_TABPREV,
	KF_CURSORUP,
	KF_CURSORDOWN,
	KF_CURSORLEFT,
	KF_CURSORRIGHT,
	KF_CTRL_CURSORUP,
	KF_CTRL_CURSORDOWN,
	KF_CTRL_CURSORLEFT,
	KF_CTRL_CURSORRIGHT,
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
	TTaskDef* FromStreamTask;
} TKeyboardData;

//-----------------------------------------------------------------------
typedef enum {vdaGetValue,vdaSetValue,vdaGetVarName,vdaGetVarDesc,vdaCountVar} TVarDataAction;

typedef struct
{
	TVarDataAction Action;
	int IndxVar;
	int CountVar;
	String *VarName;
	String *VarValue;
		
} TVarData;

#define VAR(varname) Am->Data.VarData.CountVar++; \
		if (((*Am->Data.VarData.VarName==varname) &&  (Am->Data.VarData.Action!=vdaCountVar)) || (Am->Data.VarData.Action==vdaGetVarName)) \
		{ \
			if ((Am->Data.VarData.Action==vdaGetVarName) && (Am->Data.VarData.CountVar==Am->Data.VarData.IndxVar) ) \
			{ \
				*Am->Data.VarData.VarName = varname; \
				return true; \
			} \

#define GET_VAR if (Am->Data.VarData.Action==vdaGetValue) 
#define GET_DESC if (Am->Data.VarData.Action==vdaGetVarDesc)
#define SET_VAR if (Am->Data.VarData.Action==vdaSetValue) 

#define VALUE_VAR *Am->Data.VarData.VarValue

#define EVAR }


//-----------------------------------------------------------------------
typedef enum { 
	tsaIDLE,
	tsaConnect, 
	tsaDisconnect, 
	tsaConnectError, 
	tsaReceived, 
	tsaSended,
	tsaServerStart,
	tsaServerStop,
	tsaServerStartingError,
	tsaNewClientSocket 
} TTypeSocketAction;

typedef struct
{
	TTypeSocketAction TypeSocketAction;
	TSocket* Socket;
	TSocket* NewClientSocket;
	bool DestroySocket;
	bool AcceptSocket;
	uint32_t ReceivedLength;
} 	TSocketData;
//-----------------------------------------------------------------------
typedef enum { thpaFreePTR,thpaReallocPTR } TTypeHandlePTRAction;
typedef struct
{
	void *OldPTR;
	void *NewPTR;
	void *FreePTR;
	TTypeHandlePTRAction TypeHandlePTRAction;
} 	THandlePTRData;

#define HANDLEPTR(Aptr) __HandlePTR((void **)&(Aptr),Am);
//-----------------------------------------------------------------------
struct TMessageBoard
{
	TIDMessage IDMessage;
	TTask *fromTask;
	union
	{
		TKeyboardData KeyboardData;
#ifdef XB_GUI
		TMenuData MenuData;
		TInputDialogData InputDialogData;
		TWindowData WindowData;
#endif
		TSensorData SensorData;
		TGpioData GpioData;
		TStreamData StreamData;
		TBlinkData BlinkData;
		TFrameReceiveData FrameReceiveData;
		TFrameResponseData FrameResponseData;
		TVarData VarData;
		TSocketData SocketData;
		void *PointerData;
		THandlePTRData HandlePTRData;
		String *PointerString;
		uint64_t uData64;
		uint32_t uData32;
		uint16_t uData16;
		uint8_t uData8;
	} Data;
};

#endif
