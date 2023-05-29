#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include "groupmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendverify.hpp"
#include <functional>
#include "frinedmodel.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include "usermodel.hpp"
#include <mutex>
#include <redis.hpp>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
//消息id对于的事件回调
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器服务类
class ChatService{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //登陆方法
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //注册方法
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //一对一聊天方法
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);



    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr&);

    //处理服务器异常退出
    void reset();

    //添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //加入群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理注销业务
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //处理好友验证业务
    void verifyFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    //从redis消息队列中获取订阅消息
    void handleRedisSubscribeMessage(int userid, string msg);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
private:
    ChatService();
    //消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //数据操作类 对象
    UserModel _userModel;

    OfflineMsgmodel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    FriendVerify _friendverifyModel;


    //存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnectionMap;

    //定义互斥锁，保证_userConnectionMap的线程安全
    mutex _connMutex;



    //redis操作对象
    Redis _redis;
};

#endif