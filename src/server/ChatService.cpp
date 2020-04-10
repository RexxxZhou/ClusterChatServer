#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
#include <map>
#include <iostream>
using namespace std;
using namespace muduo;

//获取单例对象的接口函数
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}
//注册消息以及对应的回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});
    //连接Redis服务器
    if(_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it=_msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end())
    {
        //返回一个默认处理器，空操作
        return [=](const TcpConnectionPtr&conn,json &js,Timestamp time)
        {
            LOG_ERROR<<"Msgid: "<<msgid<<" cannot find handler!";
            //cout<<"Msgid: "<<msgid<<" cannot find handler!"<<endl;
        };

    }
    else
    return _msgHandlerMap[msgid];
}
//服务器异常，业务重置方法
    void ChatService::reset()
    {
        //把online状态的用户，设置为offline
        _userModel.resetState();
    }

//处理登录业务
    void ChatService::login(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {

        int id=js["id"];
        string pwd=js["password"];
        User user=_userModel.query(id);
        if(user.getId()==id&&user.getPwd()==pwd)
        {
            if(user.getState()=="Online")
            {
                //该用户已经登录，不允许重复登录
                json response;
                response["msgid"]=LOGIN_MSG_ACK;
                response["errno"]=2;//用以判断返回是否正确
                response["errmsg"]="该账号已登录，请重新输入账号";
                conn->send(response.dump());
            }
            
            else
           { 
               //登陆成功，记录用户连接信息
               {
                   //加锁，离开作用域后自动解锁
                   //对_userConnMap的操作都要加互斥锁，保证多线程安全，因为可能多个用户同时访问该对象。
                   lock_guard<mutex> lock(_connMutex);
                    _userConnMap.insert({id,conn});
               }
               //id用户登录成功后，向redis订阅channel
               _redis.subscribe(id);
                //登陆成功,更新用户状态信息
                user.setState("Online");
                _userModel.updateState(user);
                json response;
                response["msgid"]=LOGIN_MSG_ACK;
                response["errno"]=0;//用以判断返回是否正确
                response["id"]=user.getId();
                response["name"]=user.getName();
                //查询该用户是否有离线消息
                vector<string> vec=_offlineMsgModel.query(id);
                if(!vec.empty())
                {
                    //有离线消息则全部传回给用户
                    response["OfflineMsg"]=vec;
                    //传回后删除数据库中的所有离线消息
                    _offlineMsgModel.remove(id);
                }
                vector<User> userVec=_friendModel.query(id);
                if(!userVec.empty())
                {
                    vector<string> vec2;
                    for(auto&user:userVec)
                    {
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        vec2.push_back(js.dump());
                    }
                    response["friends"]=vec2;
                }
                vector<Group> groupvec=_groupModel.queryGroups(id);
                if(!groupvec.empty())
                {
                    vector<string> groupV;
                    vector<string> userV;
                    for(auto&group:groupvec)
                    {
                        json grpjs;
                        grpjs["groupid"]=group.getId();
                        grpjs["groupname"]=group.getName();
                        grpjs["groupdesc"]=group.getDesc();
                        cout<<group.getUsers().size()<<endl;
                        for(auto&user:group.getUsers())
                        {
                            json userjs;
                            userjs["id"]=user.getId();
                            userjs["name"]=user.getName();
                            userjs["state"]=user.getState();
                            userjs["role"]=user.getRole();
                            userV.push_back(userjs.dump());
                        }
                        grpjs["users"]=userV;
                        groupV.push_back(grpjs.dump());
                    }
                    response["groups"]=groupV;
                }
                conn->send(response.dump());
            }
        }
        else
        {
            
            //用户不存在或密码错误，登录失败
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=1;//用以判断返回是否正确
            response["errmsg"]="用户名或密码错误";
            //response["id"]=user.getId();
            //response["name"]=user.getName();
            conn->send(response.dump());
        }
    }
//处理注册业务
    void ChatService::reg(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        string name=js["name"];
        string password=js["password"];
        User user;
        user.setName(name);
        user.setPwd(password);
        bool state=_userModel.insert(user);
        if(state)
        {
            //注册成功
            json response;
            response["msgid"]=REG_MSG_ACK;
            response["errno"]=0;//用以判断返回是否正确
            response["id"]=user.getId();
            conn->send(response.dump());
        }
        else
        {
            //注册失败
            json response;
            response["msgid"]=REG_MSG_ACK;
            response["errno"]=1;//用以判断返回是否正确
            conn->send(response.dump());   
        }
        
        //cout<<" do reg service!"<<endl;
    }
    //处理客户端异常退出
    void ChatService::clientCloseException(const TcpConnectionPtr&conn)
    {
        User user;
        {
        lock_guard<mutex> lock(_connMutex);
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();++it)
        {
            if(it->second==conn)
            {
                //从map中删除用户的连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
        }
        //用户注销，在redis中取消订阅通道
        _redis.unsubscribe(user.getId());
        //更新用户的状态信息
        if(user.getId()!=-1)
        {
            user.setState("Offline");
            _userModel.updateState(user);
        }
    }

//一对一聊天业务
    void ChatService::oneChat(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        int toId=js["to"].get<int>();
        {
            lock_guard<mutex> lock(_connMutex);
            auto it=_userConnMap.find(toId);
            if(it!=_userConnMap.end())
            {
                //toId在线，信息接收端在线，发送消息
                //服务器主动推送消息给toId目的用户
                //收发双方都在线且在同一台服务器中
                it->second->send(js.dump());
                return;
            }
        }
        //接收方在线，但不在同一台服务器，向redis的通道发布消息
        User user=_userModel.query(toId);
        if(user.getState()=="Online")
        {
            _redis.publish(toId,js.dump());
            return;
        }
        //toId不在线，信息接收端不在线，存储离线消息
        _offlineMsgModel.insert(toId,js.dump());

    }
    //添加好友业务 msgid id friendId
    void ChatService::addFriend(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        int userid=js["id"].get<int>();
        int friendId=js["friendId"].get<int>();
        //存储好友信息
        _friendModel.insert(userid,friendId);
    }

//创建群组
    void ChatService::createGroup(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        int userid=js["id"].get<int>();
        string name=js["groupname"];
        string desc=js["groupdesc"];
        Group group(-1,name,desc);
        if(_groupModel.createGroup(group))
        {
            _groupModel.addGroup(userid,group.getId(),"creator");
        }
    }
//加入群组
    void ChatService::addGroup(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        int userid=js["id"].get<int>();
        int groupid=js["groupid"].get<int>();
        _groupModel.addGroup(userid,groupid,"normal");
    }
//群组聊天  
    void ChatService::groupChat(const TcpConnectionPtr&conn,json &js,Timestamp time)
    {
        int userid=js["id"].get<int>();
        int groupid=js["groupid"].get<int>();
        vector<int>useridVec=_groupModel.queryGroupUsers(userid,groupid);
        {
            lock_guard<mutex> lock(_connMutex);
            for(int& id:useridVec)
            {
                auto it=_userConnMap.find(id);
                if(it!=_userConnMap.end())
                {
                    it->second->send(js.dump());
                }
                else
                {
                    //查询是否在别的服务器上在线
                    User user=_userModel.query(id);
                    if(user.getState()=="Online")
                    {
                        _redis.publish(id,js.dump());
                    }
                    else
                    _offlineMsgModel.insert(id,js.dump());
                }
            }
        }
    }

//处理注销业务
void ChatService::loginout(const TcpConnectionPtr&conn,json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(userid);
        if(it!=_userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    //用户注销，在redis中取消订阅通道
    _redis.unsubscribe(userid);
        User user(userid,"","Offline");
        _userModel.updateState(user);
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it =_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;

    }
    //存储用户的离线消息
    _offlineMsgModel.insert(userid,msg); 
}