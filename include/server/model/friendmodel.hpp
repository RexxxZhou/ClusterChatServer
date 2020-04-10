#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "user.hpp"
#include <vector>
using namespace std;
//维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友
    void insert(int userid,int friendId);
    
    //返回用户好友列表 friendId name state
    //Friend和User两个表联合查询
    vector<User> query(int userid);

};
#endif