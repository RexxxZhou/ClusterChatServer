#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
using namespace std;
using json=nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

//记录当前系统登录的用户信息
User g_currentUser;
//记录当前用户的好友列表信息
vector<User> g_currentUserFriendList;
//记录当前用户的群组列表信息
vector<Group> g_currentUserGroupList;
//显示当前登录成功用户的基本信息
void showCurrentUserData();

//接收线程
int readHandlerTask(int clientFd)
{
    while(1)
    {
        char buffer[1024]={0};
        int len=recv(clientFd,buffer,1024,0);
        if(-1==len||0==len)
        {
            close(clientFd);
            exit(-1);
        }
        //接收ChatServer转发的数据，反序列化得到消息数据
        json js=json::parse(buffer);
        int msg_type=js["msgid"].get<int>();
        if(ONE_CHAT_MSG==msg_type)
        {
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
        else if(GROUP_CHAT_MSG==msg_type)
        {
            cout<<"群消息[ "<<js["groupid"]<<" ]:"<<endl;
            cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
            continue;
        }
    }
}


//所有客户端命令的实际处理函数
//int形参用于传fd，string用于传数据
void help(int fd=0,string str="");
void chat(int fd,string str);
void addfriend(int fd,string str);
void creategroup(int,string);
void addgroup(int,string);
void groupchat(int,string);
void loginout(int,string);
//控制聊天程序主菜单的布尔值
bool isMainMenuRunning=false;
//获取系统时间，聊天信息会带上时间
string getCurrentTime();

//系统支持的客户端命令列表
unordered_map<string,string> commandMap={
    {"help","显示所有支持的命令，格式help"},
    {"chat","一对一聊天，格式chat:friendId:message"},
    {"addfriend","添加好友，格式addfriend:friendid"},
    {"creategroup","创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式addgroup:groupid"},
    {"groupchat","群聊，格式groupchat:groupid:message"},
    {"loginout","注销，格式loginout"}
};
//注册系统支持的客户端命令处理
unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"loginout",loginout}
};
//主聊天程序
void mainMenu(int clientFd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;
        int idx=commandbuf.find(':');
        if(-1==idx)
        {
            command=commandbuf;
        }
        else
        {
            command=commandbuf.substr(0,idx);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end())
        {
            cout<<"Invalid input command!"<<endl;
            continue;
        }
        it->second(clientFd,commandbuf.substr(idx+1,commandbuf.size()-idx));
        
    }
}
void help(int fd,string str)
{
    for(auto&it:commandMap)
    {
        cout<<it.first<<":"<<it.second<<endl;
    }
    cout<<endl;
}
void addfriend(int fd,string str)
{
    int friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendId"]=friendid;
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"Send addfriend msg error!"<<endl;
    }
}
void chat(int fd,string str)
{
    int idx=str.find(':');
    if(-1==idx)
    {
        cerr<<"Chat command invalid!"<<endl;
    }
    int friendid=stoi(str.substr(0,idx));
    string message=str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["to"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"Send chat msg error!"<<endl;
    }
}
void creategroup(int fd,string str)
{
    int idx=str.find(':');
    if(idx==-1)
    {
        cerr<<"Create group command invalid!"<<endl;
        return;
    }
    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1,str.size()-idx-1);
    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"Send create group msg error!"<<endl;

    }
}
void addgroup(int fd,string str)
{
    int groupid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(-1==len)
    {
        cerr<<"Send addgroup msg error!"<<endl;
    }
}
void groupchat(int fd,string str)
{
    int idx=str.find(':');
    if(idx==-1)
    {
        cerr<<"Groupchat command invalid!"<<endl;
        return;
    }
    int groupid=stoi(str.substr(0,idx));
    string message=str.substr(idx+1,str.size()-idx);
    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["groupid"]=groupid;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"Send group chat msg error!"<<buffer<<endl;
    }


}
void loginout(int fd,string str)
{
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();
    int len=send(fd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"Send login out msg error"<<buffer<<endl;
    }
    else
    isMainMenuRunning=false;
}

