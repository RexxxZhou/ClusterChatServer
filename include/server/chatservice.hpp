#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/TcpServer.h>
#include "json.hpp"
#include "UserModel.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include <mutex>//互斥锁
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace muduo::net;
using namespace muduo;
using namespace std;
using json=nlohmann::json;
//表示处理消息的事件回调方法类型
using MsgHandler=std::function<void(const TcpConnectionPtr&,json&,Timestamp)>;
//聊天服务器业务类
class ChatService
{
public:
//获取单理对象的接口函数
    static ChatService* instance();
//处理登录业务
    void login(const TcpConnectionPtr&conn,json &js,Timestamp time);
//处理注册业务
    void reg(const TcpConnectionPtr&conn,json &js,Timestamp time);
//获取消息对应的处理器
    MsgHandler getHandler(int msgid);
//处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr&conn);
//一对一聊天业务
    void oneChat(const TcpConnectionPtr&conn,json &js,Timestamp time);
//服务器异常，业务重置方法
    void reset();
//添加好友业务
    void addFriend(const TcpConnectionPtr&conn,json &js,Timestamp time);
//创建群组
    void createGroup(const TcpConnectionPtr&conn,json &js,Timestamp time);
//加入群组
    void addGroup(const TcpConnectionPtr&conn,json &js,Timestamp time);
//群组聊天  
    void groupChat(const TcpConnectionPtr&conn,json &js,Timestamp time);
//处理注销业务
void loginout(const TcpConnectionPtr&conn,json &js,Timestamp time);
//从redis消息队列中获取订阅的消息
void handleRedisSubscribeMessage(int userid,string msg);
private:
    ChatService();
    //存储消息id和其对应的业务处理方法
    //存的是每个业务方法的函数对象，根据不同的业务代号进行回调。
    unordered_map<int,MsgHandler> _msgHandlerMap;
    //数据操作类对象
    UserModel _userModel;
    //存储在线用户的连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;
    //定义互斥锁，保证_userConnMap的线程安全
    //因为多个用户同时登陆时，会同时对该映射进行访问
    mutex _connMutex;
    OfflineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
    Redis _redis;
};
#endif