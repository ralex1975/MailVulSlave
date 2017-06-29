/************************************************************************/
/* protocol.h                                                           */
/************************************************************************/
// 定义一些客户端和服务器端通信的一些基本相关

#if !defined(AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_PROTOCOL)
#define AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_PROTOCOL

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

////////////////////////////////////////////////////////////////////
enum PacketType						// 包类型
{
	PACKET_COMMAND,					// 命令
	PACKET_DATA,		
	PACKET_HEARTBEAT,
	PACKET_UPDATE,
	PACKET_RESEND					// 重传
};


enum CommandType					// 命令类型
{
	HOST_UPDATE,					// 更新
	HOST_CONNECT,					// 连接
	HOST_REBOOT,					// 重启
	HOST_SHUTDOWN,					// 关闭
	HOST_UNINSTALL,					// 卸载
	HOST_INFO,						// 主机信息

	HOST_HELLO,						// 测试使用
	
	//////////////////////////////////////////////////////////////////////////
	// 邮件系统命令格式定义， 先揉在一起吧
	EMAIL_xxx = 200,
	EMAIL_BRUTEFORCE,
	
	
	///////////////////////////////////////////////////////
	// DEEPWEB业务
	DEEPWEB_HBASE_PUT = 1113,
	DEEPWEB_HBASE_GET = 1114,
	
	DEEPWEB_HBASE_SCAN = 1117
};


// 
// 客户端-服务器端通信相关协议定义
// 包头部
//

#define HDR_SIZE 8				// 4|4  数据包的头部长度8个字节
#define D_HDR_SIZE 8

typedef struct PacketHeader{				
	int packetType;					// 包类型
	int packetLen;					// 包的长度
} *LPPacketHeader;


// 数据头部
typedef struct DataHeader{					
	int commandType;				// 命令类型
	int dataLen;					// 数据头部
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
	// 构造函数封装数据包
	CPacket(int packetType, int commandType, PBYTE dataBuf, int nSize)
	{
		pBuf = new BYTE[nSize + HDR_SIZE + D_HDR_SIZE];						// 请求缓冲区
		packetHeader.packetType = packetType;
		packetHeader.packetLen = nSize + D_HDR_SIZE;						// 命令包长度	

		dataHeader.commandType = commandType;
		dataHeader.dataLen = nSize;											// 数据长度

        CopyMemory(pBuf, &packetHeader, HDR_SIZE);
		CopyMemory(pBuf + HDR_SIZE, &dataHeader, D_HDR_SIZE);
		CopyMemory(pBuf + HDR_SIZE + D_HDR_SIZE, dataBuf, nSize);
		
		m_nLen = nSize + HDR_SIZE + D_HDR_SIZE;
	}

	LPBYTE getBuf()						// 获取数据包的地址
	{
		return pBuf;					
	}

	int getLen()						// 获取数据包的长度
	{
		return m_nLen;					
	}

	void ClearBuf()						// 回收空间
	{
	   delete []pBuf;
	}
};


#endif AFX_IOCPSERVER_H__75B80E90_FD25_4FFB_B273_0090AA43BBDF__INCLUDED_PROTOCOL