//聊天客户端实现,主线程做发送线程，子线程用作接收线程
int main(int argc,char**argv)
{
    if(argc<3)
    {
        cerr<<"Command invalid! Example:./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    //解析输入的ip和端口
    char*ip=argv[1];
    uint16_t port=atoi(argv[2]);
    //创建client的socket
    int clientFd=socket(AF_INET,SOCK_STREAM,0);
    if(clientFd==-1)
    {
        cerr<<"Socket create error"<<endl;
        exit(-1);
    }
    //绑定client需要连接的server的ip和端口
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);
    //client和server连接
    if(-1==connect(clientFd,(sockaddr*)&server,sizeof(sockaddr_in)))
    {
        cerr<<"Connect server error"<<endl;
        close(clientFd);
        exit(-1);
    }
    while(1)
    {
        cout<<"=================="<<endl;
        cout<<"1.Login"<<endl;
        cout<<"2.Register"<<endl;
        cout<<"3.Quit"<<endl;
        cout<<"=================="<<endl;
        cout<<"Your Choice: ";
        int choice=0;
        cin>>choice;
        cin.get();//读取缓冲区残余的回车

        switch(choice)
        {
            case(1)://Login
            {
                int id=0;
                char pwd[50]={0};
                cout<<"Input your ID:"<<endl;
                cin>>id;
                cin.get();//读掉缓冲区残留回车
                cout<<"Input your password:"<<endl;
                cin.getline(pwd,50);
                json js;
                js["msgid"]=LOGIN_MSG;
                js["id"]=id;
                js["password"]=pwd;
                string request=js.dump();
                int len=send(clientFd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1)
                {
                    cerr<<"Send login msg falied."<<endl;
                }
                else
                {
                    char buffer[1024]={0};
                    len=recv(clientFd,buffer,1024,0);
                    if(-1==len)
                    {
                        cerr<<"Receive login msg failed."<<endl;
                    }
                    else
                    {
                        json responsejs=json::parse(buffer);
                        if(responsejs["errno"]!=0)//登录失败
                        {
                            cerr<<responsejs["errmsg"]<<endl;
                        }
                        else//登陆成功
                        {
                            isMainMenuRunning=true;
                            //记录当前用户的信息
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"]);
                            //记录当前用户的好友列表
                            if(responsejs.contains("friends"))
                            {   
                                //初始化，避免多次登录内容重复
                                g_currentUserFriendList.clear();
                                vector<string>vec=responsejs["friends"];
                                for(string&s:vec)
                                {
                                    json js=json::parse(s);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    g_currentUserFriendList.push_back(user);
                                }
                            }
                            //记录当前用户的群组列表
                            if(responsejs.contains("groups"))
                            {
                                //初始化，避免多次登录内容重复
                                g_currentUserGroupList.clear();
                                vector<string> vec1=responsejs["groups"];
                                for(string&groupstr:vec1)
                                {
                                    json grpjs=json::parse(groupstr);
                                    Group group;
                                    group.setId(grpjs["groupid"].get<int>());
                                    group.setName(grpjs["groupname"]);
                                    group.setDesc(grpjs["groupdesc"]);

                                    vector<string> vec2=grpjs["users"];
                                    for(string&userstr:vec2)
                                    {
                                        GroupUser user;
                                        json js=json::parse(userstr);
                                        user.setId(js["id"].get<int>());
                                        user.setName(js["name"]);
                                        user.setState(js["state"]);
                                        user.setRole(js["role"]);
                                        group.getUsers().push_back(user);
                                    }
                                    g_currentUserGroupList.push_back(group);
                                }
                            }
                            //显示登录用户的基本信息
                            showCurrentUserData();
                            //显示当前用户的离线信息 个人聊天信息 或群组聊天信息
                            if(responsejs.contains("OfflineMsg"))
                            {
                                vector<string>vec=responsejs["OfflineMsg"];
                                for(string&str:vec)
                                {
                                    json js=json::parse(str);
                                    int msg_type=js["msgid"].get<int>();
                                    if(ONE_CHAT_MSG==msg_type)
                                    {
                                        cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
                                    }
                                    else
                                    {
                                        cout<<"群消息[ "<<js["groupid"]<<" ]:"<<endl;
                                        cout<<js["time"].get<string>()<<" ["<<js["id"]<<"]"<<js["name"].get<string>()<<" said:"<<js["msg"].get<string>()<<endl;
                                    }
                                }
                            }
                            //登录成功，启动接收线程负责接收数据
                            static int readThreadNum=0;
                            if(readThreadNum==0)
                            {
                                //C++的线程库，thread是个类型
                            //语言层面上的，所以可以跨平台，会自动用不同os的线程创建方法,线程只启动一次
                            std::thread readTask(readHandlerTask,clientFd);//相当于调用了pthread_create
                            //分离线程
                            readTask.detach();//相当于调用了pthread_detach
                            ++readThreadNum;
                            }
                            //进入聊天主菜单界面
                            mainMenu(clientFd);
                        }
                        
                    }
                }
            break;
            }
            case(2)://Register
            {
                char name[50]={0};
                char pwd[50]={0};
                cout<<"Please input your name:"<<endl;
                cin.getline(name,50);
                cout<<"Please set your password:"<<endl;
                cin.getline(pwd,50);
                json js;
                js["msgid"]=REG_MSG;
                js["name"]=name;
                js["password"]=pwd;
                string request= js.dump();
                int len=send(clientFd,request.c_str(),strlen(request.c_str())+1,0);
                if(len==-1)
                {
                    cerr<<"Send regiser msg error."<<request<<endl;
                }
                else
                {
                    char buffer[1024]={0};
                    len=recv(clientFd,buffer,1024,0);//阻塞等待
                    if(-1==len)
                    {
                        cerr<<"Receive register msg error."<<endl;
                    }
                    else
                    {
                        json responsejs=json::parse(buffer);
                        if(0!=responsejs["errno"].get<int>())//注册失败
                        {
                            cerr<<"Given name already exists, try another one!"<<endl;
                        }
                        else//注册成功
                        {
                            cout<<name<<" register success, userid is: "<<responsejs["id"]<<
                            ", do not forget it!"<<endl;
                        }
                    }
                }
                break;
            }
            case(3)://Quit
            {
                exit(0);
            }
            default:
            cerr<<"Invalid input!"<<endl;
            break;
        }
    }
}









//显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout<<"=============Login User============"<<endl;
    cout<<"Current Login User=>id: "<<g_currentUser.getId()<<" name: "<<g_currentUser.getName()<<endl;
    cout<<"-------------Friend List------------"<<endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User&user:g_currentUserFriendList)
        {
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"------------Group List-------------"<<endl;
    if(!g_currentUserGroupList.empty())
    {
        for(Group&group:g_currentUserGroupList)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            //cout<<group.getUsers().size()<<endl;
            for(GroupUser&user:group.getUsers())
            {
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<" "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"===================================="<<endl;
}
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}