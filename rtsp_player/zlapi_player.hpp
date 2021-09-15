#ifndef _ZLAPI_PLAYER_H_
#define _ZLAPI_PLAYER_H_
#include <iostream>
#include <thread>

#include "mk_events.h"
#include "mk_player.h"
#include "mk_common.h"
class ZLMPLAYER
{
public:
    ZLMPLAYER();
    ~ZLMPLAYER();

    //初始化MediaSource,推流使用 codec_id 0 h264
    void ZLM_PlayerInit(const std::string &url);
    std::string ZLM_GetPushUrl();
    // void API_CALL MKPlayCB(void *user_data, int err_code, const char *err_msg);

private:
    mk_player _mk_player;
    std::string _player_url;
};

#endif