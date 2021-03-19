/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-19 09:53:57
 * @LastEditors: li
 * @LastEditTime: 2021-03-19 10:40:42
 */
#ifndef _ZLAPIWORKER_H_
#define _ZLAPIWORKER_H_
#include <iostream>
#include <thread>

#include "mk_events.h"
#include "mk_media.h"
#include "mk_pusher.h"
#include "mk_common.h"
class ZLMWorker
{
public:
	ZLMWorker();
	~ZLMWorker();

	//初始化MediaSource,推流使用 codec_id 0 h264
	void ZLM_MediaInit(const std::string &schema,
							const std::string &host,
							const std::string &app,
							const std::string &stream,
							int codec_id = 0, 
							int width = 1920, 
							int height = 1080, 
							float fp = 25);
	mk_media ZLM_GetMediaHandle();
	mk_pusher ZLM_GetPushHandle(){return _mk_PusherHandle;};
	//初始化推流器
	void ZLM_PusherInit();
	void ZLM_PusherStart(std::string strPushUrl);
	void ZLM_SetPushUrl(std::string strPushUrl);
	//input
	void ZLM_Input(char *data, int len);
	std::string  ZLM_GetPushUrl();
	std::string get_schema(){return _schema;};
	std::string get_app(){return _app;};
	std::string get_host(){return _host;};
	std::string get_stream(){return _stream;};
private:
	mk_media  _mk_MediaHandle;
	mk_pusher _mk_PusherHandle;

	std::string _strPushUrl;
	std::string _schema,_host,_app,_stream;
};

#endif