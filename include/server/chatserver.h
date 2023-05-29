#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;


//主类
class ChatServer{
private:
    TcpServer _server; //组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop; //事件循环的指针
    //回调函数，上报链接相关信息
    void onConnection(const TcpConnectionPtr &);
    //回调函数，上报读写相关信息
    void onMessage(const TcpConnectionPtr &, Buffer*, Timestamp);
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg);
    //启动服务
    void start();


};

#endif
