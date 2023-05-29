#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;
class GroupModel{
public:
    //创建群组
    bool createGroup(Group &group);
    //加入群组
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组的信息
    vector<Group> queryGroups(int userid);
    //根据指定的groupid查询群组用户id列表，除userid自己，给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
    //新增方法：根据群id 查询群表中是否存在该群，并且查询该用户是否在该群
    bool query(int userid, int groupid);
};


#endif