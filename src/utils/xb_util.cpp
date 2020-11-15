#include "xb_util.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

	extern void *malloc(size_t size);
	extern void free(void *memblock);
	extern size_t strlen(const char *str);
	extern void *memcpy(void *dest, const void *src, size_t count);
	extern int sprintf(char *buffer, const char *format, ...);
	extern double trunc(double x);
	extern char *strcpy(char *strDestination,const char *strSource);
}

//#include "cbufSerial.h"

int32_t globalonetry=0;

uint8_t monthlen(uint8_t isleapyear, uint8_t month)
{
	if (month == 1)
	{
		return (28 + isleapyear);
	}

	if (month > 6)
	{
		month--;
	}

	if (month % 2 == 1)
	{
		return (30);
	}

	return (31);
}

//----------------------------------------------------------------------------------------------------------------------

#define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800UL
#define	EPOCH_YR	1970
#define	SECS_DAY	86400UL
#define	LEAPYEAR(year)	(!((year) % 4) && (((year) % 100) || !((year) % 400)))
#define	YEARSIZE(year)	(LEAPYEAR(year) ? 366 : 365)


#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24UL)
#define SECS_YR_2000  (946684800UL) // the time at the start of y2k


//----------------------------------------------------------------------------------------------------------------------
void RTC_DecodeUnixTime(uint32_t unix_time, struct tm *dt)
{
	uint32_t dayclock = 0;
	int  dayno = 0;
	//int wday = 0;
	//int summertime = 0;

	dt->tm_year = EPOCH_YR; //=1970
	dayclock = unix_time  % SECS_DAY;
	dayno = unix_time / SECS_DAY;

	dt->tm_sec = dayclock % 60UL;
	dt->tm_min = (dayclock % 3600UL) / 60;
	dt->tm_hour = dayclock / 3600UL;
	dt->tm_wday = (dayno + 4) % 7;      // day 0 was a thursday
	//wday = dt->tm_wday;

	while (dayno >= YEARSIZE(dt->tm_year))
	{
		dayno -= YEARSIZE(dt->tm_year);
		dt->tm_year++;
	}

	dt->tm_mon = 0;
	while (dayno >= monthlen(LEAPYEAR(dt->tm_year), dt->tm_mon))
	{
		dayno -= monthlen(LEAPYEAR(dt->tm_year), dt->tm_mon);
		dt->tm_mon++;
	}
	dt->tm_mon++;
	dt->tm_mday = dayno + 1;

	// Summertime
	//summertime = 1;
	if (dt->tm_mon < 3 || dt->tm_mon > 10)     // month 1, 2, 11, 12
	{
		//summertime = 0;                          // -> Winter
	}

	if ((dt->tm_mday - dt->tm_wday >= 25) && (dt->tm_wday || dt->tm_hour >= 2))
	{                              // after last Sunday 2:00
		if (dt->tm_mon == 10)        // October -> Winter
		{
			//summertime = 0;
		}
	}
	else
	{                              // before last Sunday 2:00
		if (dt->tm_mon == 3)        // March -> Winter
		{
			//summertime = 0;
		}
	}
}

uint32_t RTC_EncodeUnixTime(struct tm *TM)
{
	int second = SECS_YR_2000;
	int i, m, Y = TM->tm_year;//+2000 /*- 2000*/;


	for (i = 2000;i <= Y;i++)
	{
		if ((i /*+ 2000*/) == (Y/*TM->tm_year*/))
		{
			for (m = 1; m < TM->tm_mon; m++)
			{
				if ((m == 2) && (((i /*+ 2000*/) % 4) == 0)) 
				{ 
					second += SECS_PER_DAY * monthlen(1, m - 1);
					
				}
				else 
				{
					second += SECS_PER_DAY * monthlen(0, m - 1);
				}
			}
		}
		else
		{
			if (((i /*+ 2000*/) % 4) == 0)
			{
				second += (SECS_PER_DAY * 366);				
			}
			else
			{
				second += (SECS_PER_DAY * 365);				
			}
				

		}
		
	}
	second += (TM->tm_mday - 1) * SECS_PER_DAY;
	second += TM->tm_hour * SECS_PER_HOUR;
	second += TM->tm_min * SECS_PER_MIN;
	second += TM->tm_sec;

	//second -= tz_CEST;
	
	
	return second;	
}

