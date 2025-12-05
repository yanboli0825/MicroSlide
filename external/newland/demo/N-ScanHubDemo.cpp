#include "N-ScanHub.h"
#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <thread>
#include "Debug.h"

using namespace std;

#define NET_TEST 1

// Read device data
void __stdcall ReadCallback(const HANDLEDEV hDevice, const char* buf, int len)
{
	printf("-----------ReadCallback len=%d, buf=%s\n", len, buf);
}

// Monitoring device status change
void __stdcall DevStatChangeCallback(const HANDLEDEV hDevice, bool isDevExisted)
{
	if (isDevExisted)
		printf("hDevice=%p, device is pushed in\n", hDevice);
	else
		printf("hDevice=%p, device is pushed out\n", hDevice);
}
#if NET_TEST
void __stdcall TcpServiceBack(int clientSocket, char* clientIp) {
	printf("clientIp=%s\n", clientIp);
	char buf[4096] = { 0 };
	int len = 4096;
	while (1) {
		if (nl_readFromSocket(clientSocket, 2000, buf, &len) == 0) {
			printf("TcpServiceBack buf=%s\n", buf);
			memset(buf, 0, sizeof(buf));
		}
		Sleep(500);
	}
}

int piccount = 0;
void __stdcall readNetImage(unsigned char* data, int data_len){
	char filename[256] = { 0 };
	sprintf(filename, "./pic/%d.jpg", piccount++);
	//nl_saveNetImgData(0, data, data_len, filename);
}

void __stdcall readTcpIpData(unsigned char* data, int data_len) {
	printf("data=%s\n", data);
}

void ServerThread() {
	int ret = nl_CreateTcpService(10000, TcpServiceBack);
	printf("ServerThread ret=%d\n", ret);
}

void NetImageThread(char* ip, int* port1, int* port2) {
	int socket36520 = -1;

	char sendbuf[1024] = { 0 };
	char recvbuf[1024] = { 0 };
	int socket30000 = -1;

	strcpy(sendbuf, "\x01\x54\x04");

	nl_connectToService(ip, *port1, &socket30000);
	nl_connectToService(ip, *port2, &socket36520);
	const int RECV_BUFFER_SIZE = 1920 * 1080 * 4;
	char* recvBuffer = (char*)malloc(RECV_BUFFER_SIZE);
	int realLen = 0, nRet = -1;
	IMG_TYPE imgtype;
	int w, h, f, q;
	f = 2;
	q = 2;
	char filename[128] = { 0 };

	for (int i = 0; i < 1000; i++) {
		memset(recvBuffer, 0, RECV_BUFFER_SIZE);
		memset(recvbuf, 0, 1024);
		//触发识别
		if (nl_sendDataToSocket(socket30000, sendbuf, 3) != 0) {
			printf("nl_sendDataToSocket error\n");
			continue;
		}
		//获取码值
		if (nl_readFromSocket(socket30000, 2, recvbuf, &realLen) != 0) {
			printf("nl_readFromSocket error\n");
			nl_CloseClientSocket(socket30000);
			Sleep(500);
			nl_connectToService(ip, *port1, &socket30000);
			if (socket30000 == -1) {
				printf("reconncet error\n");
				continue;
			}
		}
		printf("码数据信息长度=%d,内容=%s\n", realLen, recvbuf);
		//获取图片，最常用的两个参数f和q，f代表获取图片的数据类型，q是当f=2获取jpg图片数据时，jpg的压缩质量等级
		nRet = nl_getNetImgData(socket36520, 0, 0, f, q, recvBuffer, &realLen, &imgtype, &w, &h);
		printf("-------------------------------------ip=%s [%d][%d]\n", ip, nRet, realLen);
		if (nRet != 0)
			continue;
		//f==0w为raw数据，需要调用保存图片接口对raw数据进行打包才能生成图片文件，
		//如果不想使用保存接口，也可以自行实现图片数据打包
		if (f == 0) {
			if (imgtype == TYPE_COLOR) {
				sprintf(filename, "./pic/f0-%s-%04d.bmp", ip, i);

				nl_SavePicDataToFile(filename, (unsigned char*)recvBuffer, w, h, 24); // Save image

				sprintf(filename, "./pic/f0-%s-%04d.jpg", ip, i);

				nl_SavePicDataToFile(filename, (unsigned char*)recvBuffer, w, h, 23); // Save image
				DbgPrintf("nl_SavePicDataToFile jpg end");
			}
			else {
				sprintf(filename, "./pic/f0-%s-%04d.bmp", ip, i);
				nl_SavePicDataToFile(filename, (unsigned char*)recvBuffer, w, h, 8); // Save image
				sprintf(filename, "./pic/f0-%s-%04d.jpg", ip, i);
				nl_SavePicDataToFile(filename, (unsigned char*)recvBuffer, w, h, 13); // Save image
			}
		}
		//f以下值时，获取的都是图片格式数据，可以直接保存二进制文件
		else if (f == 1) {
			sprintf(filename, "./pic/f1-%s-%04d.bmp", ip, i);
			FILE* fp = fopen(filename, "wb");
			fwrite(recvBuffer, 1, realLen, fp);
			fclose(fp);
		}
		else if (f == 2) {
			sprintf(filename, "./pic/f2-%s-%04d.jpg", ip, i);
			FILE* fp = fopen(filename, "wb");
			fwrite(recvBuffer, 1, realLen, fp);
			fclose(fp);
		}
		else if (f == 3) {
			sprintf(filename, "./pic/f3-%s-%04d.bmp", ip, i);
			FILE* fp = fopen(filename, "wb");
			fwrite(recvBuffer, 1, realLen, fp);
			fclose(fp);
		}
		else if (f == 4) {
			sprintf(filename, "./pic/f4-%s-%04d.bmp", ip, i);
			FILE* fp = fopen(filename, "wb");
			fwrite(recvBuffer, 1, realLen, fp);
			fclose(fp);
		}

		Sleep(10);
	}
	nl_CloseClientSocket(socket36520);
	nl_CloseClientSocket(socket30000);
	free(recvBuffer);
	//delete[]recvbuf;
	recvBuffer = NULL;
	printf("-------------------------------------ip=%s close\n", ip);
}
#endif

