/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-02-26 13:03:27
 * @LastEditors: li
 * @LastEditTime: 2021-03-03 18:41:08
 */
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>
#include <sys/time.h>
#include "easy_client.hpp"
#include "EasyPusherAPI.h"

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
    std::string rtsp_name;
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
        printf("Failed to open %s", jsonFile.c_str());
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
            else if (key == "rtsp_name")
            {
                printf("rtsp_name %s \n", itr2->value.GetString());
                node.rtsp_name = itr2->value.GetString();
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
    std::vector<std::shared_ptr<easy_client>> clients;
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
            auto client = std::make_shared<easy_client>(node.ws_ip, node.ws_port,node.topic);
            client->config_ip = node.rtsp_ip;
            client->config_port = node.rtsp_port;
            client->config_name = node.rtsp_name;
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