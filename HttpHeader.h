//
// Created by me on 2022/10/14.
//
#include <cstring>
#ifndef LAB1_HTTPHEADER_H
#define LAB1_HTTPHEADER_H


class HttpHeader {
    public:
        HttpHeader(){
            memset(this,0,sizeof(HttpHeader));
            //ZeroMemory(this,sizeof(HttpHeader));
        }
        char method[5]{}; // POST 或者 GET，注意有些为 CONNECT，本实验暂不考虑
        char url[1024]{}; // 请求的 url
        char host[1024]{}; // 目标主机
        char cookie[1024 * 10]{}; //cookie
};


#endif //LAB1_HTTPHEADER_H
