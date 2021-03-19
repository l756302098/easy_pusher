/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-16 15:50:44
 * @LastEditors: li
 * @LastEditTime: 2021-03-19 11:00:40
 */
#ifndef PUSH_NODE_HPP
#define PUSH_NODE_HPP

#include <thread>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <condition_variable>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "base64.hpp"
#include "zlapi_worker.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>


using namespace rapidjson;
typedef websocketpp::client<websocketpp::config::asio_client> ClientType;
typedef websocketpp::connection_hdl ConnectionType;

enum ConnectState{
    Close=1,Waitting=2,Open=3
};

typedef std::function<void(std::vector<char>&,uint64_t,uint64_t)> PushFun;

class push_node
{
public:
    PushFun push_fun;
    push_node(const std::string &addr,const std::string &core_addr,int port,const std::string &topic);
    push_node(){};
    ~push_node();
    void process();
    void registerTopic(const std::string &topic,const std::string &id, const std::string& type = "sensor_msgs/Image", int throttle_rate = 0, int queue_length = 10);
    void dealMsg(std::string &msg);

    void pusher(std::vector<char> &data);
    void initPusher(std::string host,std::string app,std::string stream);
    void stop();
    void runOnce();

    void connect();
    void onFailed(ConnectionType hdl);
    void onOpen(ConnectionType hdl);
    void onClose(ConnectionType hdl);
    void onMessage(ConnectionType hdl, ClientType::message_ptr msg);
    
protected:
    std::shared_ptr<std::thread> _thread;
    std::vector<std::string> _messages;

    ConnectState _clientStatus;
    ClientType _easy_client;
    ConnectionType _ppConnection;
    void start(const std::string &addr = "localhost", int port = 9090);
    std::string _wsUrl;
    websocketpp::lib::shared_ptr<websocketpp::lib::thread> m_thread;
    std::atomic<int> messageCount;
    std::atomic<bool> isNewConnect;

    bool _isProcessing;
    std::mutex _actionLock;  
    std::condition_variable _actionCondition;

    unsigned char header[4] = {0x00,0x00,0x00,0x01};
    std::string _host,_app,_stream;
    ZLMWorker zlpusher;
    std::string _topic,_clientName,_subInfo;
    std::string _addr,_core_addr;
    int _port;
public:
    
};


#endif