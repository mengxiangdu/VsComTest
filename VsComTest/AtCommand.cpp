/////////////////////////////////////////////////////////////////////////////////////////
// AT命令解析文件。命令格式为S：short；I：int；L：long long；D：double；C：string。用逗
// 号分割可选的参数。比如：HS,C,CC表示命令数据类型可以是HS、HSC或HSCCC。AT命令4种类型：
// 读（查询）：AT+<x>?
// 测试：AT+<x>=?
// 写：AT+<x>=<...>
// 执行：AT+<x>
// 
// 孟祥度，@2022年4月8日
// 创建此文件。
/////////////////////////////////////////////////////////////////////////////////////////
#include "AtCommand.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>
#include <mutex>
#include <condition_variable>
#include <queue>

using std::queue;
using std::mutex;
using std::unique_lock;
using std::condition_variable;

static mutex acMutex;
static condition_variable acCondi;
static queue<AcMessage> acQueue;

bool acGetMessage(AcMessage* msg)
{
	unique_lock<mutex> uniLock(acMutex);
	acCondi.wait(uniLock, []() { return !acQueue.empty(); });
	*msg = acQueue.front();
	acQueue.pop();
	return msg->id != AcMessageId::QUIT;
}

void acPostMessage(const AcMessage *msg)
{
	acMutex.lock();
	acQueue.push(*msg);
	acMutex.unlock();
	acCondi.notify_one();
}

//---------------------------------------------------------------------------------------
// 这个函数是用来打印不定数量字符串的，未来会改造成从串口输出数据
// num：输入参数数量，参数是const char*
//---------------------------------------------------------------------------------------
void acPrint(int num, ...)
{
	va_list params;
	va_start(params, num);
	for (int i = 0; i < num; i++)
	{
		printf_s("%s", va_arg(params, const char*));
	}
	va_end(params);
}

/////////////////////////////////////////////////////////////////////////////////////////

static bool acEcho = false;

void e0Exec(AcStream* params)
{
	acEcho = false;
	acPrint(1, "\r\nOK\r\n");
}

void e1Exec(AcStream* params)
{
	acEcho = true;
	acPrint(1, "\r\nOK\r\n");
}

void startWrite(AcStream* params)
{
	int16 p1;
	acAsGet(params, 'S', &p1);
	const char* p2;
	acAsGet(params, 'C', &p2);
	int32 p3;
	acAsGet(params, 'I', &p3);
	float64 p4;
	acAsGet(params, 'D', &p4);
	printf_s("input p1 is: %d, p2 is: %s, p3 is %d, p4 is %f.\r\n", p1, p2, p3, p4);
	acPrint(1, "\r\nOK\r\n");
}

/////////////////////////////////////////////////////////////////////////////////////////

//---------------------------------------------------------------------------------------
// 存储所有的AT命令的名称和处理函数
//---------------------------------------------------------------------------------------
static const AcCommand commands[] =
{
	{ "E0", "", 0, 0, 0, e0Exec },
	{ "E1", "", 0, 0, 0, e1Exec },
	{ "+START", "SCID", 0, startWrite, 0, 0 },
};

bool acIsSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool acIsDigit(char c)
{
	return c >= '0' && c <= '9';
}

