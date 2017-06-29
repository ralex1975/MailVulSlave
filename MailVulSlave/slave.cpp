// svchost.cpp : Defines the entry point for the console application.
//


#include "ClientSocket.h"
#include "Buffer.h"
#include <string.h>
#include "protocol.h"

CClientSocket cs;

void ErrorHandling(char *p)
{
	printf("%s\n", p);
	return ;
}

int main(int argc, char *argv[])
{
	
	char *p = "hello, it's a test!!!";
	CPacket packet(PACKET_COMMAND, HOST_HELLO, (LPBYTE)p, strlen(p));
	
	if(cs.Connect((LPCTSTR)"MailVulScanner", 6666) == false)
		ErrorHandling("Connect() error!!!");

	cs.Send(packet.getBuf(), packet.getLen());
	packet.ClearBuf();

	while(true)
	{
		printf("+++++++++++++++++++++++\n");
		Sleep(3000);
	}
		

	return 0;
}

