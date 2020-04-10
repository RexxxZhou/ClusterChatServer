#include "redis.hpp"
#include <iostream>
using namespace std;
Redis::Redis():_publish_context(nullptr),_subscribe_context(nullptr){}
Redis::~Redis()
{
    if(!_publish_context)
    redisFree(_publish_context);
    if(!_subscribe_context)
    redisFree(_subscribe_context);
}
//连接redis服务器
bool Redis::connect()
{
    //负责publish消息的上下文连接
    _publish_context=redisConnect("127.0.0.1",6379);
    if(!_publish_context)
    {
        cerr<<"Connect redis falied!"<<endl;
        return false;
    }
    //负责subscribe消息的上下文连接
    _subscribe_context=redisConnect("127.0.0.1",6379);
    if(!_subscribe_context)
    {
        cerr<<"Connect redis falied!"<<endl;
        return false;
    }
    //在单独的线程中，监听通道上的事件，有消息给业务层上报
    thread t([&](){observe_channel_message();});
    t.detach();
    cout<<"Connect redis successfully!"<<endl;
    return true;
}
//向redis指定通道发布消息
bool Redis::publish(int channel,string message)
{
    redisReply* reply=(redisReply*)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());
    if(!reply)
    {
        cerr<<"Publish failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
//向redis指定通道订阅消息
bool Redis::subscribe(int channel)
{
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel))
    {
        cerr<<"Subscribe command failed:AppendCommand!"<<endl;
        return false;
    }
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context,&done))
        {
            cerr<<"Subscribe command failed:redisBufferWrite!"<<endl;
            return false;
        }
    }
    return true;
}
//向redis指定通道取消订阅
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel))
    {
        cerr<<"Unsubscribe command failed:AppendCommand!"<<endl;
        return false;
    }
    int done=0;
    while(!done)
    {
        if(REDIS_ERR==redisBufferWrite(this->_subscribe_context,&done))
        {
            cerr<<"Unsubscribe command failed:redisBufferWrite!"<<endl;
            return false;
        }
    }
    return true;

}
//在独立线程中接收订阅通道的消息
void Redis::observe_channel_message()
{
    redisReply* reply=nullptr;
    while (REDIS_OK==redisGetReply(this->_subscribe_context,(void **)&reply))
    {
        //订阅的消息是一个三元素的数组
        if(reply!=nullptr&&reply->element[2]!=nullptr&&reply->element[2]->str!=nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    
}
//初始化向业务层上报通道消息的回调对象
void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler=fn;
}
