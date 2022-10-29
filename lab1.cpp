#include "HttpProxy.h"
#include "HttpHeader.h"

//Http 重要头部数据
bool InitWinSock();
//代理端口号
const int ProxyPort = 8080;
int main() {
    printf("代理服务器正在启动\n");
    printf("初始化...\n");
    if (!InitWinSock()) {
        printf("WinSock加载失败\n");
        return -1;
    }
    HttpProxy httpProxy(AF_INET, SOCK_STREAM, 0, ProxyPort);
    httpProxy.Listening();
    return 0;
}
bool InitWinSock() {
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    //版本 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //加载 dll 文件 Socket 库
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
    //找不到 winsock.dll
        printf("加载 winsock 失败，错误代码为: %d\n", WSAGetLastError());
        return FALSE;
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        printf("不能找到正确的 winsock 版本\n");
        WSACleanup();
        return FALSE;
    }
    return true;
}