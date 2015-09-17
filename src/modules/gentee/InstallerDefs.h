#ifndef InstallerDefs_h__
#define InstallerDefs_h__

#define SIG_TEXT "Gentee Launcher"

#pragma pack(push, 1)

struct lahead_data
{
	uint32_t  exesize;
	uint32_t  minsize;
	uint8_t   exeext;
	uint8_t   pack;
	uint16_t  flags;
	uint32_t  dllsize;
	uint32_t  gesize;
	int64_t  extsize[8];
};

struct lahead_v1
{
	char      sign1[16];     // 'Gentee Launcher' sign
	uint32_t  exesize;       // ������ exe-�����. ���� �� 0, �� ���� �������� SFX ������� � ����� ���� ������������e ������
	uint32_t  minsize;       // ���� �� 0, �� �������� ������ ���� ������ ����� ������ ����������
	uint8_t   console;       // 1 ���� ���������� ����������
	uint8_t   exeext;        // ���������� �������������� ������
	uint16_t  flags;         // flags
	uint32_t  dllsize;       // ����������� ������ Dll �����. ���� 0, �� ������������ ����������� gentee.dll
	uint32_t  gesize;        // ����������� ������ ����-����.
	uint32_t  mutex;         // ID ��� mutex, ���� �� 0, �� ����� ��������
	uint32_t  param;         // ������� ��������
	uint32_t  offset;        // �������� ������ ���������
	int64_t  extsize[ 8 ]; // ���������������� ��� �������� 8 ext ������. ������ ������ �������� long
};

struct lahead_v2
{
	char      sign1[16];     // 'Gentee Launcher' sign
	uint32_t  exesize;       // ������ exe-�����. ���� �� 0, �� ���� �������� SFX ������� � ����� ���� ������������e ������
	uint32_t  minsize;       // ���� �� 0, �� �������� ������ ���� ������ ����� ������ ����������
	uint8_t   console;       // 1 ���� ���������� ����������
	uint8_t   exeext;        // ���������� �������������� ������
	uint8_t   pack;          // 1 ���� ���� ��� � dll ���������
	uint16_t  flags;         // flags
	uint32_t  dllsize;       // ����������� ������ Dll �����. ���� 0, �� ������������ ����������� gentee.dll
	uint32_t  gesize;        // ����������� ������ ����-����.
	uint32_t  mutex;         // ID ��� mutex, ���� �� 0, �� ����� ��������
	uint32_t  param;         // ������� ��������
	uint32_t  offset;        // �������� ������ ���������
	int64_t  extsize[ 8 ]; // ���������������� ��� �������� 8 ext ������. ������ ������ �������� long
};

#pragma pack(pop)

#endif // InstallerDefs_h__
