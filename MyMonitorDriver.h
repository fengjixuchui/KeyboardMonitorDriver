/*
*******************************************************************************
*= = 文件名称：MyMonitorDriver.h
*= = 文件描述：关于MyMonitorDriver的头文件
*= = 作    者：胡默迪小组
*= = 编写时间：2016-07-09 19:18:00
*******************************************************************************
*/

#ifndef _MYMONITORDRIVER_H__
#define _MYMONITORDRIVER_H__

//*============================================================================ 
//*= = 头文件声明 
//*============================================================================ 

#include <NTDDK.h>
#include <WDM.h>
#include <stdlib.h>
#include <stdio.h>

#include "keycode.h"
#include "MyInterruptProc.h"

//*============================================================================ 
//*= = 宏与结构体 
//*============================================================================ 

//若存在此句，则本程序编译为替换INT0x71，否则编译为IOAPIC重定位做法。
//#define BUILD_FOR_IDT_HOOK

//预先定义长度变量，避免不同环境下编译出错.
typedef unsigned char UCHAR_8;
typedef unsigned short USHORT_16;
typedef unsigned long ULONG_16;

#define P2C_MAKELONG(low, high) \
	((ULONG_16)(((USHORT_16)((ULONG_16)(low)& 0xffff)) | ((ULONG_16)((USHORT_16)((ULONG_16)(high)& 0xffff))) << 16))

#define P2C_LOW16_OF_32(data) \
	((USHORT_16)(((ULONG_16)data) & 0xffff))

#define P2C_HIGH16_OF_32(data) \
	((USHORT_16)(((ULONG_16)data) >> 16))

#define OBUFFER_FULL 0x02
#define IBUFFER_FULL 0x01
#define LOG_PRE_SIZE 128
#define IRECORD_SIZE 32

//*============================================================================ 
//*= = 全局变量 
//*============================================================================ 

BOOLEAN UNLOAD_FLAG;
PKTIMER timer;                  //注册定时器
//KEVENT event;					//注册事件
PVOID RecordMemory;				//申请内存
void *Origin_address;				//程序自带的处理函数地址

//*============================================================================ 
//*= = 函数声明 
//*============================================================================

// 从sidt指令获得如下结构，可以从中获知IDT的开始地址
#pragma pack(push,1)
typedef struct P2C_IDTR_ {
	USHORT_16 limit;				// 范围
	ULONG_16 base;				// 基地址（就是开始地址）
} P2C_IDTR, *PP2C_IDTR;
#pragma pack(pop)

#pragma pack(push,1)
typedef struct P2C_IDT_ENTRY_ {
	USHORT_16 offset_low;
	USHORT_16 selector;
	UCHAR_8 reserved;
	UCHAR_8 type : 4;
	UCHAR_8 always0 : 1;
	UCHAR_8 dpl : 2;
	UCHAR_8 present : 1;
	USHORT_16 offset_high;
} P2C_IDTENTRY, *PP2C_IDTENTRY;
#pragma pack(pop)

VOID RecordThread();
void HookInt71(BOOLEAN hook_or_unhook);
UCHAR_8 GetIdleIdtVec();
UCHAR_8 CopyIdt71(UCHAR_8 id, void *interrupt_proc);
UCHAR_8 SeachOrSetIrq1(UCHAR_8 new_ch);
void ResetIoApic(BOOLEAN set_or_recovery);
void Write_to_file(char* file, char* str);
void DriverUnload(PDRIVER_OBJECT drv);


void zwfile_write(char* file, char* str);
VOID RecordThread();
//int RtlStringCbPrintfA();
//void getsystime(char*  buff);

#endif // End of _MYMONITORDRIVER_H_

//*============================================================================ 
//*= = 文件结束 
//*============================================================================ 