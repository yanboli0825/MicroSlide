//#define _RELEASE
//#define DBINTERFACEDLL_EXPORTS
//#define _WINDOWS
//#define _USRDLL
//#define NOLFS
//#define _CRT_SECURE_NO_WARNINGS
//#define _WINSOCK_DEPRECATED_NO_WARNINGS


#pragma once
#ifdef DBINTERFACE_EXPORTS
#define DBINTERFACE_API __declspec(dllexport)
#else
#define DBINTERFACE_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <winsock2.h> 
#include <unordered_map>
#include "nlohmann/json.hpp"
#include "ftplib.h"

using json = nlohmann::json;

// 向服务器获取病理数据的请求包
typedef struct getpathologicalinfo {
	std::string pathological_id;			//扫码枪拿到的初始病理号
	std::string slice_id;					//切片号
	std::string sensor_id;					//传感器编号
	std::string tcp_type="A01";				// A01,A02,B01分别代表3种包处理以及回复方式
} GET_PID;

// 返回模型运算数据的请求包
typedef struct pathological_result {
	std::string pathological_id; //病理号
	std::string slice_id;		  //切片号
	std::string sensor_id;		  //传感器编号
	std::string algorithm_version; //版本号
	long int diagnostic_time;   //诊断时间
	int diagnostic_cnt;	//诊断次数
	int picture_cnt;	//一个切片号对应图片数量
	int feature_cnt;	//中间参数次数
	int pathological_order;//诊断次序
	std::string diagnostic_results; //诊断结果
	std::vector<double> ai_results; //诊断概率
	std::string expert_result; //专家复审结果
	std::string tcp_type="A02"; // A01,A02,B01分别代表3种包处理以及回复方式
} PATHO_RES;

// 从服务器端接收到的数据包
typedef struct pis_response {
	std::string pathological_id; //病理号
	std::string slice_id;		  //切片号
	std::string gross_desp;        //巨检描述
	std::string surg_finding;        //手术所见
	std::vector<std::string> sample_record;        //取材记录
	std::string slice_part;        //切片部位
	std::string staining;        //染色类型	
	std::string slicesource;        //切片来源	
	std::string storage_node;    //存储节点，FTP服务地址
	int pathological_order;		 //诊断序号
	std::string models;			 //适用模型，暂时留着
	std::string ftpusr;			 //FTP服务器用户名
	std::string ftppwd;			 //FTP服务器密码
	int port;			 //FTP服务器端口号
} PIS_RES;

// 携带单张图片信息的包
typedef struct fig_feature {
	std::string tcp_type="B01";        // A01,A02,B01分别代表3种包处理以及回复方式
	std::string pathological_id;  //病理号
	std::string slice_id;		  //切片号
	int pathological_order;		  //诊断序号
	std::string fig_name;         //图片名称
	std::string ftpnode;          //图片存储的ftp地址
	int port;			          //FTP服务器端口号
	json features; // 直接将该参数设定为json格式，直接json赋值
	// features说明：
	// 该参数要么使用哈希表，之后转为json，这样可以支持特征名为中文，但是不能兼容值的类型包含多样化
	// 要么直接只用json，值的数据类型可以多样，但是键和值中都不能出现中文
} FIG_FTR;

typedef struct image_feature_queue {
	std::string image_dir;        //图片暂存的目录路径
	std::string feature_path;	  //所有图片的特征信息存放到同一个json文件中，给出json文件的具体路径
	std::string pathological_id;  //病理号
	std::string slice_id;		  //切片号
	int pathological_order;		  //诊断序号
	std::string ftpnode;          //图片存储的ftp地址
	int port;			          //FTP服务器端口号
	std::string ftpusr;			  //FTP服务器用户名
	std::string ftppwd;			  //FTP服务器密码
} IF_QUE;

typedef struct signal_of_finish {
	std::string tcp_type = "B02";        // A01,A02,B01分别代表3种包处理以及回复方式
	std::string pathological_id;  //病理号
	std::string slice_id;		  //切片号
	int pathological_order;		  //诊断序号
} SIG_FINISH;

extern "C" DBINTERFACE_API int dbSockInit(SOCKET& sockClient, std::string ipaddr, unsigned short port);
extern "C" DBINTERFACE_API int dbSockClose(SOCKET sockClient);
extern "C" DBINTERFACE_API int dbGetPathinfo(GET_PID* res, SOCKET sockClient, PIS_RES* receive);
extern "C" DBINTERFACE_API int dbResultUpload(PATHO_RES* res, SOCKET sockClient);
extern "C" DBINTERFACE_API int dbFinishSend(SIG_FINISH* fig_data, SOCKET sockClient);
extern "C" DBINTERFACE_API int SocketSend(FIG_FTR* fig_data, SOCKET sockClient);
extern "C" DBINTERFACE_API int savejson(json* js, std::string filedir, std::string filename);

extern "C" DBINTERFACE_API int ftpConnect(std::string host_ip, std::string port, ftplib* ftp, std::string user, std::string passwd);// 1, success, 0, connect failed, -1, ftp null
extern "C" DBINTERFACE_API int ftpDisconnect(ftplib* ftp);
extern "C" DBINTERFACE_API int ftpPut(ftplib* ftp, std::string local_file, std::string ftp_dir);
extern "C" DBINTERFACE_API int ftpGet(ftplib* ftp, std::string local_file, std::string ftp_dir);
extern "C" DBINTERFACE_API int ftpMkdir(ftplib* ftp, std::string ftp_dir);
extern "C" DBINTERFACE_API int ftpStoreSingle(ftplib* ftp, const std::string local_image_path, std::string ftp_dir, std::string fname);


