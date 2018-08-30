#ifndef __XB_UTIL_H__
#define __XB_UTIL_H__

#include <xb_board.h>
#include <utils\cbufSerial.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#ifndef ICACHE_FLASH_ATTR
#define ICACHE_FLASH_ATTR
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FSS
#define FSS(str) (String(F(str)).c_str())
#endif
//#define FSS(str) str

#define BEGIN_MAXMS(Aname) \
{ \
uint32_t Aname = SysTickCount; \
while(1) \
{

#define END_MAXMS() break;\
} \
}

#define	MAXMS_SKIP(Aname,Ams) if ((SysTickCount-Aname)>Ams) break;


#define DEF_WAITMS_VAR(Aname) \
static uint32_t Aname=0; 

#define BEGIN_WAITMS(Aname,Ams) \
{ \
if (Aname==0) Aname=SysTickCount; \
if (SysTickCount-Aname>(Ams)) \
{

#define END_WAITMS(Aname) \
EXITlab##Aname: \
if (Aname!=0) \
Aname = SysTickCount; \
} \
}

#define BEGIN_WAITMS_PREC(Aname,Ams) \
{ \
if (Aname==0) Aname=SysTickCount; \
if (SysTickCount-Aname>(Ams)) \
{ uint32_t Aname##_r=(SysTickCount-Aname)-Ams; 

#define END_WAITMS_PREC(Aname) \
EXITlab##Aname: \
if (Aname!=0) \
Aname = SysTickCount - Aname##_r; \
} \
}

#define END_WAITMS_BREAK(Aname) \
EXITlab##Aname: \
if (Aname!=0) \
Aname = SysTickCount; \
} \
break; \
}
	

#define EXIT_WAITMS(Aname) goto EXITlab##Aname;


#define RESET_WAITMS(Aname) {Aname = 0;} 
#define RESET_WAITMS_ADD(Aname,Aadd) {Aname = SysTickCount+Aadd;} 

#define TRY(type) {type result;
#define FINALLY(res__) result=res__; finally: {
#define END }return result;}
#define _RETURN(res__) {result=res__;goto finally;}

#define VTRY {
#define VFINALLY  finally: {
#define VEND }return;}
#define VRETURN {goto finally;}

#define OTRY(type) {type result;static int32_t onetry=0; while(onetry>0) onetry++;
#define OFINALLY(res__) result=res__; ofinally: {
#define OEND }onetry--;return result;}
#define ORETURN(res__) {result=res__;goto ofinally;}

#define OVTRY {static int32_t onetry=0; while(onetry>0) {globalonetry++;} onetry++;
#define OVFINALLY  finally: {
#define OVEND }onetry--;return;}
#define OVRETURN {goto finally;}

extern int32_t globalonetry;
	
#define tz_CEST ((2*60*60))    ///< Central Europe Summer Time




// Example of __DATE__ string: "Jul 27 2012"
// Example of __TIME__ string: "21:06:19"

//	(__DATE__[7] - '0') * 1000 +
//	(__DATE__[8] - '0') *  100 +


#define COMPUTE_BUILD_YEAR \
	( \
		(__DATE__[ 9] - '0') *   10 + \
		(__DATE__[10] - '0') \
	)


#define COMPUTE_BUILD_DAY \
	( \
		((__DATE__[4] >= '0') ? (__DATE__[4] - '0') * 10 : 0) + \
		(__DATE__[5] - '0') \
	)


#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')


#define COMPUTE_BUILD_MONTH \
	( \
		(BUILD_MONTH_IS_JAN) ?  1 : \
		(BUILD_MONTH_IS_FEB) ?  2 : \
		(BUILD_MONTH_IS_MAR) ?  3 : \
		(BUILD_MONTH_IS_APR) ?  4 : \
		(BUILD_MONTH_IS_MAY) ?  5 : \
		(BUILD_MONTH_IS_JUN) ?  6 : \
		(BUILD_MONTH_IS_JUL) ?  7 : \
		(BUILD_MONTH_IS_AUG) ?  8 : \
		(BUILD_MONTH_IS_SEP) ?  9 : \
		(BUILD_MONTH_IS_OCT) ? 10 : \
		(BUILD_MONTH_IS_NOV) ? 11 : \
		(BUILD_MONTH_IS_DEC) ? 12 : \
		/* error default */  99 \
	)

