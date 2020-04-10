#include "chatserver.hpp"
#include <iostream>
#include "chatservice.hpp"
#include <signal.h>
using namespace std;
//处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc,char**argv)
{
    char*ip=argv[1];
    uint16_t port=atoi(argv[2]);
    signal(SIGINT,resetHandler);
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"Chatserver");
    server.start();
    loop.loop();
    return 0;
}