/*
 * @Descripttion: 
 * @version: 
 * @Author: li
 * @Date: 2021-03-19 09:53:57
 * @LastEditors: li
 * @LastEditTime: 2021-03-19 11:36:23
 */
#include "zlapi_worker.hpp"

void API_CALL PushResultCB(void *user_data, int err_code, const char *err_msg)
{
	printf("err_code:%d err_msg:%s\n", err_code,err_msg);
}

void API_CALL MediasourceRegistCB(void *user_data, mk_media_source sender, int regist)
{
	printf("mk_media_source:%x\n", sender);
	printf("regist type:%x\n", regist);
	ZLMWorker* ptr = (ZLMWorker*)user_data;
	if (regist && !ptr->ZLM_GetPushHandle())
	{
		ptr->ZLM_PusherInit();
		ptr->ZLM_PusherStart(ptr->ZLM_GetPushUrl().c_str());
	}
}

void API_CALL StreamDataCB(void *user_data, int track_type, int codec_id, void *data, int len, uint32_t dts, uint32_t pts)
{
	//忽略音频
	if (track_type != 0)
	{
		return;
	}

	ZLMWorker* ptr = (ZLMWorker*)user_data;
	mk_media pMedia = ptr->ZLM_GetMediaHandle();
	mk_media_input_h264(pMedia, data, len, dts, pts);
	return;
}

ZLMWorker::ZLMWorker()
{
}

ZLMWorker::~ZLMWorker()
{
	if(_mk_MediaHandle)
		mk_media_release(_mk_MediaHandle);
	if(_mk_PusherHandle)
		mk_pusher_release(_mk_PusherHandle);
}

void ZLMWorker::ZLM_MediaInit(const std::string &schema,
							const std::string &host,
							const std::string &app,
							const std::string &stream,
							int codec_id, 
							int width, 
							int height, 
							float fp)
{
	_schema = schema;
	_host = host;
	_app = app;
	_stream = stream;
	std::cout << "mk_pusher_create:" << _schema << "://" << _host << "/" << _app << "/" << _stream << std::endl;
	_mk_MediaHandle = mk_media_create(host.c_str(),_app.c_str(),_stream.c_str(),0, false,false);
	mk_media_init_video(_mk_MediaHandle, codec_id, width, height, fp);
	mk_media_set_on_regist(_mk_MediaHandle, MediasourceRegistCB, this);
	mk_media_init_complete(_mk_MediaHandle);
}

void ZLMWorker::ZLM_Input(char *data, int len){
	if(_mk_MediaHandle)
		mk_media_input_h264(_mk_MediaHandle,data,len,0,0);
}

mk_media ZLMWorker::ZLM_GetMediaHandle()
{
	return _mk_MediaHandle;
}

void ZLMWorker::ZLM_PusherInit()
{
	std::cout << "mk_pusher_create:" << _schema << "://" << _host << "/" << _app << "/" << _stream << std::endl;
	//_mk_PusherHandle = mk_pusher_create("rtmp", "__defaultVhost__", "live", "camera1");
	_mk_PusherHandle = mk_pusher_create(_schema.c_str(),_host.c_str(),_app.c_str(),_stream.c_str());
	mk_pusher_set_on_result(_mk_PusherHandle, PushResultCB, this);
}

void ZLMWorker::ZLM_PusherStart(std::string strPushUrl)
{
	mk_pusher_publish(_mk_PusherHandle, strPushUrl.c_str());
}

void ZLMWorker::ZLM_SetPushUrl(std::string strPushUrl)
{
	_strPushUrl = strPushUrl;
}

std::string  ZLMWorker::ZLM_GetPushUrl()
{
	return _strPushUrl;
}
