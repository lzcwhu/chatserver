#include "groupmodel.hpp"
#include "db.hpp"

//创建群组
bool GroupModel :: createGroup(Group &group){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')",
        group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

//加入群组
void GroupModel::addGroup(int userid, int groupid, string role){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values('%d', '%d', '%s')",
        groupid, userid, role.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询用户所在的群组信息
vector<Group> GroupModel::queryGroups(int userid){
    /*先根据userid在groupuser表中查询出该用户所属的群组信息
    再根据群组信息，查询属于该群组的所有用户的userid，并且和user表进行多表联合查询，查询用户的详细信息*/
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
GroupUser b on a.id = b.groupid where b.userid=%d",userid);

    vector<Group> groupVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res !=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){

                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    //查询群组的用户信息
    for(Group &group : groupVec){
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a inner join \
            GroupUser b on b.userid = a.id where b.groupid=%d",group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if(res !=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){

                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);

                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
   
}


//根据指定的groupid查询群组用户id列表,群发消息给其他群组成员
vector<int> GroupModel :: queryGroupUsers(int userid, int groupid){
    //1组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid !=%d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if(mysql.connect()){
        MYSQL_RES *res = mysql.query(sql);
        if(res !=nullptr){
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr){

               idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return idVec;
}

//新增方法：根据群id 查询群表中是否存在该群，并且查询该用户是否在该群
bool GroupModel::query(int userid, int groupid){
    char sql[1024] = {0};
    sprintf(sql, "select id from AllGroup where id=%d",groupid);
    MySQL mysql;
    MYSQL_RES *res = mysql.query(sql);
    if(res == nullptr) return false;

    //groupid存在，查询该群中是否已经存在该用户
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid =%d", groupid, userid);
    res = mysql.query(sql);
    if(res == nullptr) return true; //如果该用户不在该群，则可以加入该群

    return false;
}