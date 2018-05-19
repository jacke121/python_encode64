#include "stdafx.h"
#include "CVdll.h"
#include "SimpleLog.h"
#include <iostream>
#include<fstream>
#include <sys/types.h>  
#include "opencv2/opencv.hpp"


#include "Ws2tcpip.h"


#include <winsock2.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <signal.h>
#pragma comment(lib,"ws2_32.lib")
#include <queue> 
using namespace cv;
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/log.h"
#include <libavutil\imgutils.h>   

	//#include "libavutil/imgutils.h"
};


//说明，动态库需要拷贝三个文件，否则重连会出问题


char* testchar(int plus1) {


	char* str = "hello world111111";
	return str;
}
char* testimg(char* data, int length) {


	char* str = "hello world111111";
	return str;
}



int outbuf_size = 100000;




class Rtmp_tool {
public:
	int nWidth = 0;
	int nHeight = 0;
	AVCodecContext *c;
	AVFrame *m_pRGBFrame = new AVFrame[1];  //RGB帧数据      
	AVFrame *m_pYUVFrame = new AVFrame[1];;  //YUV帧数据   
	uint8_t * yuv_buff;//  
	uint8_t * outbuf;
	SwsContext * scxt;
	FILE *f = NULL;


};
void* pre_encode(int width, int height) {

	av_log_set_level(AV_LOG_ERROR);
	Rtmp_tool *rtmp_tool;
	rtmp_tool = new Rtmp_tool();
	int nLen;
	int fileI;
	rtmp_tool->nWidth = width;
	rtmp_tool->nHeight = height;


	av_register_all();
	avcodec_register_all();
	//AVFrame *m_pRGBFrame = new AVFrame[1];  //RGB帧数据      
	//AVFrame *m_pYUVFrame = new AVFrame[1];;  //YUV帧数据    
	AVCodecContext *c = NULL;
	AVCodecContext *in_c = NULL;
	AVCodec *pCodecH264; //编码器    

						 //查找h264编码器    
	pCodecH264 = avcodec_find_encoder(AV_CODEC_ID_H264);

	

	c = avcodec_alloc_context3(pCodecH264);
	c->bit_rate = 3000000;// put sample parameters     
	c->width = width;//     
	c->height = height;//     


					   // frames per second     
	AVRational rate;
	rate.num = 1;
	rate.den = 25;
	c->time_base = rate;//(AVRational){1,25};    
	c->gop_size = 10; // emit one intra frame every ten frames     
	c->max_b_frames = 1;
	c->thread_count = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;//PIX_FMT_RGB24;    


									//av_opt_set(c->priv_data, /*"preset"*/"libvpx-1080p.ffpreset", /*"slow"*/NULL, 0);    
									//打开编码器    
	if (avcodec_open2(c, pCodecH264, NULL)<0)
		printf("不能打开编码库");


	int size = c->width * c->height;


	rtmp_tool->yuv_buff = (uint8_t *)malloc((size * 3) / 2); // size for YUV 420     




															 //图象编码    


	rtmp_tool->outbuf = (uint8_t*)malloc(outbuf_size);
	int u_size = 0;

	const char * filename = "0_Data.h264";
	rtmp_tool->f = fopen(filename, "wb");
	if (!rtmp_tool->f)
	{
		printf("could not open %s\n", filename);
		exit(1);
	}

	printf("c w h %d %d\n", c->width, c->height);
	//初始化SwsContext    
	rtmp_tool->scxt = sws_getCachedContext(rtmp_tool->scxt,c->width, c->height, AV_PIX_FMT_BGR24, c->width, c->height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);
	//rtmp_tool->scxt = sws_getCachedContext(rtmp_tool->scxt, c->width, c->height, AV_PIX_FMT_BGR24, c->width, c->height, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL);
	//sws_context = sws_getCachedContext(sws_context, c->width, c->height, AV_PIX_FMT_RGB24, c->width, c->height, AV_PIX_FMT_YUV420P, 0, 0, 0, 0);

	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		fprintf(stderr, "Could not allocate video frame\n");
		exit(1);
	}
	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;
	int ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height, c->pix_fmt, 32);
	if (ret < 0) {
		fprintf(stderr, "Could not allocate raw picture buffer\n");
		exit(1);
	}
	rtmp_tool->m_pYUVFrame = frame;
	rtmp_tool->c = c;
	return rtmp_tool;
}