void SplitString(const string& s, vector<string>& v, const string& c)
{
	string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

#if 1
int main(int argc, char* argv[])
{
#if NET_TEST
	//----------net test------------
	//ip直连方式获取码值和图片数据
	if (argc >= 2 && strcmp(argv[1], "--NetGetImg") == 0) {
		char ip1[20] = { 0 };
		char ip2[20] = { 0 };
		char ip3[20] = { 0 };
		int td1 = 1, td2 = 2;
		int port2 = 36520;
		int port1 = 30000;

		strcpy(ip1, "192.168.3.193");
		thread t1(NetImageThread, ip1, &port1, &port2);

		strcpy(ip2, "192.168.3.219");
		thread t2(NetImageThread, ip2, &port1, &port2);

		strcpy(ip3, "192.168.3.197");
		thread t3(NetImageThread, ip3, &port1, &port2);

		t1.join();
		t2.join();
		t3.join();

		return 0;
	}
	else if (argc >= 2 && strcmp(argv[1], "--ServerMode") == 0) {
		//网络服务端只提供一个简易模式以供参考，服务端模式建议根据具体需求自行实现
		thread tt(ServerThread);
		tt.detach();
		Sleep(30000);
		nl_ExitTcpService();
		printf("exit\n");
		return 0;
	}
	//----------net test over-------
#endif
	int deviceCounts = 0;
	HANDLEDEVLST hDeviceList = NULL;

	//搜索网络设备可以采用异步方式在后台刷新
	if (argc >= 2 && strcmp(argv[1], "--EnumNetDevAsyn") == 0) {
		printf("begin nl_BeginEnumNetDevice\n");
		nl_BeginEnumNetDevice();
		for (int i = 0; i < 16; i++) {
			hDeviceList = nl_EnumDevices(&deviceCounts);
			printf("------------asyn enum deviceCounts------------=[%d]\n", deviceCounts);
			Sleep(1000);
		}
		nl_StopEnumNetDevice();
		return 0;
	}

	DbgPrintf("enum nl_EnumDevices begin\n");
	hDeviceList = nl_EnumDevices(&deviceCounts, ENUM_ALL); // Enumerate device
	DbgPrintf("enum nl_EnumDevices end\n");
	printf("deviceCounts=%d\n", deviceCounts);

	for (int i = 0; i < deviceCounts; i++) // Get all device information
	{
		HANDLEDEV hDevice;
		hDevice = nl_OpenDevice(hDeviceList, i); // Open the device
		printf("hDevice=%p, %s\n", hDevice, hDevice != NULL ? "succeed in opening the device" : "failed to open the device");

		if (NULL == hDevice)
			continue;

		T_DeviceStatus status = nl_GetDevStatus(hDevice); // Get device status
		printf("status=%d\n", status);
		if (argc < 2) {
			// Write character string data
			//触发识别
			const char* strCmd = "QRYSYS"; // QRYSYS: System information
			char receivedData[65565] = { 0 };
			int recvlen = 0;
			nl_Write(hDevice, strCmd, 6, true);
			int ret = nl_Read(hDevice, receivedData, 1000, 1000);
			printf("system info: \n%s\n", receivedData);
		}

		if (argc >= 2 && strcmp(argv[1], "--WriteAsHex") == 0) // Write data to the device in HEX character string
		{
			// Write hex character string data
			const char* strCmdhEX = "7e 01 30 30 30 30 40 51 52 59 53 59 53 3b 03"; // System information
			char receivedData[1024] = { 0 };
			int nRet = 0;
			bool isWrited = nl_WriteAsHex(hDevice, strCmdhEX, true); // Write data
			nRet = nl_Read(hDevice, receivedData, sizeof(receivedData), 0); // Read data
			printf("nRet=%d, receivedData=%s\n", nRet, receivedData);
		}
		else if (argc >= 2 && strcmp(argv[1], "--GetDeviceInfo") == 0) // Write data to the device in HEX character string
		{
			//获取设备信息
			STDeviceInfo info;
			memset(&info, 0, sizeof(STDeviceInfo));
			nl_GetDeviceInfo(hDeviceList, i, &info);
			printf("GetDeviceInfo -------- info\n %s\ntype=%d\n", info.devInfo, info.devType);
		}
		else if (argc >= 2 && strcmp(argv[1], "--SendCommand") == 0) // Send control commands to the device and obtain the returned information
		{
			//发送指令，返回结果是否成功
			char strCmd[2048] = { 0 };
			strcpy(strCmd, "QRYSYS;");
			int result = nl_SendCommand(hDevice, strCmd, strlen(strCmd)); // Send commands
			printf("result=%d\n", result);
		}
		else if (argc >= 2 && strcmp(argv[1], "--SendCommandAsHex") == 0) // Send control commands to the device in the form of HEX character string and get the returned information.
		{
			const char* strCmd = "51 52 59 53 59 53"; // QRYSYS: System information
			T_CommunicationResult result = nl_SendCommandAsHex(hDevice, strCmd, strlen(strCmd)); // Send commands
			printf("result=%d\n", result);
		}
		else if (argc >= 2 && strcmp(argv[1], "--GetCommandResponse") == 0) {
			//发送指令，接收设备返回的指令，目前最大长度支持到20000
				char strCmd[20480] = { 0 };
				strcpy(strCmd, "QRYSYS;");
				char recvData[20480] = { 0 };
				int recvLen = 0;
				bool result = nl_GetCommandResponse(hDevice, strCmd, strlen(strCmd), recvData, &recvLen, 500, true, false);
				printf("result=%d\n", result);
				printf("recvData=%s\n", recvData);
		}
		else if (argc >= 2 && strcmp(argv[1], "--GetPicture") == 0) // Get device image
		{
			//获取图片的基础接口，所有参数为默认
			unsigned int imgWidth = 0, imgHeight = 0;
			char filename[1024] = { 0 };
			sprintf(filename, "1%d.bmp", i);
			bool isGetPicSizeOK = nl_GetPicSize(hDevice, &imgWidth, &imgHeight); // Get the image width and height
			printf("nl_GetPicSize isGetPicSizeOK=%d\n", isGetPicSizeOK);
			if (isGetPicSizeOK && imgWidth > 0 && imgHeight > 0)
			{
				printf("imgWidth=%d,imgHeight=%d\n", imgWidth, imgHeight);
				const int RECV_BUFFER_SIZE = imgWidth * imgHeight;
				unsigned char* recvBuffer = (unsigned char*)malloc(RECV_BUFFER_SIZE);
				bool isOK = nl_GetPicData(hDevice, recvBuffer, RECV_BUFFER_SIZE); // Get the image raw data
				if(isOK)
					nl_SavePicDataToFile(filename, recvBuffer, imgWidth, imgHeight, 8);
			}
		}
		else if (argc >= 2 && strcmp(argv[1], "--GetPictureByConfig") == 0) // Get device image
		{
			//获取图片数据，可配置参数
			int a = nl_SendCommand(hDevice, "@SCNTRG1", 8);
			printf("nl_SendCommand a=%d\n", a);
			Sleep(200);
			unsigned int imgWidth = 0, imgHeight = 0;
			bool isGetPicSizeOK = nl_GetPicSize(hDevice, &imgWidth, &imgHeight); // Get the image width and height
			printf("nl_GetPicSize isGetPicSizeOK=%d\n", isGetPicSizeOK);
			if (isGetPicSizeOK && imgWidth > 0 && imgHeight > 0)
			{
				printf("imgWidth=%d,imgHeight=%d\n", imgWidth, imgHeight);
				const int RECV_BUFFER_SIZE = imgWidth * imgHeight * 4;
				unsigned char* recvBuffer = (unsigned char*)malloc(RECV_BUFFER_SIZE);//需要分配一个足够大的存放图片数据的空间
				STImgParam imgParam;
				memset(&imgParam, 0, sizeof(STImgParam));
				imgParam.f = 0;
				imgParam.q = 3;
	/*			imgParam.t = -1;
				imgParam.r = -1;
				imgParam.f = -1;
				imgParam.q = -1;
				imgParam.i = -1;
				imgParam.a = 255;*/
				//strcpy(imgParam.b, "0540055001000100");
				STImgResolution imgR[4];
				memset(imgR, 0, sizeof(STImgResolution) * 4);
				unsigned int nRealLen = 0;
				bool isOK = nl_GetPicDataByConfig(hDevice, imgParam, recvBuffer, &nRealLen, imgR); // Get the image data
				printf("isOK=%d\n", isOK);

				char filename[1024] = { 0 };
				if (isOK) {
					if (imgParam.t == 2) {
						for (int i = 0; i < 4; i++) {
							printf("imgR[%d] width=%d height=%d\n", i, imgR->width, imgR->height);
						}
					}
					if (imgParam.f == 1) {
						sprintf(filename, "test3%d.bmp", i);
						FILE* fp = fopen(filename, "wb");
						fwrite(recvBuffer, 1, nRealLen, fp);
						fclose(fp);
					}
					else if (imgParam.f == 2) {
						sprintf(filename, "test4%d.jpg", i);
						FILE* fp = fopen(filename, "wb");
						fwrite(recvBuffer, 1, nRealLen, fp);
						fclose(fp);
					}
					else if (imgParam.f == 3) {
						sprintf(filename, "test5%d.tiff", i);
						FILE* fp = fopen(filename, "wb");
						fwrite(recvBuffer, 1, nRealLen, fp);
						fclose(fp);
					}
					else if (imgParam.f == 4) {
						sprintf(filename, "test6%d.bmp", i);
						FILE* fp = fopen(filename, "wb");
						fwrite(recvBuffer, 1, nRealLen, fp);
						fclose(fp);
					}
					else if (imgParam.f == 0) {
						long outLen = 0;
						STImgResolution imgResIn, imgResOut;
						imgResIn.width = imgWidth;
						imgResIn.height = imgHeight;
						unsigned int imgLen = 0;
						IMG_TYPE type = nl_GetDeviceImageColorType(hDevice, &imgResOut, &imgLen);
						if (type == TYPE_COLOR) {
							unsigned char* outBuf = (unsigned char*)malloc(imgLen);
							printf("imgLen=%d\n", imgLen);
							bool res = nl_ConvertImageColorSpace(hDevice, recvBuffer, RECV_BUFFER_SIZE, imgResIn, outBuf);
							int oWidth, oHeight;
							if (strlen(imgParam.b) != 0) {//如果取的是局部图像，要使用所取的局部分辨率
								oWidth = stoi(string(imgParam.b).substr(8, 4));
								oHeight = stoi(string(imgParam.b).substr(12, 4));
							}
							else {
								oWidth = imgResOut.width;
								oHeight = imgResOut.height;
							}
							sprintf(filename, "./pic/test2%d.bmp", i);
							nl_SavePicDataToFile(filename, outBuf, oWidth, oHeight, 24); // Save image
							sprintf(filename, "./pic/test2%d.jpg", i);
							nl_SavePicDataToFile(filename, outBuf, oWidth, oHeight, 23); // Save image
						}
						else {
							int oWidth, oHeight;
							if (strlen(imgParam.b) != 0) {
								oWidth = stoi(string(imgParam.b).substr(8, 4));
								oHeight = stoi(string(imgParam.b).substr(12, 4));
							}
							else {
								oWidth = imgResOut.width;
								oHeight = imgResOut.height;
							}
							sprintf(filename, "./pic/test1%d%d.bmp", i);
							nl_SavePicDataToFile(filename, recvBuffer, oWidth, oHeight, 8); // Save image
							sprintf(filename, "./pic/test1%d%d.jpg", i);
							nl_SavePicDataToFile(filename, recvBuffer, oWidth, oHeight, 13); // Save image
						}
					}
				}
				free(recvBuffer);
				recvBuffer = NULL;
			}
		}
		else if (argc >= 2 && strcmp(argv[1], "--SetListener") == 0) // Asynchronous reading of device data
		{
			//读码监听，开启后设备读到码会传给回调函数ReadCallback
			nl_SetListener(hDevice, ReadCallback);
			Sleep(1000000);
			nl_StopListener(hDevice);
		}
		else if (argc >= 2 && strcmp(argv[1], "--ReadDevCfgToXml") == 0) // Read the configuration from the device and save it to the xml file.
		{
			char filename[128] = { 0 };
			strcpy(filename, "./xml/2.xml");
			nl_ReadDevCfgToXml(hDevice, filename);
		}
		else if (argc >= 2 && strcmp(argv[1], "--WriteCfgToDev") == 0) // Write the configuration file information to the device.
		{
			bool ret = nl_WriteCfgToDev(hDevice, "./xml/2.xml");
			if (!ret)
				printf("WriteCfgToDev %s\n", nl_GetLastError());//如果导入错误，调用nl_GetLastError可以获知哪些指令导入错误
		}
		else if (argc >= 2 && strcmp(argv[1], "--SetCbDevStatusChanged") == 0) // Set the callback function when the device status changes.暂时不能使用
		{
			//监控设备拔插状态并作出打开或关闭操作，只监控本地串口和usb设备，网络设备该接口无效
			nl_SetCbDevStatusChanged(hDevice, DevStatChangeCallback);
			Sleep(100000);
			printf("SetCbDevStatusChanged finish\n");
		}
		else if (argc >= 2 && strcmp(argv[1], "--UpdateFirmware") == 0) // Update device
		{
			unsigned updateError = -1;
			bool isUpdated = nl_UpdateKernelDevice(hDevice, "Y:/Newland/scan/firmware/soldier160/SOLDIER160_V1.04.003.4.bin2", 0, &updateError); // Firmware update
			printf("updateError=%d,%s\n", updateError, isUpdated ? "succeed in updating the firmware" : "failed to update the firmware");
			switch (updateError)
			{
			case Success:
				printf("The firmware update is normal.\n");
				break;
			case FileNameExtError:
				printf("file name error\n");
				break;
			}
		}
		else if (argc >= 2 && strcmp(argv[1], "--SetNetDeviceConfig") == 0) {
			char configData[2048] = { 0 };
			strcpy(configData, "Serial Number=A6516268F31B66FC;MAC Address=00:51:62:68:F3:1B;Device Use DHCP=1;Device IP Address=192.168.76.250;Device SubNetmask=255.255.255.0;Device Gateway Address=192.168.76.1;");
			char outData[2048] = { 0 };
			int nRet = nl_SetNetDeviceConfig(configData, strlen(configData), 5000, outData);
			if (nRet != 0)
			{
				printf("nl_setNetDeviceConfig error\n");
			}
			printf("\n nl_setNetDeviceConfig outData=%s\n", outData);
		}

		bool isClosed = nl_CloseDevice(&hDevice); // Close the device
		printf("hDevice=%p,%s\n", hDevice, isClosed ? "succeed in closing the device" : "failed to close the device");
		T_DeviceStatus t = nl_GetDevStatus(hDevice);
		printf("T_DeviceStatus t=%d\n", t);
	}
	nl_ReleaseDevices(&hDeviceList); // Release the device list handle
	Sleep(1000);
	printf("all over\n");
	system("pause");

	return 0;
}
#endif