#define COMPUTE_BUILD_HOUR ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0')
#define COMPUTE_BUILD_MIN  ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0')
#define COMPUTE_BUILD_SEC  ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0')


#define BUILD_DATE_IS_BAD (__DATE__[0] == '?')

#define BUILD_YEAR  ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_YEAR)
#define BUILD_MONTH ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_MONTH)
#define BUILD_DAY   ((BUILD_DATE_IS_BAD) ? 99 : COMPUTE_BUILD_DAY)

#define BUILD_TIME_IS_BAD (__TIME__[0] == '?')

#define BUILD_HOUR  ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_HOUR)
#define BUILD_MIN   ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_MIN)
#define BUILD_SEC   ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_SEC)

#define BUILD_VERSION   ((BUILD_TIME_IS_BAD) ? 99 :  COMPUTE_BUILD_SEC)






extern void RTC_DecodeUnixTime(uint32_t unix_time, struct tm *dt);	
extern uint32_t RTC_EncodeUnixTime(struct tm *TM);
extern void GetTimeIndx(String &Atotxt, uint32_t Atimeindx);
extern void GetTimeIndx(cbufSerial *AcbufSerial, uint32_t Atimeindx);
extern uint8_t ahextoint(register uint8_t Ach);
extern uint8_t doubletostr(double v,char *buf, uint8_t prec = 2);
extern double CutPrecision(volatile double val, uint8_t prec);
extern char* _itoa(int value, char* result, int base);
extern uint8_t inttoa(register int32_t value,register  char* result);
extern uint8_t inttoaw(register int32_t value, register  char* result,uint8_t Awidth,char Ach);
extern uint8_t uinttoa(register uint32_t value, register char* result);
extern bool hexstrTouint32(char *Astr, int8_t Alen, uint32_t *Aint);
extern void uint32tohexstr(char *Aresult, uint32_t *Aint32tab, uint8_t Acount, bool Aadd);
extern void uint16tohexstr(char *Aresult, uint16_t *Aint16tab, uint8_t Acount, bool Aadd);
extern void uint8tohexstr(char *Aresult, uint8_t *Aint8tab, uint8_t Acount, char Asep=0);
extern int IndexOfChars(char *Astr, int Afrom, const char *Aofchars, int Alench);
extern uint32_t StringPos(const char *Astr,const char *Asubstr, uint32_t *Aposindx);
extern bool StringTrim(char *Astr);
extern uint8_t IPtoString(uint32_t Aip, char *Asip);
extern uint8_t StringToUINT(char *Astr, uint32_t *Aint);
extern uint8_t StringHEXToUINT(char *Astr, uint32_t *Aint);
extern uint8_t StringtoIP(char *Asip, uint32_t *Aip);
extern uint32_t StringLength(register char *Astr, register uint8_t Acharend);
extern uint32_t StringAddString(register char *Astr, register uint8_t Acharend, register char *Aaddstr, register uint8_t Aaddcharend);
extern uint32_t StringAddUINT8(register char *Astr, register uint8_t Acharend, register uint8_t Aadduint);
extern uint32_t StringAddUINT8w(register char *Astr, register uint8_t Acharend, register uint8_t Aadduint, uint8_t Awidth, char Achw);
extern uint32_t StringAddUINT16(register char *Astr, register uint8_t Acharend, register uint16_t Aadduint);
extern uint32_t StringAddUINT16w(register char *Astr, register uint8_t Acharend, register uint16_t Aadduint, uint8_t Awidth, char Achw);
extern uint32_t StringAddUINT32(register char *Astr, register uint8_t Acharend, register uint32_t Aadduint);
extern uint32_t StringAddUINT32w(register char *Astr, register uint8_t Acharend, register uint32_t Aadduint, uint8_t Awidth, char Achw);
extern uint32_t StringAddHexUINT32(register char *Astr, register uint8_t Acharend, register uint32_t Aadduint);
extern uint32_t StringAddChar(register char *Astr, register uint8_t Acharend, register char Aaddchar);
extern void uintcat(char *Astr, uint32_t Avalue);
extern void charcat(char *Astr, char Ach);
extern void xb_memoryfill(register void *Aadr, register uint32_t Alength, register uint8_t Avalue);
extern void xb_memorycopy(register void *Asource, register void *Adestination, register int32_t Alength);
extern bool xb_memorycompare(register void *Aadr1, register void *Aadr2, register uint32_t Alength);

#endif
