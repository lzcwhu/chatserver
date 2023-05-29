#include "chatserver.h"
#include <functional>
#include "json.hpp"
#include <chatservice.hpp>
using namespace std;
using namespace placeholders; //参数占位符
using json = nlohmann::json;


ChatServer::ChatServer(EventLoop* loop, const InetAddress& listenAddr, const string& nameArg):_server(loop,
    listenAddr, nameArg), _loop(loop){
        //注册链接回调函数
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
        //注册消息回调函数
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        //设置线程数量
        _server.setThreadNum(4);
}

void ChatServer::start(){
    _server.start();
}

//回调函数，上报链接相关信息
void ChatServer::onConnection(const TcpConnectionPtr &conn){
    //如果连接失败,客户断开连接
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);//客户异常断连
        conn->shutdown();
    }
}
    //回调函数，上报读写相关信息
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer* buffer, Timestamp time){
    string buff = buffer->retrieveAllAsString();
    json js = json::parse(buff); //反序列化
    //通过js["msgid"]获取业务处理器
    //完全解耦网络模块代码和业务模块代码，通过回调的思想
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());//js["msgid"]仍是json类型
    msgHandler(conn, js, time);
}