char* push_rtsp(int* plus1, int len, void* vp) {
	Rtmp_tool *rtmp_tool = (Rtmp_tool *)vp;
	av_log_set_level(AV_LOG_DEBUG);

	for (int i = 0; i < len; i++) {
		plus1[i] = (uint8_t)plus1[i];
	}



	AVCodecContext *c = rtmp_tool->c;// (AVCodecContext*)vp;
	printf("2 %d %d\n", c->width, c->height);
	//---------------
	AVPacket avpkt;
	AVFrame *m_pRGBFrame = rtmp_tool->m_pRGBFrame;
	AVFrame *m_pYUVFrame = rtmp_tool->m_pYUVFrame;
	/*unsigned char *pBmpBuf;
	pBmpBuf = new unsigned char[len];*/

	//memcpy(rgb_buff, (uint8_t*)plus1, nDataLen);
	//
	avpicture_fill((AVPicture*)m_pRGBFrame, (uint8_t*)plus1, AV_PIX_FMT_RGB24, rtmp_tool->nWidth, rtmp_tool->nHeight);
	//m_pRGBFrame->linesize[0] = c->width * 3;
	//m_pRGBFrame->linesize[1] = 0;
	//m_pRGBFrame->linesize[2] = 0;
	//m_pRGBFrame->linesize[3] = 0;
	//m_pRGBFrame->format = AV_PIX_FMT_RGB24;
	m_pRGBFrame->width = rtmp_tool->nWidth;
	m_pRGBFrame->height = rtmp_tool->nHeight;


	uint8_t *p = m_pRGBFrame->data[0];
	int y = 0, x = 0;
	for (y = 0; y < rtmp_tool->nHeight; y++) {
		for (x = 0; x < rtmp_tool->nWidth; x++) {
			*p++ = (uint8_t)plus1[(y*rtmp_tool->nWidth + x) * 3]; // R
			*p++ = (uint8_t)plus1[(y*rtmp_tool->nWidth + x) * 3 + 1]; // G
			*p++ = (uint8_t)plus1[(y*rtmp_tool->nWidth + x) * 3 + 2]; // B
		}
	}
	printf("1 %d %d \n", rtmp_tool->nWidth, rtmp_tool->nHeight);
	//将YUV buffer 填充YUV Frame    
	avpicture_fill((AVPicture*)m_pYUVFrame, (uint8_t*)rtmp_tool->yuv_buff, AV_PIX_FMT_YUV420P, rtmp_tool->nWidth, rtmp_tool->nHeight);
	//av_image_fill_arrays(m_pYUVFrame->data, m_pYUVFrame->linesize, (uint8_t*)rtmp_tool->yuv_buff, AV_PIX_FMT_RGBA, c->width, c->height, 1);

	// 翻转RGB图像    
	//m_pRGBFrame->data[0] += m_pRGBFrame->linesize[0] * (rtmp_tool->nHeight - 1);
	//m_pRGBFrame->linesize[0] *= -1;
	//m_pRGBFrame->data[1] += m_pRGBFrame->linesize[1] * (rtmp_tool->nHeight / 2 - 1);
	//m_pRGBFrame->linesize[1] *= -1;
	//m_pRGBFrame->data[2] += m_pRGBFrame->linesize[2] * (rtmp_tool->nHeight / 2 - 1);
	//m_pRGBFrame->linesize[2] *= -1;

	//-------------	
	const int in_linesize[1] = { 3 * c->width };
	sws_scale(rtmp_tool->scxt, (const uint8_t * const *)m_pRGBFrame->data, in_linesize, 0, c->height, m_pYUVFrame->data, m_pYUVFrame->linesize);


	//将RGB转化为YUV    
	//sws_scale(rtmp_tool->scxt, m_pRGBFrame->data, m_pRGBFrame->linesize, 0, c->height, m_pYUVFrame->data, m_pYUVFrame->linesize);


	int got_packet_ptr = 0;
	av_init_packet(&avpkt);
	/*avpkt.data = rtmp_tool->outbuf;
	avpkt.size = outbuf_size;
*/
	avpkt.data = NULL;
	avpkt.size = 0;
	if (m_pYUVFrame->pts %10==0) {
		m_pYUVFrame->key_frame = 1;
		m_pYUVFrame->pict_type = AV_PICTURE_TYPE_I;
	}
	else {
		m_pYUVFrame->key_frame = 0;
		m_pYUVFrame->pict_type = AV_PICTURE_TYPE_P;
	}
	int u_size = avcodec_encode_video2(c, &avpkt, m_pYUVFrame, &got_packet_ptr);

	//int send_ret= avcodec_send_frame(c, m_pYUVFrame);

	//printf("avcodec_send_frame %d\n", send_ret);

	//int u_size= avcodec_receive_packet(c, &avpkt);
	m_pYUVFrame->pts++;
	if (u_size == 0) {
		int res = fwrite(avpkt.data, 1, avpkt.size, rtmp_tool->f);
		if (res == 0) {
			printf("000");
		}
		else {
			printf("fwrite ok %d %d \n", res, avpkt.size);
		}


	}
	else {

		if (u_size == AVERROR(EAGAIN)) {//-11
			printf("avcodec_encode_video2 AVERROR(EAGAIN)\n");
		}
		else if (u_size == AVERROR(EINVAL)) {//-22
			printf("avcodec_encode_video2 AVERROR(EINVAL)\n");
		}
		else {
			printf("avcodec_encode_video2 %d\n", u_size);
		}
		
	}
	//-------end---------




	//Mat mat;
	////加载图片  
	//mat = imread("bgs.jpg", CV_LOAD_IMAGE_COLOR);
	//printf("a %d %d", mat.rows, mat.cols);
	////if (!mat.empty()) {


	//int m, n;
	//n = mat.cols * 3;
	//m = mat.rows;
	//unsigned char *data = (unsigned char*)malloc(sizeof(unsigned char) * m * n);
	//int p = 0;
	//for (int i = 0; i < m; i++)
	//{
	//	for (int j = 0; j < n; j++)
	//	{
	//		data[p] = mat.at<unsigned char>(i, j);
	//		p++;
	//	}
	//}
	//*plus1 = p;
	return NULL;
	//return (char*)data;
}