void GetTimeIndx(String &Atotxt, uint32_t Atimeindx)
{
	uint32_t m = Atimeindx / 60;
	uint32_t s = Atimeindx - (m * 60);
	uint32_t g = m / 60;
	m = m - (g * 60);

#ifdef ARDUINO_ARCH_STM32
	sprintf(timeindxstr, FSS("%4d:%02d:%02d\0"), g, m, s);
#endif

#if defined(ESP8266) 
	ets_sprintf(timeindxstr, FSS("%4d:%02d:%02d"), g, m, s);
#endif

#if defined(ESP32)
//	sprintf(timeindxstr, FSS("%4d:%02d:%02d"), g, m, s);
	char timeindxstr[40]; xb_memoryfill(timeindxstr, 40, 0);
	uint8_t i = 0;
	i += uinttoaw(g, &timeindxstr[i], 4, '0');
	timeindxstr[i++] = ':';
	i += uinttoaw(m, &timeindxstr[i], 2, '0');
	timeindxstr[i++] = '.';
	i += uinttoaw(s, &timeindxstr[i], 2, '0');
#endif

#if defined(__riscv64)
	sprintf(timeindxstr, FSS("%4d:%02d:%02d"), g, m, s);
#endif
	Atotxt += String(timeindxstr);
	
}

void GetTime(String& Atotxt, uint32_t Adatetimeunix,bool Ayear, bool Amonth, bool Aday )
{
	tm dt;
	RTC_DecodeUnixTime(Adatetimeunix, &dt);
#if defined(ESP32)
	char timeindxstr[40]; xb_memoryfill(timeindxstr, 32, 0);
	uint8_t i = 0;
	if (Ayear)
	{
		i += uinttoaw(dt.tm_year, &timeindxstr[i], 4, '0');
		if ((!Amonth) && (!Aday)) timeindxstr[i++] = ' ';
	}
	if (Amonth)
	{
		if (Ayear) timeindxstr[i++] = '-';
		i += uinttoaw(dt.tm_mon, &timeindxstr[i], 2, '0');
		if (!Aday) timeindxstr[i++] = ' ';
	}
	if (Aday)
	{
		if ((Amonth) || (Ayear)) timeindxstr[i++] = '-';
		i += uinttoaw(dt.tm_mday, &timeindxstr[i], 2, '0');
		timeindxstr[i++] = ' ';
	}
	i += uinttoaw(dt.tm_hour, &timeindxstr[i], 2, '0');
	timeindxstr[i++] = ':';
	i += uinttoaw(dt.tm_min, &timeindxstr[i], 2, '0');
	timeindxstr[i++] = '.';
	i += uinttoaw(dt.tm_sec, &timeindxstr[i], 2, '0');
#endif

	Atotxt += String(timeindxstr);

}

//----------------------------------------------------------------------------------------------------------------------
uint8_t ahextoint(REGISTER uint8_t Ach)
{
	if ((Ach >= '0') && (Ach <= '9'))  return Ach - '0';
	if ((Ach >= 'a') && (Ach <= 'f'))  return Ach - 'a' + 10;
	if ((Ach >= 'A') && (Ach <= 'F'))  return Ach - 'A' + 10;
	return 0xff;
}

//#include <bits/basic_string.h>

double strtodouble(String Astrdouble)
{
	//std::string s();
	double result = 0;
	result = std::strtod(Astrdouble.c_str(),NULL);
	
	return result;
}

uint8_t doubletostr(double v,char *buf,uint8_t prec)
{
	String s = String(v,prec);
	s.toCharArray(buf, 16, 0);
	return s.length();
	
	
	int d1 = v;            // Get the integer part (678).
	double f2 = (double)v - d1;     // Get fractional part (0.01234567).
	int d2 = (int)trunc(f2 * 10000.00);   // Turn into integer (123).
	double f3 = f2 * 10000.00 - d2;   // Get next fractional part (0.4567).
	int d3 = (int)trunc(f3 * 10000.00);   // Turn into integer (4567).
	uint8_t min=0,d , i, c = sprintf(buf, "%d.%04d%04d", d1, d2, d3);



	for (i = 0; i < c; i++)
	{
		if (buf[i] == '.')
		{
			for (d = i+1; d < c; d++)
			{
				if (buf[d] != '-')
				{
					prec--;
					if (prec == 0)
					{
						if (min > 0)
						{
							for (uint8_t cm = min; cm < d; cm++)
							{
								buf[cm] = buf[cm + 1];
							}
							d--;
						}
						
						
						return d + 1;
					}
				}
				else
				{
					min = d;
				}
			}
		}
	}
	return c;
}

