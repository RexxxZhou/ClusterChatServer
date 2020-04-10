#include "groupmodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

    //创建群组
    bool GroupModel::createGroup(Group& group)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"insert into AllGroup(groupname,groupdesc) values('%s','%s')",group.getName().c_str(),group.getDesc().c_str());
        MySQL mysql;
        if(mysql.connect())
        {
            if(mysql.update(sql))
            {
                group.setId(mysql_insert_id(mysql.getConnection()));
                return true;
            }
        }
        return false;
    }

    //加入群组
    void GroupModel::addGroup(int userid,int groupid,string role)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"insert into GroupUser(groupid,userid,grouprole) values(%d,%d,'%s')",groupid,userid,role.c_str());
        MySQL mysql;
        if(mysql.connect())
        {
            mysql.update(sql);
        }
    }
    //查询用户所在群组,并将每个群组中的所有用户都获得后返回vector
    vector<Group> GroupModel::queryGroups(int userid)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on a.id=b.groupid where b.userid=%d",userid);
        MySQL mysql;
        vector<Group> groupVec;
        if(mysql.connect())
        {
            MYSQL_RES* res=mysql.query(sql);
            if(res!=nullptr)
            {
                MYSQL_ROW row;
                while((row=mysql_fetch_row(res))!=nullptr)
                {
                    Group group;
                    group.setId(atoi(row[0]));
                    group.setName(row[1]);
                    group.setDesc(row[2]);
                    groupVec.push_back(group);
                }
            }
            mysql_free_result(res);
        }
        
        for(auto&group:groupVec)
        {
            sprintf(sql,"select a.id,a.name,a.state,b.grouprole from User a inner join GroupUser b on a.id=b.userid where b.groupid=%d",group.getId());
            cout<<"entering here!"<<endl;
            MYSQL_RES* res=mysql.query(sql);
            if(res!=nullptr)
            {
                MYSQL_ROW row;
                while((row=mysql_fetch_row(res))!=nullptr)
                {
                    GroupUser user;
                    user.setId(atoi(row[0]));
                    user.setName(row[1]);
                    user.setState(row[2]);
                    user.setRole(row[3]);
                    group.getUsers().push_back(user);
                }
            }
            mysql_free_result(res);
        }
        return groupVec;
    }
    //根据userid和groupid查询对应群组中除userid以外的所有用户，用于群发信息
    vector<int> GroupModel::queryGroupUsers(int userid,int groupid)
    {
        //1.组装sql语句
        char sql[1024]={0};
        //sprintf拼接一条mysql语句，这一条语句是insert into User(...) values(...)
        //所以执行了对User表进行插入
        //mysql的操作指定是c style string 所以用的是char数组
        sprintf(sql,"select userid from GroupUser where groupid=%d and userid!=%d",groupid,userid);
        MySQL mysql;
        vector<int> ivec;
        if(mysql.connect())
        {
            MYSQL_RES* res=mysql.query(sql);
            if(res!=nullptr)
            {
                MYSQL_ROW row;
                while((row=mysql_fetch_row(res))!=nullptr)
                {
                    ivec.push_back(atoi(row[0]));
                }
            }
            mysql_free_result(res);
        }
        return ivec;
    }   