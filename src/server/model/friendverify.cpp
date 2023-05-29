#include "friendverify.hpp"

#include "db.hpp"

//插入离线消息
void FriendVerify::insert(int userid, int friendid){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into FriendVerify values('%d', '%d')",
        userid, friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
    
    
}

//删除离线消息
void FriendVerify::remove(int userid, int friendid){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from FriendVerify where userid=%d and friendid=%d",userid, friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
//查询userid的好友验证消息
vector<int> FriendVerify::query(int userid){
    char sql[1024] = {0};
    sprintf(sql, "select userid from FriendVerify where friendid=%d", userid);
    vector<int> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){//查询成功
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                vec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}