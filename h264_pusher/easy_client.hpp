/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-02 11:32:41
 * @LastEditors: li
 * @LastEditTime: 2021-03-05 10:26:01
 */
#ifndef EASY_CLIENT_HPP
#define EASY_CLIENT_HPP
#include <thread>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <condition_variable>
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "base64.hpp"
#include "nalu.hpp"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "EasyPusherAPI.h"
#include "EasyTypes.h"

using namespace rapidjson;
typedef websocketpp::client<websocketpp::config::asio_client> ClientType;
typedef websocketpp::connection_hdl ConnectionType;

enum ConnectState{
    Close=1,Waitting=2,Open=3
};

typedef std::function<void(std::vector<char>&,uint64_t,uint64_t)> PushFun;

class easy_client
{
public:
    PushFun push_fun;
    easy_client(const std::string &addr,const std::string &core_addr,int port,const std::string &topic);
    easy_client(){};
    ~easy_client();
    void process();
    void registerTopic(const std::string &topic,const std::string &id, const std::string& type = "sensor_msgs/Image", int throttle_rate = 0, int queue_length = 10);
    void dealMsg(std::string &msg);

    void pusher(std::vector<char> &data);
    void openRtsp();
    void stop();
    void runOnce();

    void connect();
    void onFailed(ConnectionType hdl);
    void onOpen(ConnectionType hdl);
    void onClose(ConnectionType hdl);
    void onMessage(ConnectionType hdl, ClientType::message_ptr msg);
    
protected:
    std::thread* _thread;
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

    Easy_Handle	fPusherHandle = 0;
    EASY_MEDIA_INFO_T mediainfo;
    bool init_nal;
    unsigned char btHeader[4] = {0x00,0x00,0x00,0x01};
    unsigned char bpHeader[4] = {0x00,0x00,0x01};

    std::string _topic,_clientName,_subInfo;
    std::string _addr,_core_addr;
    int _port,_seq;
public:
    std::string config_ip,config_name;
    int config_port;
};

#endif