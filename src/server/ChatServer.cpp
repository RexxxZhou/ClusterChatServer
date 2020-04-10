#include "chatserver.hpp"
#include "json.hpp"
#include <functional>
#include <string>
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using json=nlohmann::json;
//初始化聊天服务器
ChatServer::ChatServer(EventLoop* loop,
        const InetAddress& listenAddr,
        const string& nameArg):_server(loop,listenAddr,nameArg),
        _loop(loop)
        {
            //注册连接回调
            _server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));
            //注册信息回调
            _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));
            //设置线程数量
            _server.setThreadNum(4);
        }
//启动服务
void ChatServer::start()
    {
        _server.start();
    }

void ChatServer::onConnection(const TcpConnectionPtr&conn)
{
    //客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr&conn,Buffer*buffer,Timestamp time)
{
    string buf=buffer->retrieveAllAsString();
    //数据的反序列化，解码
    json js=json::parse(buf);
    //目的：完全解耦网络模块与业务模块的代码,不在服务器类中处理业务，业务给专门的业务类处理，只负责转发
    //获取处理服务的唯一实例
    //msgid是我们给定的数据库中的键，对不同的信息类型实现不同的回调。
    auto msgHandler=ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，执行相应的额业务处理
    msgHandler(conn,js,time);
}

