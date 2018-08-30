#ifndef cbufSerial_h
#define cbufSerial_h

#if defined(ESP8266) || defined(ESP32)
#include <inttypes.h>
#include <cbuf.h>
#else
#include "_cbuf.h"
#endif
#include "Stream.h"

class cbufSerial : public Stream
{
public:
	cbufSerial(int Asizebuf);
	virtual ~cbufSerial() { delete(buffer); }

	bool empty();
	int available(void) override;
	int peek(void) override;
	void flush(void) override;
	int read(void) override;
	size_t write(uint8_t) override;

	String peekString();

	inline size_t write(unsigned long n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(long n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(unsigned int n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(int n)
	{
		return write((uint8_t)n);
	}
	using Print::write; // pull in write(str) and write(buf, size) from Print

protected:
cbuf *buffer;
};

extern void clearbuf(cbufSerial *Abuf);

#endif
