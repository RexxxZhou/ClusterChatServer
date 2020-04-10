#ifndef GROUPMODEL_H
#define GROUPMODEL_H
//#include "groupuser.hpp"
#include "group.hpp"
#include <string>
#include <vector>
class GroupModel
{
    public:
    //创建群组
    bool createGroup(Group& group);
    //加入群组
    void addGroup(int userid,int groupid,string role);
    //查询用户所在群组
    vector<Group> queryGroups(int userid);
    //根据userid和groupid查询对应群组中除userid以外的所有用户，用于群发信息
    vector<int> queryGroupUsers(int userid,int groupid);
    private:

};

#endif