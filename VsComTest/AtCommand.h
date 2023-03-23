#pragma once

#define MALLOC malloc
#define FREE free

#ifdef __cplusplus
#define STRCPY(dst, length, src) strcpy_s((dst), (length), (src))
#define MEMCPY(dst, dstLength, src, srcLength) memcpy_s((dst), (dstLength), (src), (srcLength))
#define MEMMOVE(dst, dstLength, src, srcLength) memmove_s((dst), (dstLength), (src), (srcLength))
#else
#define STRCPY(dst, length, src) strcpy((dst), (src))
#define MEMCPY(dst, dstLength, src, srcLength) memcpy((dst), (src), (srcLength))
#define MEMMOVE(dst, dstLength, src, srcLength) memmove_s((dst), (src), (srcLength))
#endif // __cplusplus

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef char int8;
typedef short int16;
typedef int int32;
typedef long long int64;
typedef float float32;
typedef double float64;

//#define INT16_MIN ((int16)(1 << 15))
//#define INT16_MAX ((int16)~(1 << 15))
//#define INT32_MIN ((int32)(1 << 31))
//#define INT32_MAX ((int32)~(1 << 31))
//#define INT64_MIN ((int64)(1ULL << 63))
//#define INT64_MAX ((int64)~(1ULL << 63))

typedef void(*CleanupFunc)(void*);

enum AcMessageId
{
	QUIT,
	AT_COMMAND,
};

typedef struct
{
	AcMessageId id;
	char* cmd;
} AcMessage;

typedef struct 
{
	char* streamBegin;
	const char* streamEnd;
	CleanupFunc freer;
	const char* cursor;
} AcStream;

typedef void(*CommandProcessor)(AcStream* params);

typedef struct
{
	const char* name;
	const char* format;
	CommandProcessor read;
	CommandProcessor write;
	CommandProcessor test;
	CommandProcessor exec;
} AcCommand;

void acThread();

void acPostMessage(const AcMessage *msg);

bool acAsGet(AcStream* acs, char type, void* output);

bool acAsInit(AcStream* acs, char* params, CleanupFunc func, const char* format);

AcStream* acAsCopyCreate(const AcStream* src);

void acAsUninit(AcStream* acs);

AcStream* acAsCopyCreate(const AcStream* src);

