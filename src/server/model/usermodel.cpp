#include "UserModel.hpp"
#include "db.h"
#include<iostream>
bool UserModel::insert(User &user)
{
    //1.组装sql语句
    char sql[1024]={0};
    //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
    //所以执行了对User表进行插入
    //mysql的操作指定是c style string 所以用的是char数组
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            //获取插入成功的用户数据的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
User UserModel::query(int id)
{
    //1.组装sql语句
    char sql[1024]={0};
    //sprintf拼接一条mysql语句，这一条语句是查表语句
    //mysql的操作指定是c style string 所以用的是char数组
    sprintf(sql,"select * from User where id=%d",id);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr)
        {
            MYSQL_ROW row=mysql_fetch_row(res);
            if(row!=nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                //释放资源
                mysql_free_result(res);
                return user;
            }
        }
        return User();
    }
}
bool UserModel::updateState(User user)
{
     //1.组装sql语句
    char sql[1024]={0};
    //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
    //mysql的操作指定是c style string 所以用的是char数组
    sprintf(sql,"update User set state='%s' where id=%d",
        user.getState().c_str(),user.getId());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
    //重置用户的状态信息
    void UserModel::resetState()
    {
        //1.组装sql语句
        char sql[1024]="update User set state='Offline' where state='Online'";
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);
        }
    }