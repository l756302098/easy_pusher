#include "zlapi_player.hpp"
#include <bits/stdc++.h>

void API_CALL MKPlayCB(void *user_data, int err_code, const char *err_msg)
{
    printf("err_code:%d err_msg:%s\n", err_code, err_msg);
}

void API_CALL MKPlayDataCB(void *user_data, int track_type, int codec_id, void *data, size_t len, uint32_t dts, uint32_t pts)
{
    //printf("err_code:%d err_msg:%s\n", err_code, err_msg);
    //printf("len:%i dts:%i pts:%i \n", len, dts, pts);
    std::cout << "len:" << len << " dts:" << dts << " pts:" << pts << std::endl;
}

ZLMPLAYER::ZLMPLAYER()
{
}

ZLMPLAYER::~ZLMPLAYER()
{
}

void ZLMPLAYER::ZLM_PlayerInit(const std::string &url)
{
    std::cout << "mk_player_create:" << url << std::endl;
    _mk_player = mk_player_create();
    //set_option
    //mk_player_set_option
    // set callback
    mk_player_set_on_result(_mk_player, MKPlayCB, this);
    mk_player_set_on_data(_mk_player, MKPlayDataCB, this);
    mk_player_play(_mk_player, url.c_str());
}

std::string ZLMPLAYER::ZLM_GetPushUrl()
{
    return "";
}