struct RecStruct //数据包  
{
	int size;
	int data_type;
	int cam_no;
	int error_code;
	char recvbuf[1500];
};
struct SendStcuct
{
	int size;
	int data_type;
	int cam_no;
	char sendbuf[1000];
}data_send;






static ErrorCallBack g_errorcall = 0;
static CamInfoCallBack g_caminfocall = 0;


typedef struct CameraInfo
{
	std::ofstream foutV;
	int timeInHour = -1;
}caminfo;
std::map<int, CameraInfo*> cameraMap;
//static std::map<int, queue<RecStruct*>> namemap;


static SOCKET g_sockClient;
HANDLE hMutex;
//char* deviceId;
/**判断str1是否以str2开头
* 如果是返回1
* 不是返回0
* 出错返回-1
* */
int is_begin_with(const char * str1, char *str2)
{
	if (str1 == NULL || str2 == NULL)
		return -1;
	int len1 = strlen(str1);
	int len2 = strlen(str2);
	if ((len1 < len2) || (len1 == 0 || len2 == 0))
		return -1;
	char *p = str2;
	int i = 0;
	while (*p != '\0')
	{
		if (*p != str1[i])
			return 0;
		p++;
		i++;
	}
	return 1;
}


char* Strcpy(char * a, const char * b)
{
	int i = 0;
	while (*b) a[i++] = *b++;
	a[i] = 0;
	return a;
}


