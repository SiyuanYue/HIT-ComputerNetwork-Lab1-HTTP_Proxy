//
// Created by me on 2022/10/14.
//

#include <cassert>
#include "HttpProxy.h"
namespace fs = std::filesystem;
std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value);
static BOOL ConnectToServer(SOCKET *serverSocket, char *host);
static std::set<std::string> UserFilters{};
static std::set<std::string> WebFilters{};
static std::unordered_map<std::string,std::string> FishFilters{};
static void setfish()
{
    FishFilters["www.hit.edu.cn"]="oa.hit.edu.cn";
    FishFilters["jwts.hit.edu.cn"]="news.hit.edu.cn";
}
static void setFilters()
{
    WebFilters.insert("today.hit.edu.cn");
}
[[noreturn]] void HttpProxy::Listening() const {
    ProxyParam *lpProxyParam;
    SOCKET clientSocket;
    while (true) {
        clientSocket = accept(this->ProxySever.proxyseversocket, nullptr, nullptr);
        lpProxyParam = new ProxyParam();
        lpProxyParam->clientSocket = clientSocket;
        //新建线程
        HANDLE createThread = CreateThread(
                nullptr,
                0,
                Proxythread,
                (void *) lpProxyParam,
                0, //创建后立即运行
                nullptr);
        CloseHandle(createThread);
        Sleep(100);
    }
    closesocket(this->ProxySever.proxyseversocket);
    WSACleanup();
}

DWORD WINAPI HttpProxy::Proxythread(void *lpproxypram) {
    //TODO cache 网络过滤与引导功能
    setfish();
    setFilters();
    char buffer[65536];
    memset(buffer, 0, 65536);
    auto *tlpproxypram = (ProxyParam *) lpproxypram;
    int recvSize = recv(tlpproxypram->clientSocket, buffer, 65535, 0);
    if (recvSize < 0) {
        std::cout << "recv client len <0" << std::endl;
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        _endthreadex(1);
    }
    std::string buffer2(buffer);
    auto *http_header = new HttpHeader();
    HttpProxy::ParseHttpHead((char *) buffer2.c_str(), http_header);
    if(websitefilter(http_header))
    {
        printf("关闭套接字\n");
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        delete tlpproxypram;
        _endthreadex(1);
    }
    //FISHING
    std::string rebuffer(buffer);
    //std::cout<<FishFilters.contains(std::string(http_header->host))<<"  strlen(host)= "<<strlen(http_header->host)<<std::endl;
    if(FishFilters.contains(http_header->host))
    {
        std::string oldhost(http_header->host);
        std::string newhost(FishFilters[http_header->host]);
        strcpy(http_header->host,newhost.c_str());
        replace_all(rebuffer,oldhost,newhost);
    }
    if (!ConnectToServer(&tlpproxypram->serverSocket, http_header->host)) {
        printf("关闭套接字\n");
        closesocket(tlpproxypram->clientSocket);
        closesocket(tlpproxypram->serverSocket);
        delete tlpproxypram;
        _endthreadex(1);
    }
    printf("代理连接主机 %s 成功\n", http_header->host);
    HttpProxy::cache(tlpproxypram,http_header,rebuffer.c_str());
////将客户端发送的 HTTP 数据报文直接转发给目标服务器
//    send(tlpproxypram->serverSocket, buffer, (int)strlen(buffer) + 1, 0);
////等待目标服务器返回数据
//    recvSize = recv(tlpproxypram->serverSocket, buffer, 65536, 0);
//    if (recvSize < 0) {
//        goto error;
//    }
////将目标服务器返回的数据直接转发给客户端
//    send(tlpproxypram->clientSocket, buffer, recvSize, 0);//recvSize!!!
////错误处理
//    error:
    printf("关闭套接字\n");
    Sleep(100);
    closesocket(tlpproxypram->clientSocket);
    closesocket(tlpproxypram->serverSocket);
    delete tlpproxypram;
    _endthreadex(0);
}

