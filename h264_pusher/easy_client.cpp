/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-02 11:35:55
 * @LastEditors: li
 * @LastEditTime: 2021-03-03 15:35:08
 */
#include "easy_client.hpp"
#include <thread>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

easy_client::easy_client(const std::string &addr,int port,const std::string &topic):messageCount(0),isNewConnect(false),_clientStatus(ConnectState::Close)
{
    _addr =addr;
    _port = port;
    _topic = topic;
    
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    registerTopic(topic,_uuid);

    _wsUrl = "ws://"+addr+":"+std::to_string(port);
    std::cout << "open url " << _wsUrl << std::endl;
    _easy_client.set_access_channels(websocketpp::log::alevel::all);
    _easy_client.clear_access_channels(websocketpp::log::alevel::all);
    _easy_client.clear_error_channels(websocketpp::log::alevel::all);
    _easy_client.init_asio();
    _easy_client.start_perpetual();
    //m_thread.reset(new websocketpp::lib::thread(&ClientType::run, &_easy_client));
    _thread = new std::thread(std::bind(&easy_client::process, this));
    m_thread.reset(new websocketpp::lib::thread(&ClientType::run, &_easy_client));
}

easy_client::~easy_client()
{
    stop();
}


void easy_client::connect(){
    _easy_client.reset();
    websocketpp::lib::error_code ec;
    ClientType::connection_ptr con = _easy_client.get_connection(_wsUrl, ec);
    if (ec) {
        std::cout << "[easy_client] could not create connection because: " << ec.message() << std::endl;
        return;
    }
    _easy_client.set_open_handler(websocketpp::lib::bind(&easy_client::onOpen, this, ::_1));
    _easy_client.set_fail_handler(websocketpp::lib::bind(&easy_client::onFailed, this, ::_1));
    _easy_client.set_close_handler(websocketpp::lib::bind(&easy_client::onClose, this, ::_1));
    _easy_client.set_message_handler(websocketpp::lib::bind(&easy_client::onMessage, this, ::_1, ::_2));
    _easy_client.connect(con);
}

void easy_client::onFailed(ConnectionType hdl){
    std::cout <<"[easy_client] onFailed  " << std::endl;
    if(hdl.lock().get()==_ppConnection.lock().get()){
        _ppConnection = hdl;
        _clientStatus = ConnectState::Close;
    }
}
void easy_client::onOpen(ConnectionType hdl){
    std::cout <<"[easy_client] open  " << std::endl;
    isNewConnect = true;
    _ppConnection = hdl;
    _clientStatus = ConnectState::Open;
    websocketpp::lib::error_code ec;
    _easy_client.send(_ppConnection, _subInfo, websocketpp::frame::opcode::binary, ec);
    if (ec)
    {
        std::cout <<"[easy_client] send message failed: " << ec.message() << std::endl;
    }
}
void easy_client::onClose(ConnectionType hdl){
    if(hdl.lock().get()==_ppConnection.lock().get()){
        _ppConnection = hdl;
        _clientStatus = ConnectState::Close;
    }
}
void easy_client::onMessage(ConnectionType hdl, ClientType::message_ptr msg){
    //std::cout << "onMessage" << std::endl;
    std::unique_lock<std::mutex> lock(_actionLock);
    _messages.push_back(msg->get_payload());
    _actionCondition.notify_one();
}

void easy_client::process()
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

