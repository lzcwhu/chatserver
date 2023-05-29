#include "offlinemessagemodel.hpp"
#include "db.hpp"

//插入离线消息
void OfflineMsgmodel::insert(int userid, string msg){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into OfflineMessage values('%d', '%s')",
        userid, msg.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
    
    
}

//删除离线消息
void OfflineMsgmodel::remove(int userid){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from OfflineMessage where userid=%d",userid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}
//查询userid的离线消息
vector<string> OfflineMsgmodel::query(int userid){
    char sql[1024] = {0};
    sprintf(sql, "select message from OfflineMessage where userid=%d", userid);
    vector<string> vec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES* res = mysql.query(sql);
        if(res != nullptr){//查询成功
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;

        }
    }
    return vec;
}