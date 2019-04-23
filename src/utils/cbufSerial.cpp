#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include <utils/cbufSerial.h>

void clearbuf(cbufSerial *Abuf)
{
	while (Abuf->available()) Abuf->read();
}

cbufSerial::cbufSerial(int Asizebuf)
{
	buffer = new cbuf(Asizebuf);
	setTimeout(0);
}

String cbufSerial::peekString()
{
	String s=this->readString();
	this->print(s);
	return s;
}

bool cbufSerial::empty()
{
	return buffer->empty();
}

int cbufSerial::available(void)
{
	return buffer->available();
}

int cbufSerial::peek(void)
{
	return buffer->peek();
}

#ifndef __riscv64
void cbufSerial::flush(void)
{
	buffer->flush();
}
#endif


int cbufSerial::read(void)
{
	return buffer->read();
}

size_t cbufSerial::write(uint8_t c)
{
	;
	return buffer->write(c);
}