void easy_client::dealMsg(std::string &msg)
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
            if(fPusherHandle!=0){
                pusher(rdata);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

void easy_client::stop()
{
    _isProcessing = false;
    _firstFrame = false;
    _easy_client.stop_perpetual();
    websocketpp::lib::error_code ec;
    _easy_client.close(_ppConnection, websocketpp::close::status::going_away, "", ec);
    m_thread->join();
    delete m_thread.get();
    _thread->join();
    if(_thread) delete _thread;
}

void easy_client::runOnce()
{
    if(messageCount<=0){
        std::cout  << "runOnce:topic "<< _topic << " no message" << std::endl;
    }
    else{
        if(isNewConnect){
            std::cout << "runOnce:again get new message " << std::endl;
            openRtsp();
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

void easy_client::registerTopic(const std::string &topic,const std::string &id, const std::string& type, int throttle_rate, int queue_length)
{
    //Assembly data
    std::string message = "\"op\":\"subscribe\", \"topic\":\"" + topic + "\",\"ip\":\"192.168.1.11\"";
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

void easy_client::openRtsp()
{
    if(fPusherHandle==0){
        //init sps pps
        uint8_t sps[] = {0x67,0x4d,0x0,0x1f,0x8d,0x8d,0x40,0x28,0x2,0xdd,0x8,0x0,0x0,0x38,0x40,0x0,0xa,0xfc,0x80,0x20};
        uint8_t pps[] = {0x68,0xee,0x38,0x80};
        //init pusher
        memset(&mediainfo, 0x00, sizeof(EASY_MEDIA_INFO_T));
        mediainfo.u32VideoCodec =   EASY_SDK_VIDEO_CODEC_H264;
	    mediainfo.u32VideoFps = 25;
        mediainfo.u32SpsLength = std::end(sps)-std::begin(sps);
	    mediainfo.u32PpsLength = std::end(pps)-std::begin(pps);
	    memcpy(mediainfo.u8Sps, sps, mediainfo.u32SpsLength);
	    memcpy(mediainfo.u8Pps, pps, mediainfo.u32PpsLength);
        fPusherHandle = EasyPusher_Create();
        auto f = [](int _id, EASY_PUSH_STATE_T _state, EASY_AV_Frame *_frame, void *_userptr)->int{
            
            if (_state == EASY_PUSH_STATE_CONNECTING) printf("Connecting...\n");
            else if (_state == EASY_PUSH_STATE_CONNECTED)           printf("Connected\n");
            else if (_state == EASY_PUSH_STATE_CONNECT_FAILED)      printf("Connect failed\n");
            else if (_state == EASY_PUSH_STATE_CONNECT_ABORT)       printf("Connect abort\n");
	        else if (_state == EASY_PUSH_STATE_PUSHING)             printf("Pushing to rtsp");
            else if (_state == EASY_PUSH_STATE_DISCONNECTED)        printf("Disconnect.\n");
            
            return 0;
        };
        int(*cb)(int _id, EASY_PUSH_STATE_T _state, EASY_AV_Frame *_frame, void *_userptr) = f; 
        EasyPusher_SetEventCallback(fPusherHandle, cb, 0, NULL);
        EasyPusher_StartStream(fPusherHandle, const_cast<char*>(config_ip.c_str()), config_port, const_cast<char*>(config_name.c_str()), EASY_RTP_OVER_TCP, "admin", "admin", &mediainfo, 1024, false);
        printf("*** live streaming url:rtsp://%s:%d/%s ***\n", config_ip.c_str(), config_port, config_name.c_str());
    }
}

void easy_client::setRtsp(std::string url, int width, int height)
{
    _rtspUrl = url;
    _width = width;
    _height = height;
}

void easy_client::pusher(std::vector<char> &data){
    if(fPusherHandle!=0){
        std::cout << "ready pusher ..." << std::endl;
        if(data.size()>5)
		{
            bool bKeyFrame = false;
            unsigned char naltype =  (unsigned char)(data[4] & 0x1F);
            if (naltype==0x07 || naltype==0x05) bKeyFrame = true;
            struct timeval time;
            gettimeofday(&time, NULL);
            printf("s: %ld, ms: %ld\n", time.tv_sec, (time.tv_sec*1000 + time.tv_usec/1000));
            if (bKeyFrame)//I֡
			{
                std::cout << "is I frame " << std::endl;
                int prefix_size = mediainfo.u32PpsLength + mediainfo.u32SpsLength + 8;
                int result_size = data.size() + prefix_size;
                unsigned char *ptr = new unsigned char [result_size];
                
				memmove(ptr+prefix_size, (unsigned char*)&(data[0]), data.size());
				memcpy(ptr, btHeader, 4);
				memcpy(ptr+4, mediainfo.u8Sps, mediainfo.u32SpsLength);
				memcpy(ptr+4+mediainfo.u32SpsLength, btHeader, 4);
				memcpy(ptr+4+mediainfo.u32SpsLength+4, mediainfo.u8Pps, mediainfo.u32PpsLength);

			    EASY_AV_Frame  avFrame;
			    memset(&avFrame, 0x00, sizeof(EASY_AV_Frame));
			    avFrame.u32AVFrameLen = result_size;
			    avFrame.pBuffer = (unsigned char*)ptr;
			    avFrame.u32VFrameType = (bKeyFrame)?EASY_SDK_VIDEO_FRAME_I:EASY_SDK_VIDEO_FRAME_P;
			    avFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
			    avFrame.u32TimestampSec = time.tv_sec;
			    avFrame.u32TimestampUsec = time.tv_usec;
			    EasyPusher_PushFrame(fPusherHandle, &avFrame);
			}else{
                std::cout << "is P/B frame " << std::endl;
                int prefix_size = 3;
                int result_size = data.size() + prefix_size;
                unsigned char *ptr = new unsigned char [result_size];
                memmove(ptr+prefix_size, (unsigned char*)&(data[0]), data.size());
				memcpy(ptr, bpHeader, prefix_size);

                EASY_AV_Frame  avFrame;
			    memset(&avFrame, 0x00, sizeof(EASY_AV_Frame));
			    avFrame.u32AVFrameLen = result_size;
			    avFrame.pBuffer = (unsigned char*)ptr;
			    avFrame.u32VFrameType = (bKeyFrame)?EASY_SDK_VIDEO_FRAME_I:EASY_SDK_VIDEO_FRAME_P;
			    avFrame.u32AVFrameFlag = EASY_SDK_VIDEO_FRAME_FLAG;
			    avFrame.u32TimestampSec = time.tv_sec;
			    avFrame.u32TimestampUsec = time.tv_usec;
			    EasyPusher_PushFrame(fPusherHandle, &avFrame);
            }
  
		}
    }
}