double CutPrecision(volatile double val, uint8_t prec)
{
	volatile int32_t l=0,x=0;
	volatile double w=0;
	switch (prec)
	{
	case 1:
		l = val * 10;
		return l / 10;
	case 2:
		l = val * 100;
		x=l % 100;
		w=x* 0.01;
		x=l/100;
		w=w+x;
		w=w-val;
		return w;
	case 3:
		l = val * 1000;
		val=l / 1000.00;
		return  val;
	case 4:
		l = val * 10000;
		return l / 10000;
	default:
		return val;
	}
}

char* _itoa(int value, char* result, int base) 
{
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

uint8_t inttoa(REGISTER int32_t value, REGISTER char* result)
{
	REGISTER char *ptr = result, *ptr1 = result, tmp_char;
	REGISTER int32_t tmp_value;
	uint8_t s;

	do {
		tmp_value = value;
		value /= 10;
		*ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	s = ptr - result;
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return s;
}

uint8_t inttoaw(REGISTER int32_t value, REGISTER  char* result, uint8_t Awidth, char Ach)
{
	REGISTER char *ptr = result, *ptr1 = result, tmp_char;
	REGISTER int32_t tmp_value;
	uint8_t s;

	do {
		tmp_value = value;
		value /= 10;
		*ptr++ = "9876543210123456789"[9 + (tmp_value - value * 10)];
	} while (value);

	while ((ptr - result) < Awidth)
	{
		*ptr++ = Ach;
	}
	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	s = ptr - result;
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return s;
}

uint8_t uinttoaw(REGISTER uint32_t value, REGISTER  char* result, uint8_t Awidth, char Ach)
{
	REGISTER char *ptr = result, *ptr1 = result, tmp_char;
	REGISTER uint32_t tmp_value;
	uint8_t s;

	do {
		tmp_value = value;
		value /= 10;
		*ptr++ = "9876543210123456789"[9 + (tmp_value - value * 10)];
	} while (value);

	while ((ptr - result) < Awidth)
	{
		*ptr++ = Ach;
	}
	// Apply negative sign
	s = ptr - result;
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return s;
}

uint8_t uinttoa(REGISTER uint32_t value, REGISTER char* result)
{
	REGISTER char *ptr = result, *ptr1 = result, tmp_char;
	REGISTER uint32_t tmp_value;
	uint8_t s;

	do {
		tmp_value = value;
		value /= 10;
		*ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
	} while ( value );

//	if (tmp_value < 0) *ptr++ = '-';
	s = ptr - result;
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return s;
}
//=================================================================================================================
bool hexstrTouint32(char *Astr, int8_t Alen, uint32_t *Aint)
{
	uint8_t x = 0;
	*Aint = 0;
	for (int8_t i = 0; i < Alen; i++)
	{
		x = ahextoint(Astr[i]);
		if (x != 255)
		{
			*Aint = (*Aint << 4);
			*Aint |= x;
		}
		else
		{
			break;
		}
	}
	return true;
}
//=================================================================================================================
const char tab[17] = "0123456789ABCDEF";
void uint32tohexstr(char *Aresult, uint32_t *Aint32tab, uint8_t Acount, bool Aadd)
{
	REGISTER uint32_t Aint    ;
	REGISTER uint8_t c;
	REGISTER int32_t i;
	char result[8];
	if (Acount > 8) Acount = 8;
	
	for (c = 0;c < Acount;c++)
	{
		for (i = 0;i < 8;i++) Aresult[i] = '0';
		Aint = Aint32tab[c];
		for (i = 7; i >= 0; i--)
		{
			result[i] = tab[Aint & 0x0f];
			Aint >>= 4;
		}
		if (Aadd) for (i=0;i<8;i++) *(Aresult++)=result[i];
		else for (i=0;i<8;i++) Aresult[i]=result[i];
	}

}
//=================================================================================================================
void uint16tohexstr(char *Aresult, uint16_t *Aint16tab, uint8_t Acount, bool Aadd)
{
	REGISTER uint16_t Aint;
	REGISTER uint8_t c;
	REGISTER int32_t i;
	char result[8];
	if (Acount > 8) Acount = 8;
	
	for (c = 0; c < Acount; c++)
	{
		for (i = 0; i < 4; i++) Aresult[i] = '0';
		Aint = Aint16tab[c];
		for (i = 3; i >= 0; i--)
		{
			result[i] = tab[Aint & 0x0f];
			Aint >>= 4;
		}
		if (Aadd) for (i = 0; i < 4; i++) *(Aresult++) = result[i];
		else for (i = 0; i < 4; i++) Aresult[i] = result[i];
	}

}
//=================================================================================================================
void uint8tohexstr(char *Aresult, uint8_t *Aint8tab, uint32_t Acount, char Asep,bool Aenter)
{
	REGISTER uint8_t Aint;
	REGISTER int32_t i;
	char sep = 0;
	for (i = 0; i < Acount; i++)
	{
		if (sep != 0)
		{
			*(Aresult++) = Asep;
		}
		Aint = Aint8tab[i];
		*(Aresult++) = tab[(Aint & 0xf0) >> 4];
		*(Aresult++) = tab[(Aint & 0x0f)];
		if (Aenter)
		{
			if (Aint == 0x0a) 
			{
				*(Aresult++) = 0x0d;
				*(Aresult++) = 0x0a;
			}
		}
		sep = Asep;
	}
}
//=================================================================================================================
int IndexOfChars(char *Astr, int Afrom, const char *Aofchars, int Alench)
{
	int len = strlen(Astr)+1;
	int indx;
	int indxch;
	
	for (indx = Afrom; indx < len; indx++)
	{
		for (indxch = 0; indxch < Alench; indxch++)
		{
			if (Astr[indx] == Aofchars[indxch])
			{
				return indx;
			}
		}
	}
	return -1;
}
//=================================================================================================================
// Wyszukanie Stringa w stringu
// <- indeks wzglêdem Astr koñca znalezionego stringu lub 0 nie znaleziono
// <- Aposindx indeks wzglêdem Astr znalezionego stringu
// -> Asubstr szukany
// -> Astr przeszukiwany
uint32_t StringPos(const char *Astr,const char *Asubstr,uint32_t *Aposindx)
{
	uint32_t indxstr=0, indxsubstr=0;
	uint32_t lensubstr = strlen(Asubstr);
	uint32_t lenstr = strlen(Astr)-lensubstr;
	for (;indxstr < lenstr;indxstr++)
	{
		for (indxsubstr = 0;indxsubstr < lensubstr;indxsubstr++)
		{
			if (Astr[indxstr] != Asubstr[indxsubstr])			
			{
				indxstr -= indxsubstr;
				indxsubstr = 0;
				break;
			}
			else
			{
				indxstr++;
			}
		}
		if (indxsubstr != 0)
		{
			*Aposindx = indxstr - lensubstr;
			return indxstr;
		}
	}
	return 0;
}

bool StringTrim(char *Astr, char Achtrim)
{
	uint32_t i, ci = 0;
	
	for (i = 0;Astr[i] != 0;i++)
	{
		if (Astr[i] != Achtrim)
		{
			ci = i;
			break;
		}
	}
	
	if (ci != 0)
	{
		strcpy(&Astr[0], &Astr[ci]);
	}
	
	for (i = 0;Astr[i] != 0;i++)
	{
		if (Astr[i] != Achtrim)
		{
			ci = i;
		}
	}
	
	Astr[ci + 1] = 0;
	return true;
	

}

bool StringTrimRight(char* Astr,char Achtrim)
{
	uint32_t i, ci = 0;

	for (i = 0; Astr[i] != 0; i++)
	{
		if (Astr[i] != Achtrim)
		{
			ci = i;
		}
	}

	Astr[ci + 1] = 0;
	return true;


}

bool StringTrimRight(String *Astr, char Achtrim)
{
	uint32_t i, ci = 0;

	for (i = 0; Astr->charAt(i) != 0; i++)
	{
		if (Astr->charAt(i) != Achtrim)
		{
			ci = i;
		}
	}

	*Astr = Astr->substring(0,ci+1);
	return true;


}

bool StringTrimLeft(char* Astr, char Achtrim)
{
	uint32_t i, ci = 0;

	for (i = 0; Astr[i] != 0; i++)
	{
		if (Astr[i] != Achtrim)
		{
			ci = i;
			break;
		}
	}

	if (ci != 0)
	{
		strcpy(&Astr[0], &Astr[ci]);
	}

	return true;


}


uint8_t IPtoString(uint32_t Aip, char *Asip)
{
	uint8_t l, al = 0;
	l = inttoa((Aip >> 24), Asip);
	Asip += l; al += l;
	*Asip = '.'; 
	Asip++; al++;
	l = inttoa(((Aip >> 16) & 0xff), Asip);
	Asip += l; al += l;
	*Asip = '.';
	Asip++; al++;
	l = inttoa(((Aip >> 8) & 0xff), Asip);
	Asip += l; al += l;
	*Asip = '.';
	Asip++; al++;
	l = inttoa((Aip & 0xff), Asip);
	Asip += l; al += l;
	*Asip = 0;
	return al;
}

uint8_t StringToUINT(char *Astr, uint32_t *Aint)
{
	uint8_t l = 0;
	*Aint = 0;
	while (*Astr == ' ') 
	{
		Astr++; l++;
	}

	while (1)
	{
		if ((*Astr >= '0') && (*Astr <= '9'))
		{
			*Aint = ((*Aint * 10) + (*Astr - '0'));
		}
		else break;
		Astr++; l++;
	}
	return l;
}

uint8_t StringToUINT(const char* Astr, uint32_t* Aint)
{
	uint8_t l = 0;
	*Aint = 0;
	while (*Astr == ' ')
	{
		Astr++; l++;
	}

	while (1)
	{
		if ((*Astr >= '0') && (*Astr <= '9'))
		{
			*Aint = ((*Aint * 10) + (*Astr - '0'));
		}
		else break;
		Astr++; l++;
	}
	return l;
}


uint8_t StringHEXToUINT(char *Astr, uint32_t *Aint)
{
	uint8_t l = 0;
	*Aint = 0;
	while (*Astr == ' ') {
		Astr++; l++;
	}
	;

	while (1)
	{
		if (((*Astr >= '0') && (*Astr <= '9')) || ((*Astr >= 'a') && (*Astr <= 'f')) || ((*Astr >= 'A') && (*Astr <= 'F')))
		{
			*Aint = ((*Aint) << 4);
			if ((*Astr >= '0') && (*Astr <= '9'))
			{
				*Aint |= ((uint32_t)(*Astr - '0'));
			}
			else if ((*Astr >= 'a') && (*Astr <= 'f'))
			{
				*Aint |= (((uint32_t)(*Astr - 'a')) + 10);
			}
			else if ((*Astr >= 'A') && (*Astr <= 'F'))
			{
				*Aint |= (((uint32_t)(*Astr - 'A')) + 10);
			}
		}
		else break;
		Astr++; l++;
	}
	return l;
}

uint8_t StringtoIP(char *Asip, uint32_t *Aip)
{
	//uint8_t i = 0;
	uint8_t l = 0, al = 0;
	uint32_t v = 0, ip = 0;

	l = StringToUINT(Asip, &v);
	if (l == 0) return 0;
	Asip += l; al += l;
	if (*Asip != '.') return 0;
	Asip++;al++;
	ip |= (v << 24);

	l = StringToUINT(Asip, &v);
	if (l == 0) return 0;
	Asip += l; al += l;
	if (*Asip != '.') return 0;
	Asip++;al++;
	ip |= (v << 16);

	l = StringToUINT(Asip, &v);
	if (l == 0) return 0;
	Asip += l; al += l;
	if (*Asip != '.') return 0;
	Asip++;al++;
	ip |= (v << 8);

	l = StringToUINT(Asip, &v);
	if (l == 0) return 0;
	Asip += l; al += l;
	Asip++;al++;
	ip |= (v);

	*Aip = ip;

	return al;
}

uint32_t StringLength(REGISTER char *Astr,REGISTER uint8_t Acharend)
{
	for (REGISTER uint32_t i = 0;;i++)
	{
		if (((uint8_t *)Astr)[i] == Acharend)
		{
		     return i;
        }
	}
}

uint32_t StringLength(const char *Astr, REGISTER uint8_t Acharend)
{
	for (REGISTER uint32_t i = 0;; i++)
	{
		if (((uint8_t *)Astr)[i] == Acharend)
		{
			return i;
		}
	}
}

uint32_t StringAddString(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER char *Aaddstr, REGISTER uint8_t Aaddcharend)
{
	uint32_t indx = StringLength(Astr, Acharend);
	uint32_t l = StringLength(Aaddstr, Aaddcharend);
	
	xb_memorycopy((void *)Aaddstr, (void *)&Astr[indx], l);
	Astr[indx + l] = Acharend;
	return indx + l;
}

uint32_t StringAddUINT8(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint8_t Aadduint)
{
	char Aaddstr[16] = { 0 };
	uinttoa(Aadduint, Aaddstr);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddUINT8w(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint8_t Aadduint,uint8_t Awidth,char Achw)
{
	char Aaddstr[16] = { 0 };
	uinttoaw(Aadduint, Aaddstr,Awidth,Achw);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddUINT16(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint16_t Aadduint)
{
	char Aaddstr[16] = { 0 };
	uinttoa(Aadduint, Aaddstr);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddUINT16w(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint16_t Aadduint, uint8_t Awidth, char Achw)
{
	char Aaddstr[16] = { 0 };
	uinttoaw(Aadduint, Aaddstr, Awidth, Achw);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddUINT32(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint32_t Aadduint)
{
	char Aaddstr[16] = { 0 };
	uinttoa(Aadduint, Aaddstr);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddUINT32w(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint32_t Aadduint, uint8_t Awidth, char Achw)
{
	char Aaddstr[16] = { 0 };
	uinttoaw(Aadduint, Aaddstr, Awidth, Achw);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddHexUINT32(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER uint32_t Aadduint)
{
	char Aaddstr[16] = { 0 };
	uint32tohexstr(Aaddstr, &Aadduint,1,true);
	return StringAddString(Astr, Acharend, Aaddstr, 0);
}

uint32_t StringAddChar(REGISTER char *Astr, REGISTER uint8_t Acharend, REGISTER char Aaddchar)
{
	uint32_t indx = StringLength(Astr, Acharend);
	Astr[indx]	= Aaddchar;
	indx++;
	Astr[indx]	= Acharend;
	return indx;
}


void uintcat(char *Astr, uint32_t Avalue)
{
	uint32_t l = strlen(Astr);
	uinttoa(Avalue, &Astr[l]);
}

void charcat(char *Astr, char Ach)
{
	uint32_t l = strlen(Astr);
	Astr[l++] = Ach;
	Astr[l] = 0;

}

void StringSetWidth(String &Astr, uint32_t Awidth, TStringTextAlignment Astringtextalignment, char Ach)
{
	if (Astringtextalignment == staLeft)
	{
		if (Astr.length() == Awidth) return;	
		if (Astr.length() < Awidth) 
		{
			uint32_t l = Awidth - Astr.length();
			for (uint32_t i = 0; i < l; i++) Astr += Ach;
			return;
		}
		if (Astr.length() > Awidth) 
		{
			Astr = Astr.substring(Awidth , Astr.length());
			return;
		}
		
	}
	else if (Astringtextalignment == staCentre)
	{
		
		
	}
	else if (Astringtextalignment == staRight)
	{
		
		
	}
	
}

/*int round(float value) {
	float val1;
	if (value < 0.0f)
		val1 = value - 0.5f;
	else
		val1 = value + 0.5f;
	int intPart = int(val1);
	return intPart;
}*/

float _CutPrecision(float liczba,uint8_t digprecis)
{
	switch (digprecis)
	{
	default:
	case 2:
		return round(liczba * 100.0f) / 100.0f;
	case 1:
		return round(liczba * 10.0f) / 10.0f;
	}
}

void xb_memoryfill(REGISTER void *Aadr, REGISTER uint32_t Alength, REGISTER uint8_t Avalue)
{
	for (REGISTER uint32_t i = 0;i<Alength;i++) ((uint8_t *)Aadr)[i] = Avalue;
}

void xb_memorycopy(REGISTER void *Asource, REGISTER void *Adestination, REGISTER int32_t Alength)
{ 
	if (Alength == -1)
	{
		uint8_t ch;
		for (REGISTER uint32_t i = 0;; i++) 
		{
			 ch = ((uint8_t *)Asource)[i];
			if (ch == 0)
			{
				((uint8_t *)Adestination)[i] = ch;	
				return;
			}
			else
				((uint8_t *)Adestination)[i] = ch;	
		}
	}
	else
	{
		for (REGISTER uint32_t i = 0; i < Alength; i++) ((uint8_t *)Adestination)[i] = ((uint8_t *)Asource)[i];	
	}		
	
}

uint32_t xb_memorycompare(REGISTER void *Aadr1, REGISTER void *Aadr2, REGISTER uint32_t Alength,REGISTER uint8_t Aendch)
{
	uint32_t lencompre = 0;

	if (Alength == 0)
	{
		uint32_t  l1 = StringLength((const char *)Aadr1, Aendch);
		uint32_t  l2 = StringLength((const char *)Aadr2, Aendch);

		if (l1 <= l2)
		{
			Alength = l1;
		}
		else
		{
			Alength = l2;
		}
	}

	for (REGISTER uint32_t i = 0;i<Alength;i++)
	{
		if (((uint8_t *)Aadr1)[i] != ((uint8_t *)Aadr2)[i])
		{
			lencompre = 0;
			return lencompre;
		}
		lencompre++;
	}

	return lencompre;
}

