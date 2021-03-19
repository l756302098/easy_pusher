/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-16 15:54:56
 * @LastEditors: li
 * @LastEditTime: 2021-03-16 16:07:28
 */
#include <iostream>
#include "string.h"

namespace monibuca{
    struct VideoInfo
    {
        int width;
        int height;
        float fps;
    };
}

struct InitParam
{
    /* data */
    std::string url;
    std::string vhost;
    std::string app;
    std::string stream;
};



