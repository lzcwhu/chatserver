#include "chatserver.h"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>
using namespace std;

//处理服务器异常退出
void resetHandler(int){
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char** argv){
    if(argc < 3){
        cerr << "command error! example:127.0.0.1 6000" << endl;
        exit(-1);
    }
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]);

    signal(SIGINT, resetHandler);//异常退出执行resetHandler
    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();
    return 0;
}