//
// Created by me on 2022/10/14.
//
#include <cstdio>
#include <Windows.h>
#include <process.h>
#include "HttpHeader.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <tchar.h>
#include <set>
#include <fstream>
#include <filesystem>
#include "HttpHeader.h"
#include "ProxyServerSocket.h"
#pragma comment(lib,"Ws2_32.lib")
#define cachepath ".\\cacheFile"
#define HTTP_PORT 80 //http 服务器端口
#include <thread>
#ifndef LAB1_HTTPPROXY_H
#define LAB1_HTTPPROXY_H
struct ProxyParam {
    SOCKET clientSocket;
    SOCKET serverSocket;
};
class HttpProxy {
public:
    ProxyServerSocket ProxySever;
    HttpProxy(int af,int type,int protocol,int proxyport): ProxySever(af,type,protocol,proxyport){
        this->ProxySever.setProxyAddr(AF_INET, INADDR_ANY);
        this->ProxySever.listenclient();
        printf("代理服务器正在运行，监听端口 %d\n", proxyport);
    }
    [[noreturn]] void Listening() const;
    static void ParseHttpHead(char *buffer, HttpHeader * httpHeader);
    const static int cacheSize=50;
    static void cache(ProxyParam *proxyParam, HttpHeader *httpHeader, const char* request_buffer);
    static std::string checkCacheLeft(bool& delflag);
    static DWORD WINAPI Proxythread(void *lpproxypram);
    static bool websitefilter(HttpHeader *httpHeader);
};
#endif //LAB1_HTTPPROXY_H



