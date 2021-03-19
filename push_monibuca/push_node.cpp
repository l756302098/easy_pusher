/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-16 15:50:44
 * @LastEditors: li
 * @LastEditTime: 2021-03-19 11:36:44
 */
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <sys/time.h>

#include "push_node.hpp"
#include <thread>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

push_node::push_node(const std::string &addr,const std::string &core_addr,int port,const std::string &topic):messageCount(0),isNewConnect(false),_clientStatus(ConnectState::Close)
{
    _addr = addr;
    _port = port;
    _topic = topic;
    _core_addr = core_addr;
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    std::string uuid = boost::uuids::to_string(a_uuid);
    registerTopic(topic,uuid);

    _wsUrl = "ws://"+addr+":"+std::to_string(port);
    std::cout << "open url " << _wsUrl << std::endl;
    _easy_client.set_access_channels(websocketpp::log::alevel::all);
    _easy_client.clear_access_channels(websocketpp::log::alevel::all);
    _easy_client.clear_error_channels(websocketpp::log::alevel::all);
    _easy_client.init_asio();
    _easy_client.start_perpetual();
    //m_thread.reset(new websocketpp::lib::thread(&ClientType::run, &_easy_client));
    _thread.reset(new std::thread(std::bind(&push_node::process, this)));
    m_thread.reset(new websocketpp::lib::thread(&ClientType::run, &_easy_client));
}

push_node::~push_node()
{
    stop();
}


void push_node::connect(){
    _easy_client.reset();
    websocketpp::lib::error_code ec;
    ClientType::connection_ptr con = _easy_client.get_connection(_wsUrl, ec);
    if (ec) {
        std::cout << "[push_node] could not create connection because: " << ec.message() << std::endl;
        return;
    }
    _easy_client.set_open_handler(websocketpp::lib::bind(&push_node::onOpen, this, ::_1));
    _easy_client.set_fail_handler(websocketpp::lib::bind(&push_node::onFailed, this, ::_1));
    _easy_client.set_close_handler(websocketpp::lib::bind(&push_node::onClose, this, ::_1));
    _easy_client.set_message_handler(websocketpp::lib::bind(&push_node::onMessage, this, ::_1, ::_2));
    _easy_client.connect(con);
}

void push_node::onFailed(ConnectionType hdl){
    std::cout <<"[push_node] onFailed  " << std::endl;
    if(hdl.lock().get()==_ppConnection.lock().get()){
        _ppConnection = hdl;
        _clientStatus = ConnectState::Close;
    }
}
void push_node::onOpen(ConnectionType hdl){
    std::cout <<"[push_node] open  " << std::endl;
    isNewConnect = true;
    _ppConnection = hdl;
    _clientStatus = ConnectState::Open;
    websocketpp::lib::error_code ec;
    _easy_client.send(_ppConnection, _subInfo, websocketpp::frame::opcode::binary, ec);
    if (ec)
    {
        std::cout <<"[push_node] send message failed: " << ec.message() << std::endl;
    }
}
void push_node::onClose(ConnectionType hdl){
    if(hdl.lock().get()==_ppConnection.lock().get()){
        _ppConnection = hdl;
        _clientStatus = ConnectState::Close;
    }
}
void push_node::onMessage(ConnectionType hdl, ClientType::message_ptr msg){
    //std::cout << "onMessage" << std::endl;
    std::unique_lock<std::mutex> lock(_actionLock);
    _messages.push_back(msg->get_payload());
    _actionCondition.notify_one();
}

void push_node::process()
{
    _isProcessing = true;
    while (_isProcessing)
    {
        std::unique_lock<std::mutex> lock(_actionLock);
        while (_messages.empty() && _isProcessing)
            _actionCondition.wait(lock);

        std::vector<std::string> localMsgs;
        localMsgs.swap(_messages);
        lock.unlock();

        for (std::vector<std::string>::iterator itr = localMsgs.begin();
             itr != localMsgs.end(); ++itr)
        {
            dealMsg(*itr);
        }
    }
}

