#if !defined(AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_SMTP_BRUTEFORCE)
#define AFX_BUFFER_H__829F6693_AC4D_11D2_8C37_00600877E420__INCLUDED_SMTP_BRUTEFORCE
char send_wait(char *, char *, char);		// 函数声明:发送数据并等待接受响应码
void usage();								// 程序使用说明
void initialsocket();						// 重置连接
int smtp_bruteforce();
#endif