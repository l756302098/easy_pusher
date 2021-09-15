#include <iostream>
#include "zlapi_player.hpp"
#include <unistd.h>
using namespace std;
int main()
{
    int x;
    float y;
    cout << "Please input an int number:" << endl;
    cin >> x;
    cout << "The int number is x= " << x << endl;
    cout << "Please input a float number:" << endl;
    cin >> y;
    cout << "The float number is y= " << y << endl;
    ZLMPLAYER player;
    std::string rtsp_url = "rtsp://admin:123qweasd@192.168.1.65:554/h264/ch1/main/av_stream";
    player.ZLM_PlayerInit(rtsp_url);
    while (1)
    {
        usleep(1000000);
    }

    return 0;
}