bool acIsSeperator(char c)
{
	return !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

bool acAsMoveNext(AcStream* params)
{
	if (*params->cursor == '\0')
	{
		params->cursor++;
	}
	bool hasMore = (params->cursor < params->streamEnd);
	return hasMore;
}

bool acConvertToInt16(const char** pos, int16* output)
{
	const char* p = *pos;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	int16 x = 0;
	while (acIsDigit(*p))
	{
		int lastChar = *p - '0';
		if (!negative)
		{
			if ((INT16_MAX - lastChar) / 10 < x)
			{
				return false;
			}
			x = x * 10 + lastChar;
		}
		else
		{
			if ((INT16_MIN + lastChar) / 10 > x)
			{
				return false;
			}
			x = x * 10 - lastChar;
		}
		p++;
	}
	*output = x;
	*pos = p;
	return true;
}

bool acConvertToInt32(const char** pos, int32* output)
{
	const char* p = *pos;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	int32 x = 0;
	while (acIsDigit(*p))
	{
		int lastChar = *p - '0';
		if (!negative)
		{
			if ((INT32_MAX - lastChar) / 10 < x)
			{
				return false;
			}
			x = x * 10 + lastChar;
		}
		else
		{
			if ((INT32_MIN + lastChar) / 10 > x)
			{
				return false;
			}
			x = x * 10 - lastChar;
		}
		p++;
	}
	*output = x;
	*pos = p;
	return true;
}

bool acConvertToInt64(const char** pos, int64* output)
{
	const char* p = *pos;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	int64 x = 0;
	while (acIsDigit(*p))
	{
		int lastChar = *p - '0';
		if (!negative)
		{
			if ((INT64_MAX - lastChar) / 10 < x)
			{
				return false;
			}
			x = x * 10 + lastChar;
		}
		else
		{
			if ((INT64_MIN + lastChar) / 10 > x)
			{
				return false;
			}
			x = x * 10 - lastChar;
		}
		p++;
	}
	*output = x;
	*pos = p;
	return true;
}

bool acConvertToFloat64(const char** pos, float64* output)
{
	const char* p = *pos;
	bool negative = false;
	if (*p == '-')
	{
		negative = true;
		p++;
	}
	float64 x = 0;
	float64 dotv = 0;
	while (acIsDigit(*p))
	{
		int lastChar = *p - '0';
		if (!negative)
		{
			x = x * 10 + lastChar;
		}
		else
		{
			x = x * 10 - lastChar;
		}
		p++;
	}
	if (*p == '.')
	{
		p++;
		float64 base = 10;
		while (acIsDigit(*p))
		{
			int lastChar = *p - '0';
			if (!negative)
			{
				dotv += lastChar / base;
			}
			else
			{
				dotv -= lastChar / base;
			}
			base *= 10;
			p++;
		}
	}
	*output = x + dotv;
	*pos = p;
	return true;
}

bool acConvertToString(const char** pos, const char** output)
{
	const char* p = *pos;
	while (*p != '\0')
	{
		p++;
	}
	*output = *pos;
	*pos = p;
	return true;
}

bool acAsIsParameterRight(AcStream* acs, const char* format)
{
	union 
	{
		short s;
		int i;
		long long l;
		double d;
		const char* c;
	} value;
	bool good = false;
	while (acAsMoveNext(acs) && *format != '\0')
	{
		switch (*format)
		{
		case 'S':
			good = acConvertToInt16(&acs->cursor, &value.s);
			break;
		case 'I':
			good = acConvertToInt32(&acs->cursor, &value.i);
			break;
		case 'L':
			good = acConvertToInt64(&acs->cursor, &value.l);
			break;
		case 'D':
			good = acConvertToFloat64(&acs->cursor, &value.d);
			break;
		case 'C':
			good = acConvertToString(&acs->cursor, &value.c);
			break;
		default:
			/* 不支持的类型 */
			break;
		}
		format++;
		if (*format == ',')
		{
			format++;
		}
		if (!good)
		{
			return false;
		}
	}
	good = (acs->cursor == acs->streamEnd && (*format == ',' || *format == '\0'));
	acs->cursor = acs->streamBegin; /* 重置指针 */
	return good;
}

bool acAsGet(AcStream* acs, char type, void* output)
{
	if (!acAsMoveNext(acs))
	{
		return false;
	}
	switch (type)
	{
	case 'S':
		return acConvertToInt16(&acs->cursor, (int16*)output);
	case 'I':
		return acConvertToInt32(&acs->cursor, (int32*)output);
	case 'L':
		return acConvertToInt64(&acs->cursor, (int64*)output);
	case 'D':
		return acConvertToFloat64(&acs->cursor, (float64*)output);
	case 'C':
		return acConvertToString(&acs->cursor, (const char**)output);
	default:
		return false;
	}
}

//---------------------------------------------------------------------------------------
// AcStruct的构造函数
//---------------------------------------------------------------------------------------
bool acAsInit(AcStream* acs, char* params, CleanupFunc func, const char* format)
{
	char* p = params;
	for (; *p != '\0'; p++)
	{
		if (*p == ',')
		{
			*p = '\0';
		}
	}
	acs->streamBegin = params;
	acs->streamEnd = p + 1; /* '\0'后面一字节 */
	acs->cursor = params;
	acs->freer = func;
	bool good = acAsIsParameterRight(acs, format);
	return good;
}

//---------------------------------------------------------------------------------------
// AcStruct的析构函数
//---------------------------------------------------------------------------------------
void acAsUninit(AcStream* acs)
{
	if (acs->freer)
	{
		acs->freer(acs->streamBegin);
		acs->streamBegin = 0;
		acs->streamEnd = 0;
	}
}

//---------------------------------------------------------------------------------------
// AcStruct的复制构造函数
//---------------------------------------------------------------------------------------
AcStream* acAsCopyCreate(const AcStream* src)
{
	AcStream *temp = (AcStream*)MALLOC(sizeof(AcStream));
	if (!temp)
	{
		return 0;
	}
	int length = src->streamEnd - src->streamBegin;
	temp->streamBegin = (char*)MALLOC(length);
	MEMCPY(temp->streamBegin, length, src->streamBegin, length);
	temp->streamEnd = temp->streamBegin + length; /* '\0'后面一字节 */
	temp->cursor = temp->streamBegin;
	temp->freer = FREE;
	return temp;
}

void acRemoveExtraSpace(char* cmd)
{
	char* p = cmd;
	while (true)
	{
		while (acIsSpace(*p))
		{
			p++;
		}
		char* start = p;
		while (*p != '=' && *p != ',' && *p != '\0')
		{
			p++;
		}
		char* separator = p;
		for (p -= 1; acIsSpace(*p); p--)
		{
			/* 啥也不做 */;
		}
		char* end = p + 1;
		int length = end - start;
		MEMMOVE(cmd, length, start, length);
		cmd += length;
		*cmd = *separator;
		if (*separator == '\0')
		{
			break;
		}
		p = separator + 1;
		cmd++;
	}
}

char* acEatPrefix(char* cmd, const char* prefix)
{
	for (; *prefix != '\0' && *cmd != '\0' && toupper(*cmd) == *prefix; prefix++, cmd++)
	{
		/* 什么也不做 */;
	}
	if (*prefix == '\0')
	{
		return cmd;
	}
	return 0;
}

void acProcessCommand(char* cmd)
{
	if (acEcho)
	{
		acPrint(2, cmd, "\r\n");
	}
	acRemoveExtraSpace(cmd);
	bool execed = false;
	char* noAt = acEatPrefix(cmd, "AT");
	if (noAt)
	{
		for (int i = 0; i < sizeof(commands) / sizeof(AcCommand); i++)
		{
			char* right = acEatPrefix(noAt, commands[i].name);
			if (!right || !acIsSeperator(*right))
			{
				continue;
			}
			if (*right == '\0')
			{
				if (commands[i].exec)
				{
					commands[i].exec(0);
					execed = true;
				}
				break;
			}
			else if (*right == '?')
			{
				if (commands[i].read)
				{
					commands[i].read(0);
					execed = true;
				}
				break;
			}
			else if (*right == '=' && *(right + 1) == '?')
			{
				if (commands[i].test)
				{
					commands[i].test(0);
					execed = true;
				}
				break;
			}
			else if (*right == '=' && *(right + 1) != '\0')
			{
				char* params = right + 1;
				AcStream stream;
				bool good = acAsInit(&stream, params, 0, commands[i].format);
				if (good && commands[i].write)
				{
					commands[i].write(&stream);
					execed = true;
				}
				acAsUninit(&stream);
				break;
			}
		}
	}
	if (!execed)
	{
		acPrint(1, "\r\nERROR\r\n");
	}
}

void acThread()
{
	AcMessage msg;
	while (acGetMessage(&msg))
	{
		switch (msg.id)
		{
		case AT_COMMAND:
			acProcessCommand(msg.cmd);
			break;
		default:
			/* 其它的指令无法识别 */
			break;
		}
		if (msg.cmd)
		{
			FREE(msg.cmd);
		}
	}
}

