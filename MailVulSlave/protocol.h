/************************************************************************/
/* protocol.h                                                           */
/************************************************************************/
// ����һЩ�ͻ��˺ͷ�������ͨ�ŵ�һЩ�������

#if !defined(AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_PROTOCOL)
#define AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_PROTOCOL

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////
enum PacketType						// ������
{
	PACKET_COMMAND,					// ����
	PACKET_DATA,		
	PACKET_HEARTBEAT,
	PACKET_UPDATE,
	PACKET_RESEND					// �ش�
};


enum CommandType					// ��������
{
	HOST_UPDATE,					// ����
	HOST_CONNECT,					// ����
	HOST_REBOOT,					// ����
	HOST_SHUTDOWN,					// �ر�
	HOST_UNINSTALL,					// ж��
	HOST_INFO,						// ������Ϣ

	HOST_HELLO,						// ����ʹ��
	
	//////////////////////////////////////////////////////////////////////////
	// �ʼ�ϵͳ�����ʽ���壬 ������һ���
	EMAIL_xxx = 200,
	EMAIL_BRUTEFORCE,
	
	
	///////////////////////////////////////////////////////
	// DEEPWEBҵ��
	DEEPWEB_HBASE_PUT = 1113,
	DEEPWEB_HBASE_GET = 1114,
	
	DEEPWEB_HBASE_SCAN = 1117
};


// 
// �ͻ���-��������ͨ�����Э�鶨��
// ��ͷ��
//

#define HDR_SIZE 8				// 4|4  ���ݰ���ͷ������8���ֽ�
#define D_HDR_SIZE 8

typedef struct PacketHeader{				
	int packetType;					// ������
	int packetLen;					// ���ĳ���
} *LPPacketHeader;


// ����ͷ��
typedef struct DataHeader{					
	int commandType;				// ��������
	int dataLen;					// ����ͷ��
} *LPDataHeader;


//////////////////////////////////////////////////////////////
// PacketType | PacketLen | CommandType | DataLen | Data ...//
// ----- HDR_SIZE --------| ------ D_HDR_SIZE ----| DATA ...//
//////////////////////////////////////////////////////////////
class CPacket
{
private:
	PacketHeader packetHeader;
	DataHeader dataHeader;
	LPBYTE  pBuf;
	int m_nLen;

public:
	CPacket(){}

	//
	// ���캯����װ���ݰ�
	CPacket(int packetType, int commandType, PBYTE dataBuf, int nSize)
	{
		pBuf = new BYTE[nSize + HDR_SIZE + D_HDR_SIZE];						// ���󻺳���
		packetHeader.packetType = packetType;
		packetHeader.packetLen = nSize + D_HDR_SIZE;						// ���������	

		dataHeader.commandType = commandType;
		dataHeader.dataLen = nSize;											// ���ݳ���

        CopyMemory(pBuf, &packetHeader, HDR_SIZE);
		CopyMemory(pBuf + HDR_SIZE, &dataHeader, D_HDR_SIZE);
		CopyMemory(pBuf + HDR_SIZE + D_HDR_SIZE, dataBuf, nSize);
		
		m_nLen = nSize + HDR_SIZE + D_HDR_SIZE;
	}

	LPBYTE getBuf()						// ��ȡ���ݰ��ĵ�ַ
	{
		return pBuf;					
	}

	int getLen()						// ��ȡ���ݰ��ĳ���
	{
		return m_nLen;					
	}

	void ClearBuf()						// ���տռ�
	{
	   delete []pBuf;
	}
};


#endif AFX_IOCPSERVER_H__75B80E90_FD25_4FFB_B273_0090AA43BBDF__INCLUDED_PROTOCOL