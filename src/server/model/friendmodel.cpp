#include "friendmodel.hpp"
#include "db.h"
#include <vector>
#include "user.hpp"
//添加好友
void FriendModel::insert(int userid,int friendId)
{
    //1.组装sql语句
    char sql[1024]={0};
    //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
    //所以执行了对User表进行插入
    //mysql的操作指定是c style string 所以用的是char数组
    sprintf(sql,"insert into Friend values(%d,%d)",userid,friendId);
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

//返回用户好友列表 friendId name state
//Friend和User两个表联合查询
vector<User> FriendModel::query(int userid)
{
    //1.组装sql语句
    char sql[1024]={0};
    vector<User> vec;
    //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
    //所以执行了对User表进行插入
    //mysql的操作指定是c style string 所以用的是char数组
    sprintf(sql,"select a.id,a.name,a.state from User a inner join Friend b on b.friendid=a.id where b.userid=%d",userid);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row;
            //把userid用户的所有离线消息放入vec中返回
            while((row=mysql_fetch_row(res))!=nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return vec;
}