void push_node::dealMsg(std::string &msg)
{
    try
    {
        messageCount++;
        //std::cout << "get new msg " << msg<< std::endl;
        const char *json = msg.c_str();
        Document document;
        ParseResult result = document.Parse(json);
        if (result.IsError())
        {
            std::cout << "JSON parse error:" << result.Code() << " offset:" << result.Offset() << std::endl;
            return;
        }
        if (!document.IsObject())
        {
            std::cout << "message is not object " << msg << std::endl;
            return;
        }
        Value &v = document["msg"];
        /*
        uint64_t secs = 0;
        uint64_t nsecs = 0;
        if (v.HasMember("header") && v["header"].IsObject())
        {
            auto header = v["header"].GetObject();
            if (header["stamp"].IsObject())
            {
                auto stamp = header["stamp"].GetObject();
                secs = stamp["secs"].GetUint64();
                nsecs = stamp["nsecs"].GetUint64();
                std::cout << "secs:" << stamp["secs"].GetUint64() << "nsecs:" << stamp["nsecs"].GetUint64() << std::endl;
            }
        }
        */
        if (v["data"].IsString())
        {
            std::string value = v["data"].GetString();
            std::string decodeData = rtsptool::base64_decode(value);
            char *vdata = (char *)decodeData.c_str();
            int count = decodeData.size();
            std::vector<char> rdata;
            rdata.insert(rdata.end(), vdata, vdata + count);
            pusher(rdata);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void push_node::stop()
{
    _isProcessing = false;
    _easy_client.stop_perpetual();
    websocketpp::lib::error_code ec;
    _easy_client.close(_ppConnection, websocketpp::close::status::going_away, "", ec);
    m_thread->join();
    delete m_thread.get();
    _thread->join();
}

void push_node::runOnce()
{
    if(messageCount<=0){
        std::cout  << "runOnce:topic "<< _topic << " no message" << std::endl;
    }
    else{
        if(isNewConnect){
            std::cout << "runOnce:again get new message " << std::endl;
            //openRtsp();
            isNewConnect = false;
        }
        std::cout << "runOnce:topic "<< _topic << std::endl;
    }
    messageCount=0;
    if(_clientStatus==ConnectState::Close){
        std::cout  << "runOnce:topic "<< _topic << " reconnect" << std::endl;
        connect();
        sleep(5);
    }
}

void push_node::registerTopic(const std::string &topic,const std::string &id, const std::string& type, int throttle_rate, int queue_length)
{
    //Assembly data
    std::string message = "\"op\":\"subscribe\", \"topic\":\"" + topic + "\",\"ip\":\""+ _core_addr +"\"";
    if (id.compare("") != 0)
    {
        message += ", \"id\":\"" + id + "\"";
    }
    if (type.compare("") != 0)
    {
        message += ", \"type\":\"" + type + "\"";
    }
    if (throttle_rate > -1)
    {
        message += ", \"throttle_rate\":" + std::to_string(throttle_rate);
    }
    if (queue_length > -1)
    {
        message += ", \"queue_length\":" + std::to_string(queue_length);
    }
    message = "{" + message + "}";
    std::cout << "message:" << message << std::endl;
    _subInfo = message;
}

void push_node::initPusher(std::string host,std::string app,std::string stream)
{
	std::string rtsp_url = "rtsp://"+host+"/"+app+"/"+stream;
	std::cout << "open url " << rtsp_url << std::endl;
	zlpusher.ZLM_SetPushUrl(rtsp_url);
	zlpusher.ZLM_MediaInit("rtsp",host,app,stream);//"__defaultVhost__"
}

void push_node::pusher(std::vector<char> &data){
	if(data.size()>5){
		int prefix_size = 4;
        int result_size = data.size() + prefix_size;
        char *ptr = new char [result_size];
		memmove(ptr+prefix_size, &(data[0]), data.size());
		memcpy(ptr, header, 4);
		zlpusher.ZLM_Input(ptr,result_size);
	}
}