int sendcmd(char* data, int cam_no, int type, int size) {
	memset(data_send.sendbuf, 0, 1000);
	//data2send.sendbuf = new char[strlen(data)];
	//memset(data2send.sendbuf, 0, strlen(data));
	data_send.size = size;
	data_send.data_type = type;
	data_send.cam_no = cam_no;
	memcpy(data_send.sendbuf, data, sizeof(char) * (size));
	printf("data_send len %d\n", sizeof(data_send));
	if (g_sockClient)
		send(g_sockClient, (char *)&data_send, sizeof(struct SendStcuct), 0);
	return 0;
}
int set_callback(ErrorCallBack terrorcall(int error_type, int cam_no, int err_no, int msg_level, char* msg_txt, int spare)) {
	g_errorcall = (ErrorCallBack)terrorcall;
	return 0;
}
MYLIBDLL int getcameralist(int type, CamInfoCallBack caminfocall(int cam_no, char* cam_info, int cam_info_size)) {
	g_caminfocall = (CamInfoCallBack)caminfocall;
	SendStcuct data_send;
	memset(&data_send, 0, sizeof(struct SendStcuct));
	data_send.size = 20;
	data_send.data_type = 1;
	data_send.cam_no = 0;
	char* data = "getcameralist";
	memcpy(data_send.sendbuf, data, sizeof(char) * (20));
	printf("data_send len %d\n", sizeof(data_send));
	if (g_sockClient)
		send(g_sockClient, (char *)&data_send, sizeof(struct SendStcuct), 0);
	return 0;
}
DWORD WINAPI RecvThread(LPVOID lpParameter);
DWORD WINAPI RecvThread(LPVOID lpParameter) {
	SOCKET sockClient = (SOCKET)lpParameter;
	while (1) {
		RecStruct data_recv;
		int ret;
		memset(&data_recv, '0', sizeof(struct RecStruct));
		ret = recv(sockClient, (char *)&data_recv, sizeof(struct RecStruct), 0);  //第二个参数使用强制类型，为一个数据包  
		if (ret == 0) // server调用了close 
		{
			printf("server close");
			break;
		}
		else if (ret == SOCKET_ERROR) // 网络错误 
		{
			int err = WSAGetLastError();
			printf("get message %d %d %d \n", ret, SOCKET_ERROR, err);
			if (err == WSAECONNRESET || err == WSAECONNABORTED) {
				printf("tcp error %d %d \n", err, SOCKET_ERROR);
				//int n = namemap.erase(deviceId);//如果删除了会返回1，否则返回0  
			}
			break;


		}
		//printf("reve type %d %d", data_recv.data_type, data_recv.size);
		switch (data_recv.data_type)
		{
		case 1://摄像头列表
		{
			g_caminfocall(data_recv.cam_no, data_recv.recvbuf, data_recv.size);
		}
		break;
		case 3://异常信息
		{
			if (g_errorcall != 0)
				g_errorcall(1, 1, data_recv.error_code, 4, NULL, 0);
			break;
		}
		case 2:
		{
			char* recemsg = data_recv.recvbuf;
			int is_null = is_begin_with(recemsg, "00000");
			if (is_null == 1) {
				printf("recv type 2 00000");
				continue;
			}

			break;
		}
		default:
			break;
		}


		if (ret < 0) {
			printf("WSAStartup() failed!\n");
			return -1;
		}
		Sleep(20);


	}


	return 0;
}






int tcpInit(char* ip, int port)
{
	av_log_set_level(AV_LOG_PANIC);
	WSADATA wsaData;


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("初始化Winsock失败");
		return -1;
	}


	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);


	int nRecvBuf = 0;//设置为32K
	setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
	//setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (char *)&nZero, sizeof(nZero));
	inet_pton(AF_INET, ip, &addrSrv.sin_addr.s_addr);
	if (connect(sockClient, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) == -1)
		return -2;
	//throw "连接失败";
	if (SOCKET_ERROR == sockClient) {
		printf("Socket() error:%d", WSAGetLastError());
		return -3;
	}
	g_sockClient = sockClient;
	HANDLE h_thread = CreateThread(NULL, 0, RecvThread, (LPVOID)sockClient, 0, NULL);
	CloseHandle(h_thread);
	return 0;
}


class DeviceInfo {


public:
	string cam_name;
	int cam_no;
	SOCKET sockClient;
};


