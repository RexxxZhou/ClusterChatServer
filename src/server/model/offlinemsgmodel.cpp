#include "offlinemsgmodel.hpp"
#include "db.h"
//存储用户的离线消息
    void OfflineMsgModel::insert(int userid,string msg)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"insert into OfflineMessage values(%d,'%s')",userid,msg.c_str());
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);
        }
    }
    //删除用户的离线消息
    void OfflineMsgModel::remove(int userid)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"delete from OfflineMessage where userid=%d",userid);
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);
        }
    }
    //查询用户的离线消息
    vector<string> OfflineMsgModel::query(int userid)
    {
        //1.组装sql语句
        char sql[1024]={0};
        vector<string> vec;
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"select message from OfflineMessage where userid=%d",userid);
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
                    vec.push_back(row[0]);
                }
                mysql_free_result(res);
            }
        }
        return vec;
    }
