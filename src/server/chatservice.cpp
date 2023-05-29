#include "chatservice.hpp"
#include "public.hpp"
#include <functional>
#include <vector>
#include <iostream>
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace std;
//获取单例对象的接口函数
ChatService* ChatService::instance(){
    static ChatService service;
    return &service;
}

//注册消息以及对应的回调操作
ChatService::ChatService(){
    //用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({VERIFY_FRIEND_MSG, std::bind(&ChatService::verifyFriend, this, _1,_2,_3)});

    //群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATG_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});


    //连接redis服务器
    if(_redis.connect()){
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this,_1, _2));
    }
}

//获取消息对应的处理函数
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()){
        return [=] (const TcpConnectionPtr &conn, json &js, Timestamp){
            LOG_ERROR << "msgid: " << msgid << " cannot find handler!";
        };
        
    }
    else{
        return _msgHandlerMap[msgid];
    }
    return _msgHandlerMap[msgid];
}

//服务器异常，业务重置
void ChatService :: reset(){
    //online用户设置为offline
    _userModel.resetState();
}

//业务层和数据层分离，业务层操作的都是对象
    //登陆方法，填id,密码
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user  = _userModel.query(id);

    if(user.getId()== id && user.getPwd() == pwd){
        
        if(user.getState() == "online"){
            //已登录用户不允许再次登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2; //为0表示成功
            response["errmsg"] = "this id is using!";
            conn->send(response.dump());
        }
        else{
            //登陆成功,更新状态信息
            user.setState("online");
            _userModel.updateState(user);
            //记录用户连接信息
            //由于用户连接信息表需要工作在多线程，所以需要加上互斥操作

            //{}限制互斥锁的作用域
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnectionMap.insert({id,conn});
            }

            //id用户登录后，向redis订阅channel(id)
            _redis.subscribe(id);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["id"] = user.getId();
            response["errno"] = 0; //为0表示成功
            response["name"] = user.getName();
            //登录成功需要看离线消息表中也没有相关的离线消息
            vector<string> vec = _offlineMsgModel.query(user.getId());
            if(!vec.empty()){
                response["offlinemsg"] = vec;//json库和vector自动匹配序列化
                _offlineMsgModel.remove(id);//离线消息发送后删除
            }

            //查看是否有好友验证消息
            vector<int> verifyVec = _friendverifyModel.query(user.getId());
            if(!verifyVec.empty()){
                response["friendverify"] = verifyVec;
            }
            
            //查询该用户好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if(!userVec.empty()){
                //
                vector<string> vec2;
                for(User &user : userVec){
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            //查询用户群信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if(!groupuserVec.empty()){
                vector<string> groupV;
                for(Group &group : groupuserVec){
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user : group.getUsers()){
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());

                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }

    }
    else{
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1; //为0表示成功
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}
//注册方法,填name和password即可
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time){
    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPwd(password);
    bool state = _userModel.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 0; //为0表示成功
        conn->send(response.dump());
    }
    else{
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["id"] = user.getId();
        response["errno"] = 1; //为1表示失败
        conn->send(response.dump());

    }
}


//处理注销业务
void ChatService::logout(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnectionMap.find(userid);
        if(it != _userConnectionMap.end()){
            _userConnectionMap.erase(it);
        }
    }

    //用户注销，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户状态
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
    
}

//处理用户异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        
        for(auto it = _userConnectionMap.begin(); it != _userConnectionMap.end(); ++it){
            if(it->second == conn){
                user.setId(it->first);
                _userConnectionMap.erase(it);//从map表中删除连接信息
                break;
            }
        }
        
    }
    
    //用户注销，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());
    // 更新用户状态信息
    if(user.getId() != -1){
        user.setState("offline");
        _userModel.updateState(user);
    }
    
}

//一对一聊天业务
void ChatService :: oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int toid = js["toid"].get<int>();//消息发送对象的id

    //这里验证toid是否注册过
    User user  = _userModel.query(toid);
    json response;

    //如果toid不存在(其实只验证是否是用户好友就够了，这里的验证其实多此一举了)
    if(user.getId() == -1){
        response["msg"] = "toid has not registered!";
        conn->send(response.dump());
        return;
    }
    
    //验证toid是否是用户的好友
    int userid = js["id"].get<int>();
    vector<User> friendlist = _friendModel.query(userid);
    for(auto &f : friendlist){
        if(toid == f.getId()){
            //在好友列表中找到了toid
            {
                lock_guard<mutex> lock(_connMutex);
                auto it = _userConnectionMap.find(toid);
                if(it != _userConnectionMap.end()){
                    //toid在线，服务器主动推送消息给用户
                    it->second->send(js.dump());
                    return;
                }
            }
            //查询toid是否在线
            user = _userModel.query(toid);
            if(user.getState()=="online"){
                _redis.publish(toid, js.dump());//用户在其他服务器上，向redis发布消息
                return;
            }


            //toid不在线时，存储离线消息
            _offlineMsgModel.insert(toid, js.dump());

            response["msg"] = "message sent seccessfully!";
            conn->send(response.dump());

            return;
        }
    }

    //好友列表中没找到toid
    response["msg"] = "this toid is not your friend!";
    conn->send(response.dump());
    
   
}