void HttpProxy::ParseHttpHead(char *buffer, HttpHeader *httpHeader) {
    char *p;
    char *ptr;
    const char *delim = "\r\n";
    p = strtok_s(buffer, delim, &ptr);//提取第一行
    if(p== nullptr)
    {
        return;
    }
    //printf("%s\n", p);
    if (p[0] == 'G') {//GET 方式
        memcpy(httpHeader->method, "GET", 3);
        memcpy(httpHeader->url, &p[4], strlen(p) - 13);
    } else if (p[0] == 'P') {//POST 方式
        memcpy(httpHeader->method, "POST", 4);
        memcpy(httpHeader->url, &p[5], strlen(p) - 14);
    }
    //printf("%s\n", httpHeader->url);
    p = strtok_s(nullptr, delim, &ptr);
    while (p) {
        switch (p[0]) {
            case 'H'://Host
                memcpy(httpHeader->host, &p[6], strlen(p) - 6);
                break;
            case 'C'://Cookie
                if (strlen(p) > 8) {
                    char header[8];
                    ZeroMemory(header, sizeof(header));
                    memcpy(header, p, 6);
                    if (!strcmp(header, "Cookie")) {
                        memcpy(httpHeader->cookie, &p[8], strlen(p) - 8);
                    }
                }
                break;
            default:
                break;
        }
        p = strtok_s(nullptr, delim, &ptr);
    }
}

void HttpProxy::cache(ProxyParam *proxyParam, HttpHeader *httpHeader, const char *request_buffer) {
    //std::cout<<"cache:"<<std::endl;
    //std::cout<<absolute(fs::path("."))<<std::endl;
    fs::path Cachehostpath(cachepath+std::string("\\")+ std::string(httpHeader->host));
    if (!fs::exists(Cachehostpath)) {
        if (!fs::create_directory(Cachehostpath)) {
            std::cout << "当前host的缓存文件夹创建失败" << std::endl;
        }
    }
    auto file_cut = std::to_string(std::hash<std::string>{}(std::string(httpHeader->url)));
    auto filename = cachepath +std::string ("\\")+ std::string(httpHeader->host)+std::string("\\") + file_cut +".txt";
    std::ofstream fileout;
    if (!fs::exists(fs::path(filename)))//文件不存在
    {
        //判满
//        bool delflag;
//        std::string deletefile=checkCacheLeft(delflag);
//        if(delflag)
//            std::cout<< deletefile <<"被删除"<<std::endl;
        // 向服务器转发原请求
        send(proxyParam->serverSocket, request_buffer, (int)strlen(request_buffer) + 1, 0);
        char buffer[65536];char cachebuffer[65536];ZeroMemory(buffer,65536);ZeroMemory(cachebuffer,65536);
        int recvSize = recv(proxyParam->serverSocket, buffer, 65535, 0);
        if (recvSize < 0||recvSize >65535 ) {
            std::cout << "接受size<0,错误" << std::endl;
            _endthreadex(0);
        }
        memcpy(cachebuffer,buffer,recvSize);
        send(proxyParam->clientSocket, buffer, recvSize, 0);
        fileout.open(filename,std::ios::binary|std::ios::out);
        //printf("%s",cachebuffer);
        //BUG!!!!!
        if(fileout){
            fileout.write(cachebuffer,recvSize);
        }else{
            std::cout<<"文件打开失败"<<std::endl;
        }
        fileout.close();
        std::cout<<"新建缓存文件: "<<filename<<std::endl;
    }
    else//已存在，判断是否需要更新
    {
        auto ft =fs::last_write_time(fs::path(filename) ) ;
        std::time_t t= std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(ft));
        char* asctime=std::asctime(std::localtime(&t));
        std::istringstream its(asctime);
        std::string dayname,month,day,minutes,year;
        its>>dayname>>month>>day>>minutes>>year;
        std::ostringstream fmt;
        fmt<<"\r\nIf-Modified-Since: "<<dayname<<", "<<day<<", "<<month<<", "<<year<<", "<<minutes<<" GMT\r\n";
        std::string gram=fmt.str();
        send(proxyParam->serverSocket,gram.c_str(),(int)gram.size(),0);
        char buffer[65536];
        ZeroMemory(buffer,65536);
        int recvSize=recv(proxyParam->serverSocket,buffer,65535,0);
        if (recvSize < 0) {
            std::cout << "接受size<0,错误" << std::endl;
            closesocket(proxyParam->clientSocket);
            closesocket(proxyParam->serverSocket);
            delete proxyParam;
            _endthreadex(0);
        }
        std::string resgram(buffer);
        //判断是否含304 缓存可用
        char *ptr,*phead;//printf("%s\r\n",buffer);
        phead=strtok_s(buffer, "\r\n", &ptr);
        std::string _phead(phead);
        if(_phead.find("304"))
        {//cache hit
            std::cout<<"cache hit"<<filename<<std::endl;
            std::ifstream cachefilestram(filename,std::ios::binary);
            if(!cachefilestram.is_open())
            {
                std::cout << "缓存读取失败，无法代开文件流" << std::endl;
                closesocket(proxyParam->clientSocket);
                closesocket(proxyParam->serverSocket);
                delete proxyParam;
                _endthreadex(0);
            }
            std::stringstream res;
            res<<cachefilestram.rdbuf();
            std::string resstr(res.str());
            cachefilestram.close();
            send(proxyParam->clientSocket,resstr.c_str(),(int)strlen(resstr.c_str()),0);
        }else if(_phead.find("200"))
        {//cache update
            std::cout<<"cache update"<<filename<<std::endl;
            send(proxyParam->clientSocket,resgram.c_str(),(int)resgram.size(),0);//转发
            std::ofstream cachefilestram(filename,std::ios::binary|std::ios::out);
            if(!cachefilestram.is_open()) {
                std::cout << "缓存更新失败，无法代开文件流" << std::endl;
                closesocket(proxyParam->clientSocket);
                closesocket(proxyParam->serverSocket);
                delete proxyParam;
                _endthreadex(0);
            }
            cachefilestram<<resgram;
            cachefilestram.close();
        }
    }
}

