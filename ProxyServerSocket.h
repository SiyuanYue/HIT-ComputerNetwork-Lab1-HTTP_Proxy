//
// Created by me on 2022/10/14.
//
#pragma once
#include <Windows.h>
#include <cstdio>

//#pragma comment(lib,"Ws2_32.lib")

#ifndef LAB1_PROXYSERVERSOCKET_H
#define LAB1_PROXYSERVERSOCKET_H


class ProxyServerSocket {
public:
    SOCKET proxyseversocket;
    sockaddr_in ProxySeverAddr{};
    const int proxyport;
    ProxyServerSocket(int af,int type,int protocol,int ProxyPort):proxyport(ProxyPort){
        proxyseversocket=socket(af,type,protocol);
        if(INVALID_SOCKET == proxyseversocket){
            printf("创建套接字失败，错误代码为：%d\n",WSAGetLastError());
            exit(1);
        }
    }
    bool setProxyAddr(short sinfamily,short addrs);
    void listenclient() const;
};


#endif //LAB1_PROXYSERVERSOCKET_H