//Callback  
int read_buffer(void *opaque, uint8_t *buf, int buf_size) {


	DeviceInfo deviceInfo = *((DeviceInfo *)opaque);

	int null_count = 0;
	int display_count = 0;
	while (1) {
		RecStruct data_recv;
		int ret;
		memset(&data_recv, '0', sizeof(struct RecStruct));


		ret = recv(deviceInfo.sockClient, (char *)&data_recv, sizeof(struct RecStruct), 0);  //第二个参数使用强制类型，为一个数据包  
		if (ret == 0) // server调用了close 
		{
			printf("server close");
			break;
		}
		else if (ret == SOCKET_ERROR) // 网络错误 
		{
			printf("get message %d %d \n", ret, SOCKET_ERROR);

			int err = WSAGetLastError();
			if (g_errorcall != 0)
				g_errorcall(1, deviceInfo.cam_no, err, 4, "socket err", 0);
			//if (err == WSAECONNRESET || err == WSAECONNABORTED) {
			//    printf("server break %s", deviceId);
			//    //int n = namemap.erase(deviceId);//如果删除了会返回1，否则返回0  
			//}
			break;


		}
		if (data_recv.size == 0) {
			null_count++;
			if (null_count % 1000 == 0) {
				if (g_errorcall != 0)
					g_errorcall(1, deviceInfo.cam_no, 0, 2, "data_recv 0", 11);
				printf("reve len=0 type %d\n", data_recv.data_type);
				null_count = 0;
			}
			Sleep(2);
			continue;
		}
		else if (data_recv.size >1500) {
			if (g_errorcall != 0)
				g_errorcall(1, deviceInfo.cam_no, 0, 2, "data_recv too long", data_recv.size);
			printf("reve data too long %d\n", data_recv.size);
			continue;
		}
		if (data_recv.data_type == 3)
		{
			if (g_errorcall) {
				char err_str[10];
				_itoa(data_recv.error_code, err_str, 10); //正确解法一
				g_errorcall(1, deviceInfo.cam_no, data_recv.error_code, 4, err_str, 0);
			}
		}
		else if (data_recv.data_type == 2)
		{
			null_count = 0;
			display_count++;
			char* recemsg = data_recv.recvbuf;
			int is_null = is_begin_with(recemsg, "00000");
			if (is_null == 1) {
				printf("recv 00000");
				continue;
			}
			//printf("cam_no  %d", data_recv.cam_no);
			//int cam_no = data_recv.cam_no;
			buf_size = data_recv.size;
			memcpy(buf, data_recv.recvbuf, buf_size);


			if (g_errorcall && buf_size>1000 && display_count % 20 == 0) {
				g_errorcall(2, deviceInfo.cam_no, 1, 0, "rece data", 1);//err_type, cam_no, column, msg_level, msg_txt, spare
				display_count = 0;
			}
			//保存流数据并分小时存储
			time_t tt = time(NULL);//这句返回的只是一个时间cuo
			tm* t = localtime(&tt);
			auto iter = cameraMap.find(deviceInfo.cam_no);
			if (iter != cameraMap.end()) {
				iter->second->foutV.write(data_recv.recvbuf, data_recv.size);
				if (t->tm_min == 0 && (iter->second->timeInHour != t->tm_hour)) {
					//判断间隔一小时
					iter->second->timeInHour = t->tm_hour;
					iter->second->foutV.close();
					time_t tt = time(NULL);//这句返回的只是一个时间cuo
					tm* t = localtime(&tt);
					char ctmBegin[20];
					strftime(ctmBegin, 20, "/%Y%m%d%H%M", t);
					char str3[80];
					sprintf(str3, "create data:%s%s%s", deviceInfo.cam_name, ctmBegin, ".dat");
					SLOG1(str3);
					printf("%s", deviceInfo.cam_name + std::string(ctmBegin) + ".dat");
					iter->second->foutV.open(deviceInfo.cam_name + std::string(ctmBegin) + ".dat", ios::binary);
				}
			}
			return buf_size;
		}
		if (ret < 0) {
			printf("WSAStartup() failed!\n");
			continue;
			//return 0;
		}
	}
	return 0;
}


int send_cmd(int cam_no, int size, int datatype, char* cam_name, SOCKET& sockClient) {
	SendStcuct data_send;
	memset(&data_send, 0, sizeof(struct SendStcuct));
	data_send.size = size;
	data_send.data_type = datatype;
	data_send.cam_no = cam_no;
	memcpy(data_send.sendbuf, cam_name, sizeof(char) * (size));
	printf("data_send len %d\n", sizeof(data_send));


	send(sockClient, (char *)&data_send, sizeof(struct SendStcuct), 0);
	return 0;
}


