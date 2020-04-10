#include "db.h"
#include<string>
#include<mysql/mysql.h>
#include<muduo/base/Logging.h>
//数据配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "wang0923K";
static string dbname = "chat";
// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    mysql_close(_conn);
}
// 连接数据库
bool MySQL::connect()
{
    //根据给定的数据库服务器地址，用户名密码，进行数据库连接
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
    password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        //设置gbk能够读取中文，C和C++的默认编码是ASCII码，否则从数据库拉下来的可能是乱码
        mysql_query(_conn, "set names gbk");
        LOG_INFO<<"Successully connected to mysql!";
    }
    else
    {
        LOG_INFO<<"Failed to connect to mysql!";
    }
    return p;
}
// 更新操作
bool MySQL::update(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
    << sql << "更新失败!";
    return false;
    }
    return true;
}
// 查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
    LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
    << sql << "查询失败!";
    return nullptr;
    }
    return mysql_use_result(_conn);
}

//获取连接
MYSQL* MySQL::getConnection()
{
    return _conn;
}