//添加好友业务
void ChatService :: addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    
    User user  = _userModel.query(friendid);
    //如果friendid不在用户列表里，则返回提醒
    if(user.getId()== -1) {
        json response;
        response["msg"] = "this id is not exist!";
        response["errno"] = 1;
        conn->send(response.dump());
        return;
    }
    //如果已经有该好友了就不用进行申请,返回提醒
    vector<User> friendlist = _friendModel.query(userid);
    for(auto &f : friendlist){
        if(friendid == f.getId()){
            json response;
            response["msg"] ="this id is alreadly in your friendlist, do not add it repeatedly!";
            
            conn->send(response.dump());
            return;
        }
    }

    json response;
    response["msg"] ="send friendverify sucess! Waiting for agreement!";
    
    conn->send(response.dump());
    //存储好友验证信息
    //这里暂时不进行添加好友，而是等好友同意(当前用户，要加的好友)
    _friendverifyModel.insert(userid, friendid);

    //这里需要用户给要加的好友发送好友申请的信息（借用onechat模块中的代码）
    json js1;
    char msg[30];
    sprintf(msg, "%d want to be your friend.",userid);
    js1["toid"] = friendid;
    js1["msg"] = msg;
    js1["id"] = userid;//当前用户id
    js1["time"] = js["time"];

    //不能直接调用oneChat函数，因为这样会连续两次只send！！！
    // oneChat(conn, js1, time);
    
    //查询toid是否在线
    user = _userModel.query(friendid);
    if(user.getState()=="online"){
        _redis.publish(friendid, js1.dump());//用户在其他服务器上，向redis发布消息
        return;
    }
    //toid不在线时，存储离线消息
    _offlineMsgModel.insert(friendid, js1.dump());

}

void ChatService :: verifyFriend(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"];
    int friendid = js["friendid"];
    //这里verifyVec是userid的所有好友申请
    json response;
    vector<int> verifyVec = _friendverifyModel.query(userid);
    if(verifyVec.empty()){
        response["errno"] = 1;
        response["msg"] = "the friendid isnot exist!";
        conn->send(response.dump());
        return;
    }
    auto it =  find(verifyVec.begin(), verifyVec.end(), friendid);
    
    if(it == verifyVec.end()){
        response["errno"] = 1;
        response["msg"] = "the friendid isnot exist!";
        conn->send(response.dump());
        return;
    }
    _friendverifyModel.remove(friendid, userid);//将好友id移出该用户好友申请
    // _friendverifyModel.remove(userid, friendid);//
    _friendModel.insert(userid, friendid);//将好友id加入该用户好友列表
    _friendModel.insert(friendid, userid);//好友是双向的
    response["errno"] = 0;
    response["msg"] = "success!";
    conn->send(response.dump());
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    //创建群组，id是自增的，不用验证是否已经存在
    string name = js["groupname"];
    string desc = js["groupdesc"];
    //存储xin创建的群组信息
    Group group(-1,name,desc);
    if(_groupModel.createGroup(group)){
        //存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    //创建json用于反馈
    json response;

    //先查询一下groupid是否存在,如果存在，该群里是否已经存在该用户
    if(_groupModel.query(userid, groupid)){
        _groupModel.addGroup(userid, groupid, "normal");
        response["msg"] = "addgroup success!";
        conn->send(response.dump());
        return;
    }
    response["msg"] = "the groupid is not exist or userid is alreadly in group!";
    conn->send(response.dump());
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time){
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    
    json response;
    if(useridVec.empty()){
        response["msg"] = "groupid invalid!";
        conn->send(response.dump());
        return;
    }

    lock_guard<mutex> lock(_connMutex);//在操作_userConnMap时需要加锁

    for(int id : useridVec){
        auto it = _userConnectionMap.find(id);
        if(it != _userConnectionMap.end()){
            //转发消息
            it->second->send(js.dump());
        }
        else{
            //查询toid是否在线
            User user = _userModel.query(id);
            if(user.getState()=="online"){
                _redis.publish(id, js.dump());//用户在其他服务器上，向redis发布消息
                return;
            }
            //存储离线消息
            else 
                _offlineMsgModel.insert(id, js.dump());
        }
    }
    response["msg"] = "success!";
    conn->send(response.dump());
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg){

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnectionMap.find(userid);
    if(it != _userConnectionMap.end()){
        it->second->send(msg);
        return;
    }

    //存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}