std::string& replace_all(std::string& src, const std::string& old_value, const std::string& new_value) {
    // 每次重新定位起始位置，防止上轮替换后的字符串形成新的old_value
    for (std::string::size_type pos(0); pos != std::string::npos; pos += new_value.length()) {
        if ((pos = src.find(old_value, pos)) != std::string::npos) {
            src.replace(pos, old_value.length(), new_value);
        }
        else break;
    }
    return src;
}

bool HttpProxy::websitefilter(HttpHeader *httpHeader){
    if(WebFilters.contains(httpHeader->host))
    {
        std::cout<<"网站过滤：不允许访问"<<httpHeader->host<<std::endl;
        return true;
    }
//    if(){
//
//    }
    return false;
}


std::string HttpProxy::checkCacheLeft(bool& delflag) {
    std::vector<fs::path> filelist;
    delflag=false;
    for(auto& p: fs::directory_iterator(fs::path(cachepath)))//获得基目录下目录列表
    {
        if (fs::directory_entry(p).status().type()== fs::file_type::directory)
            filelist.push_back(p.path());
        else
            std::cout<<"checkCacheLeft wrong"<<std::endl;
    }
    if(filelist.empty())
        return "";
    int count=0;
    std::time_t todeletetime=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    fs::directory_entry toDelfile;
    for(auto &str:filelist)//对每个host目录
    {
        if(!str.string().starts_with("."))
        {
            for(auto &file_name : fs::directory_iterator(str))
            {
                count++;
                std::time_t fileLastmodifytime=std::chrono::system_clock::to_time_t(std::chrono::file_clock::to_sys(fs::last_write_time(file_name)));
                if(fileLastmodifytime<todeletetime)
                {
                    todeletetime=fileLastmodifytime;
                    toDelfile=file_name;
                    delflag=true;
                }
            }
        }
    }
    if(count < cacheSize )
    {
        delflag=false;
        return "";
    }
    else if(delflag) // >=cachesize &&delflag
    {
        if(!fs::remove(toDelfile))
            std::cout<<"删除失败"<<std::endl;
        return toDelfile.path().string();
    }
    return "";
}

BOOL ConnectToServer(SOCKET *serverSocket, char *host) {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(HTTP_PORT);
    HOSTENT *hostent = gethostbyname(host);//DNS?
    if (!hostent) {
        return FALSE;
    }
    in_addr Inaddr = *((in_addr *) *hostent->h_addr_list);
    serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
    *serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*serverSocket == INVALID_SOCKET) {
        return FALSE;
    }
    if (connect(*serverSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr))
        == SOCKET_ERROR) {
        closesocket(*serverSocket);
        return FALSE;
    }
    return TRUE;
}