int tcp_recv_conn(char* ip, int port, char* cam_name, int size, int cam_no, FrameFunc tcallback(char* a, int size, int cam_no, int height, int width))
{
	WSADATA wsaData;


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("初始化Winsock失败");
		return -1;
	}


	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	SOCKET	sockClient = socket(AF_INET, SOCK_STREAM, 0);


	int nRecvBuf = 0;//设置为32K
	setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
	setsockopt(sockClient, SOL_SOCKET, SO_SNDBUF, (char *)&nRecvBuf, sizeof(int));
	inet_pton(AF_INET, ip, &addrSrv.sin_addr.s_addr);
	if (connect(sockClient, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) == -1)
		return -2;
	//throw "连接失败";
	if (SOCKET_ERROR == sockClient) {
		printf("Socket() error:%d", WSAGetLastError());
		return -3;
	}

	DeviceInfo deviceInfo;
	deviceInfo.cam_no = cam_no;
	deviceInfo.sockClient = sockClient;
	av_register_all();
	unsigned version = avcodec_version();
	//printf("FFmpeg version: %d\n", version);


	//初始化流文件状态
	time_t tt = time(NULL);//这句返回的只是一个时间cuo
	tm* t = localtime(&tt);
	char ctmBegin[20];
	strftime(ctmBegin, 20, "/%Y%m%d%H%M", t);
	caminfo cinfoInstance;
	deviceInfo.cam_name = cam_name;
	//std::string dataName = cam_name;
	cinfoInstance.foutV.open(cam_name + std::string(ctmBegin) + ".dat", ios::binary);
	//判断间隔一小时
	cinfoInstance.timeInHour = t->tm_hour;
	cameraMap[cam_no] = &cinfoInstance;
	char str3[20];
	sprintf(str3, "camno: %d start", cam_no);
	SLOG1(str3);
	AVFormatContext *pFormatCtx;
	int            i, videoindex;
	AVCodecContext  *pCodecCtx;
	AVCodec        *pCodec;
	char filepath[] = "video.264";
	//av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	//patha = "C:\\Users\\sbd01\\Pictures\\ffmpegtest\\Debug\\video.dat";


	//fp_open = fopen(patha.c_str(), "rb+");
	unsigned char *aviobuffer = (unsigned char *)av_malloc(1512);

	send_cmd(cam_no, size, 2, cam_name, sockClient);
	AVIOContext *avio = avio_alloc_context(aviobuffer, 1512, 0, &deviceInfo, read_buffer, NULL, NULL);


	pFormatCtx->pb = avio;
	//if (avformat_open_input(&pFormatCtx, patha.c_str(), NULL, NULL) != 0) {
	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
		printf("Couldn't open input stream %d\n", cam_no);
		return -1;
	}
	printf("camno %d find stream\n", cam_no);
	pFormatCtx->probesize = 1000 * 1024;
	pFormatCtx->max_analyze_duration = 10 * AV_TIME_BASE;


	pCodec = NULL;
	while (pCodec == NULL) {
		printf("%d start find stream info \n", cam_no);
		if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
			printf("Couldn't find stream info %d\n", cam_no);
			goto restart_stream;
			continue;
		}
		videoindex = -1;
		for (i = 0; i < pFormatCtx->nb_streams; i++)
			if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
				if (videoindex == -1) {
					videoindex = i;
				}
				//break;
			}
		if (videoindex == -1) {
			printf("%d Didn't find a video stream.\n", cam_no);
			goto restart_stream;
		}
		pCodecCtx = pFormatCtx->streams[videoindex]->codec;
		//pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);

		pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
		if (pCodec == NULL) {
			printf("%d Codec not found \n", cam_no);
			goto restart_stream;
			//return -1;
		}
		if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
			printf("%d Could not open codec.\n", cam_no);
			goto restart_stream;
			continue;
			//return -1;
		}
		if (pCodecCtx->width <= 0 || pCodecCtx->height <= 0 || pCodecCtx->height >2000 || pCodecCtx->width >3000) {
			printf("cam %d pCodecCtx error 1 width %d height %d ", cam_no, pCodecCtx->width, pCodecCtx->height);
			goto restart_stream;
		}
		goto ok;
	restart_stream:
		printf("%d restart 1  ", cam_no);
		avformat_free_context(pFormatCtx);
		printf("restart 2  ");
		//avformat_close_input(&pFormatCtx);
		pFormatCtx = NULL;
		pFormatCtx = avformat_alloc_context();
		printf("restart 3  ");
		//av_freep(aviobuffer);
		//printf("restart 4");
		aviobuffer = (unsigned char *)av_malloc(1512);
		printf("restart 4  ");
		AVIOContext *avio2 = avio_alloc_context(aviobuffer, 1512, 0, &deviceInfo, read_buffer, NULL, NULL);
		pFormatCtx->pb = avio2;
		pFormatCtx->probesize = 1000 * 1024;
		pFormatCtx->max_analyze_duration = 10 * AV_TIME_BASE;
		if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
			printf("2Couldn't open input stream %d\n", cam_no);
			//return -1;
		}
		printf("restart 5\n");
		pCodec = NULL;
		continue;
	ok:
		break;
	}


	printf("camno:%d code name :%s width %d height %d\n", cam_no, pCodec->name, pCodecCtx->width, pCodecCtx->height);
	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	int ret, got_picture;


	if (g_errorcall) {
		char* cc;
		int length = strlen(pCodec->name);
		cc = new char[length + 1];
		strcpy(cc, pCodec->name);
		g_errorcall(0, cam_no, pCodecCtx->width, pCodecCtx->height, cc, 11);
	}
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
	uint8_t *out_buffer;
	printf("cam %d ready decode 2", cam_no);
	out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)];
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


	//av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 1);
	printf("cam %d ready decode 3", cam_no);
	int dec_error_count = 0;
	int tmp_test = 0;
	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			//tmp_test++;
			if (packet->size < 50) {
				av_free_packet(packet);
				//printf("cam:%d packet is too small %d\n", cam_no, packet->size);
				Sleep(3);
				continue;
			}
			if (g_errorcall != 0)
				g_errorcall(2, deviceInfo.cam_no, 1, 2, "start decode", 3);
			char str_decode[40];
			sprintf(str_decode, "cam %d start decode", cam_no);
			SLOG1(str_decode);
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				dec_error_count++;
				char str3[80];
				sprintf(str3, "%d%s decode_error:%d error_count %d", cam_no, " Decode Error", ret, dec_error_count);
				SLOG1(str3);
				if (g_errorcall != 0)
					g_errorcall(1, deviceInfo.cam_no, 0, 2, str3, 80);
				printf("cam:%d Decode Error got_picture %d decode_error_num %d\n", cam_no, got_picture, dec_error_count);
				if (dec_error_count > 2) {
					dec_error_count = 0;
					// restart ffmpeg
					av_free_packet(packet);


					Sleep(50);
					sws_freeContext(img_convert_ctx);
					img_convert_ctx = NULL;
					printf("cam %d sws_freeContext 1\n", cam_no);
					//av_free(out_buffer);  
					//av_free(pFrameYUV);
					avcodec_close(pCodecCtx);
					//pCodecCtx = NULL;
					if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
						printf("Could not open codec.\n");
						return -1;
					}
					/*pFrame = av_frame_alloc();
					pFrameYUV = av_frame_alloc();*/


					//packet = (AVPacket *)av_malloc(sizeof(AVPacket));
					printf("cam_no %d avcodec_open2 ok width:%d height:%d\n", cam_no, pCodecCtx->width, pCodecCtx->height);
					img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

					char str3[40];
					sprintf(str3, "ffmpeg restart cam %d ", cam_no);
					SLOG1(str3);
					continue;
				}
			}
			if (got_picture) {
				if (g_errorcall != 0)
					g_errorcall(2, deviceInfo.cam_no, 1, 2, "got_picture", 4);
				char str3[40];
				sprintf(str3, "cam %d got_picture", cam_no);
				SLOG1(str3);
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
				/*fwrite(pFrameYUV->data[0], (pCodecCtx->width)*(pCodecCtx->height) * 3, 1, output);*/
				tcallback((char*)pFrameYUV->data[0], pCodecCtx->height * pCodecCtx->width * 3, cam_no, pCodecCtx->height, pCodecCtx->width);
			}
		}
		av_free_packet(packet);
		Sleep(10);
	}
	sws_freeContext(img_convert_ctx);


	//av_free(out_buffer);  
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	return 0;
}


