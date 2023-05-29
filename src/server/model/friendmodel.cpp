#include <vector>
#include "frinedmodel.hpp"
#include "db.hpp"
using namespace std;
//维护好友信息的操作接口方法

//添加好友关系(后续提供删除好友操作)
void FriendModel :: insert(int userid, int friendid){
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values('%d', '%d')",
        userid, friendid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
//返回用户好友列表 
vector<User> FriendModel :: query(int userid){
    char sql[1024] = {0};

    //friend表和user表的联合查询
    sprintf(sql, "select a.id,a.name,a.state from User a inner join Friend b on b.friendid = a.id where b.userid = %d",
                userid);
    vector<User> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){//查询成功
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;

        }
    }
    return vec;
}
