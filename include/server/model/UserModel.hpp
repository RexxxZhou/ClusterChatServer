#ifndef USERMODEL_H
#define USERMODEL_H
//User表的数据操作类
#include"user.hpp"
class UserModel
{
public:
    //User表的增加方法
    bool insert(User &user);

    //根据用户号码查询用户信息
    User query(int id);

    //更新用户状态信息state
    bool updateState(User user);

    //重置用户的状态信息
    void resetState();
};
#endif