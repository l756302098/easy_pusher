/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-16 15:08:06
 * @LastEditors: li
 * @LastEditTime: 2021-03-19 11:43:36
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

bool isRunning;			        //Default Topic

void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    isRunning = false;
    // 清理并关闭
    exit(signum);
}

struct StreamNode
{
    std::string rtsp_ip;
    int rtsp_port;
    std::string rtsp_app,rtsp_stream;
    std::string ws_ip,core_ip;
    int ws_port;
    std::string client_name;
    std::string topic;
    int width;
    int height;

    StreamNode()
    {
    }
};

bool loadJson(const std::string &jsonFile, std::vector<StreamNode> &nodes)
{
    std::ifstream in(jsonFile.c_str());
    if (!in)
    {
        printf("Failed to open %s \n", jsonFile.c_str());
        return false;
    }

    rapidjson::IStreamWrapper isw(in);
    rapidjson::EncodedInputStream<rapidjson::UTF8<>, rapidjson::IStreamWrapper> eis(isw);
    rapidjson::Document doc;
    if (doc.ParseStream(eis).HasParseError())
    {
        printf("Failed to parse json in %s:", jsonFile.c_str());
        return false;
    }

    if (!doc.HasMember("streams"))
    {
        printf("No 'scene' field in %s:", jsonFile.c_str());
        return false;
    }

    const rapidjson::Value &streams = doc["streams"];
    if (!streams.IsObject())
    {
        printf("Invalid 'scene' field in %s:", jsonFile.c_str());
        return false;
    }
    for (rapidjson::Value::ConstMemberIterator itr = streams.MemberBegin();
         itr != streams.MemberEnd(); ++itr)
    {
        std::string stationName = itr->name.GetString();
        printf("station %s \n", stationName.c_str());
        const rapidjson::Value &msg = itr->value;
        if (!msg.IsObject())
        {
            continue;
        }
        StreamNode node;
        node.client_name = stationName;
        for (rapidjson::Value::ConstMemberIterator itr2 = msg.MemberBegin();
             itr2 != msg.MemberEnd(); ++itr2)
        {
            std::string key = itr2->name.GetString();
            //std::cout << "key:" << key << std::endl;
            if (key == "rtsp_ip")
            {
                printf("rtsp_ip %s \n", itr2->value.GetString());
                node.rtsp_ip = itr2->value.GetString();
            }
            else if (key == "rtsp_port")
            {
                printf("rtsp_port %i \n", itr2->value.GetInt());
                node.rtsp_port = itr2->value.GetInt();
            }
            else if (key == "rtsp_app")
            {
                printf("rtsp_name %s \n", itr2->value.GetString());
                node.rtsp_app = itr2->value.GetString();
            }
            else if (key == "rtsp_stream")
            {
                printf("rtsp_name %s \n", itr2->value.GetString());
                node.rtsp_stream = itr2->value.GetString();
            }
            else if (key == "core_ip")
            {
                printf("core_ip %s \n", itr2->value.GetString());
                node.core_ip = itr2->value.GetString();
            }
            else if (key == "ws_ip")
            {
                printf("ws_ip %s \n", itr2->value.GetString());
                node.ws_ip = itr2->value.GetString();
            }
            else if (key == "ws_port")
            {
                printf("ws_port %i \n", itr2->value.GetInt());
                node.ws_port = itr2->value.GetInt();
            }
            else if (key == "topic")
            {
                printf("topic %s \n", itr2->value.GetString());
                node.topic = itr2->value.GetString();
            }
        }
        nodes.push_back(node);
    }
    return true;
}

int main(int argc, char **argv)
{
    std::string config;
    std::vector<StreamNode> nodes;
    std::vector<std::shared_ptr<push_node>> clients;
    if (argc > 1)
    {
        config = argv[1];
        std::cout << "config:" << config << std::endl;
    }
    else
    {
        boost::filesystem::path currentPath = boost::filesystem::current_path();
        currentPath = currentPath / "/config.json";
        config = currentPath.string();
        std::cout << "config:" << config << std::endl;
    }
    try
    {
        if (!loadJson(config, nodes))
        {
            std::cout << "config.json does not exist or is empty " << std::endl;
            return 0;
        }
        isRunning = true;
        signal(SIGINT, signalHandler);
        size_t len = nodes.size();
        for (size_t i = 0; i < len; i++)
        {
            StreamNode &node = nodes[i];
            auto client = std::make_shared<push_node>(node.ws_ip,node.core_ip, node.ws_port,node.topic);
            client->initPusher(node.rtsp_ip,node.rtsp_app,node.rtsp_stream);
            clients.push_back(client);
        }
        while (isRunning)
        {
            for (auto &client : clients)
            {
                client->runOnce();
            }
            sleep(1);
        }
        for (auto &client : clients)
        {
            client->stop();
        }
    }
    catch (std::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
    return 0;
}