int tcp_init(char* ip, int port) {
	int res = tcpInit(ip, port);
	//printf("conn server\t%d\n", res);
	return res;
}




int ffmpeg_recv(int cam_no, FrameFunc tcallback(char* a, int size, int cam_no, int height, int width))
{
	av_register_all();
	unsigned version = avcodec_version();


	printf("FFmpeg version: %d\n", version);


	AVFormatContext *pFormatCtx;
	int            i, videoindex;
	AVCodecContext  *pCodecCtx;
	AVCodec        *pCodec;
	char filepath[] = "video.264";
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	//string patha = "C:\\Users\\sbd01\\Videos\\video.264";


	//fp_open = fopen(patha.c_str(), "rb+");
	unsigned char *aviobuffer = (unsigned char *)av_malloc(1512);
	AVIOContext *avio = avio_alloc_context(aviobuffer, 1512, 0, &cam_no, read_buffer, NULL, NULL);


	pFormatCtx->pb = avio;
	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
		printf("Could not open codec.\n");
		return -1;
	}
	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();


	int ret, got_picture;


	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));


	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);


	uint8_t *out_buffer;


	out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)];
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);


				/*fwrite(pFrameYUV->data[0], (pCodecCtx->width)*(pCodecCtx->height) * 3, 1, output);*/
				tcallback((char*)pFrameYUV->data[0], pCodecCtx->height * pCodecCtx->width * 3, cam_no, pCodecCtx->height, pCodecCtx->width);
			}
		}
		av_free_packet(packet);
	}
	sws_freeContext(img_convert_ctx);


	//fclose(fp_open);


	//SDL_Quit();


	//av_free(out_buffer);  
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	return 0;
}








