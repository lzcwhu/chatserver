#ifndef FRIENDVERIFY_H
#define FRIENDVERIFY_H

#include <vector>
using namespace std;
//提供离线消息表的操作
class FriendVerify{
    public:
        //存储用户的好友请求消息
        void insert(int userid, int friendid);
        //删除用户的好友请求
        void remove(int userid, int friendid);
        //查询用户好友请求消息
        vector<int> query(int userid);
};


#endif