//Callback  
int file_buffer(void *opaque, uint8_t *buf, int buf_size) {


	FILE *fp_open = (FILE *)opaque;
	if (!feof(fp_open)) {
		int true_size = fread(buf, 1, buf_size, fp_open);
		return true_size;
	}
	else {
		return -1;
	}


}
int play_file(char* file_name, FrameFunc tcallback(char* a, int size, int num, int height, int width))
{
	av_register_all();
	unsigned version = avcodec_version();


	printf("FFmpeg version: %d\n", version);


	AVFormatContext *pFormatCtx;
	int            i, videoindex;
	AVCodecContext  *pCodecCtx;
	AVCodec        *pCodec;
	char filepath[] = "video.264";
	//av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	string patha = "C:\\Users\\sbd01\\Videos\\video.264";
	//patha = "C:\\Users\\sbd01\\Pictures\\ffmpegtest\\Debug\\video.dat";
	FILE *fp_open = fopen(file_name, "rb+");
	unsigned char *aviobuffer = (unsigned char *)av_malloc(32768);
	//printf("avio_alloc_context %d\n", cam_no);
	AVIOContext *avio = avio_alloc_context(aviobuffer, 32768, 0, (void*)fp_open, file_buffer, NULL, NULL);


	pFormatCtx->pb = avio;
	//if (avformat_open_input(&pFormatCtx, patha.c_str(), NULL, NULL) != 0) {
	if (avformat_open_input(&pFormatCtx, NULL, NULL, NULL) != 0) {
		printf("Couldn't open input stream.\n");
		return -1;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex = -1;
	for (i = 0; i<pFormatCtx->nb_streams; i++)
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	if (videoindex == -1) {
		printf("Didn't find a video stream.\n");
		return -1;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec;
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL) {
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
		printf("Could not open codec.\n");
		return -1;
	}
	AVFrame *pFrame, *pFrameYUV;
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();


	/*if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
	printf("Could not initialize SDL - %s\n", SDL_GetError());
	return -1;
	}*/


	/*int screen_w = 0, screen_h = 0;
	SDL_Surface *screen;
	screen_w = pCodecCtx->width;
	screen_h = pCodecCtx->height;
	screen = SDL_SetVideoMode(screen_w, screen_h, 0, 0);


	if (!screen) {
	printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
	return -1;
	}
	SDL_Overlay *bmp;
	bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);
	SDL_Rect rect;
	rect.x = 0;
	rect.y = 0;
	rect.w = screen_w;
	rect.h = screen_h;*/
	//SDL End------------------------  
	int ret, got_picture;


	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));


	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);


	uint8_t *out_buffer;


	out_buffer = new uint8_t[avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height)];
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);


	while (av_read_frame(pFormatCtx, packet) >= 0) {
		if (packet->stream_index == videoindex) {
			ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			if (ret < 0) {
				printf("Decode Error.\n");
				return -1;
			}
			if (got_picture) {
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);


				/*fwrite(pFrameYUV->data[0], (pCodecCtx->width)*(pCodecCtx->height) * 3, 1, output);*/


				tcallback((char*)pFrameYUV->data[0], pCodecCtx->height * pCodecCtx->width * 3, 1, pCodecCtx->height, pCodecCtx->width);




			}
		}
		av_free_packet(packet);
	}
	sws_freeContext(img_convert_ctx);
	//fclose(fp_open);
	//SDL_Quit();


	//av_free(out_buffer);  
	av_free(pFrameYUV);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	return 0;
}