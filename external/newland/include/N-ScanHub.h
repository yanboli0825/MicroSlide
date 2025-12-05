#pragma once
#ifndef NLSDEVICEMASTER_H_
#define NLSDEVICEMASTER_H_
#include <cstddef>


typedef void* HANDLEDEVLST;	///< Device list handle.
typedef void* HANDLEDEV;	///< Device handle.
typedef int EnumType;

#define ENUM_SERIAL 1
#define ENUM_USB	2
#define ENUM_NETDEV	4
#define ENUM_ALL	ENUM_SERIAL | ENUM_USB | ENUM_NETDEV

/**
* @brief Abnormal type.
*/
enum T_ErrorType
{
	Success						= 0, ///< Normal.
	UnknownError				= 1, ///< Unknown Error.
	NotExistError				= 2, ///< The device doesn't exit.
	NotOpenError				= 3, ///< The device is not opened.
	AlreadyOpenError			= 4, ///< The device is opened.
	AccessDeniedError			= 5, ///< Access to the device is denied.
	NotInitializedError			= 6, ///< The Device is not initialized.
	InvalidParamsError			= 8, ///< Invalid parameters.
	InvalidFileFormatError		= 9, ///< Invalid file format.
	FileNameExtError			= 10,///< File name error.
	CommunicationError			= 11,///< Communication error.
	MallocError					= 12,///< Memory allocation error.
	UpdateFailedError			= 13,///< Failed to update.
	NoUpdateObjectError			= 14,///< No updating object.
	FileNotExistError			= 15,///< the file doesn't exist.
	BufferOverflowError			= 16,///< Buffer overflows.
	FileNotSuitableError		= 17,///< The file is not suitable.
	DeviceNotUniqueError		= 18,///< The device is not unique.
	NoConversionNeeded			= 19,///< No conversion needed 
	ConvertColorSpaceError		= 20,///< Failed to convert
	ParamError					= 21,///< Entered incorrect parameters
};

/**
* @brief Device status.
*/
enum T_DeviceStatus
{
	Opened = 0,			///< Opened.
	NotOpened,			///< Not opened.
	Closed,				///< Closed.
	NotClosed,			///< Not closed.
	Updating,			///< Updating...
	Updated,			///< Updating is finished.
	Writing,			///< Writing data...
	Written,			///< Data writing is finished.
	Reading,			///< Reading data...
	ReadOK,				///< Data reading is finished.
	GettingPicData,		///< Getting image data...
	GetPicDataOK,		///< Image data has been ontained.
	GettingPicColorType,
	GetPicColorTypeOk,
	ConveringColorSpace,///< Convering image color space
	ConvertColorSpaceOK,///< Conversion finished.
	UnknownStatus		///< Unknown status.
};

/**
* @brief Commands sending result.
*/
enum T_CommunicationResult
{
	SendError = 0,	///< Sending error.
	Support,		///< Commands supported.
	Unsupport,		///< Commands not supported. 
	OutOfRange,		///< Data value is not within the range. 
	UnknownResult,	///< Unknown error.
};

/**
* @brief Protocol.
*/
enum T_Porotocol
{
	Nlscan = 0, // Newland.
};

typedef struct img_param {
	int t;
	int r;
	int f;
	int q;
	int i;
	int a;
	char b[17];
}STImgParam;

typedef struct img_resolution {
	int width;
	int height;
}STImgResolution;

enum IMG_TYPE {
	TYPE_UNKNOW = 0,
	TYPE_GRAY = 1,
	TYPE_COLOR = 2
};

enum NL_DEVICE_TYPE {
	DEV_TYPE_UNKNOW = 0,
	DEV_TYPE_USB = 1,
	DEV_TYPE_COM = 2,
	DEV_TYPE_NET = 3,
};

typedef struct st_device_info
{
	char devInfo[2048];
	NL_DEVICE_TYPE devType;
}STDeviceInfo;

#ifdef __cplusplus
extern "C"  
{
#endif
	/**
	* @brief enumerate device.
	* @param[out] deviceCount Number of device
	* @return Device list handle  Non-null: device list exists.  Null: device list doesn't exist. 
	*/
	HANDLEDEVLST nl_EnumDevices(int* deviceCount, EnumType = ENUM_ALL);

	/**
	* @brief Release the device list handle.
	* @param[in] hDeviceList Device list handle
	*/
	void nl_ReleaseDevices(HANDLEDEVLST* hDeviceList);
	
	/**
	* @brief Specify the indexed device on the device list.
	* @param[in] hDeviceList Device list handle
	* @param[in] index device index
	* @param[in] porotocol  Protocol of the manufacturer
	* @return Device handle  Non-null: succeed in opening.  Null: failed to open.
	*/
	HANDLEDEV nl_OpenDevice(const HANDLEDEVLST hDeviceList, unsigned int index, T_Porotocol porotocol = Nlscan);

	/**
	* @brief Write data to the device.
	* @param[in] hDevice Device handle
	* @param[in] data Written data
	* @param[in] len Data length
	* @param[in] isPacked Whether data is packed
	* @return Whether data is written. true: succeed in writing data. false: failed to write data.     
	*/
	bool nl_Write(const HANDLEDEV hDevice, const char* data, unsigned int len, bool isPacked = true);

	/**
	* @brief Write data to the device in the form of HEX character string.
	* @param[in] hDevice Device handle
	* @param[in] data Written data
	* @param[in] isPacked Whether data is packed
	* @return Whether data is written. true: succeed in writing data. false: failed to write data. 
	*/
	bool nl_WriteAsHex(const HANDLEDEV hDevice, const char* data, bool isPacked = false);

	/**
	* @brief Send control commands to the device (Commands will be packed according to different protocols inside the interface).
	* @param[in] hDevice Device handle
	* @param[in] command commands sent
	* @param[in] commandLen Command length 
	* @return Communication result 
	*/
	T_CommunicationResult nl_SendCommand(const HANDLEDEV hDevice, const char* command, unsigned int commandLen);

	/**
	* @brief Send control commands to the device in the form of HEX character string (Commands will be packed according to different protocols inside the interface).

	* @param[in] hDevice Device handle
	* @param[in] command Commands sent
	* @param[in] commandLen Command length 
	* @return Communication result
	*/
	T_CommunicationResult nl_SendCommandAsHex(const HANDLEDEV hDevice, const char* command, unsigned int commandLen);

	/**
	* @brief Send control commands to the device and receive data from the device (Commands will be packed according to different protocols inside the interface).
	* @param[in] hDevice Device handle
	* @param[in] command commands sent
	* @param[in] commandLen Command length
	* @param[out] response data from the device
	* @param[out] responseLen response length
	* @param[in] timeout millisecond
	* @param[in] isHex is HEX character string
	* @return  Get response result
	*/
	bool nl_GetCommandResponse(const HANDLEDEV hDevice, const char* command, unsigned int commandLen, char* response, int *responseLen, unsigned int timeout, bool isPacked = true, bool isHex = false);


	/**
	* @brief Read device data.
	* @param[in] hDevice Device handle
	* @param[out] buf data returned from the device
	* @param[in] len Received data length  
	* @param[in] timeout Data reading timeout When it is set as 0, it continues reading until there is no returned data. 
	* @return Data length returned from the device
	*/
	unsigned int nl_Read(const HANDLEDEV hDevice, char* buf, unsigned int len, unsigned int timeout);

	/**
	* @brief Callback function for asynchronous reading data, disabled to receive the data returned by the commands.
	* @param[in] hDevice Device handle
	* @param[in] buf Received data 
	* @param[in] len Received data length
	*/
	typedef void(__stdcall*readCallback)(const HANDLEDEV hDevice, const char* buf, int len);

	/**
	* @brief Set monitor.
	* @param[in] hDevice Device handle
	* @param[in] callback callback function
	*/
	void nl_SetListener(const HANDLEDEV hDevice, readCallback callback);

	/**
	* @brief Stop monitoring device data.
	* @param[in] hDevice Device handle
	* @return Whether monitoring device data is stopped. true: succeed in stopping monitoring. false: failed to stop monitoring.
	*/
	bool nl_StopListener(const HANDLEDEV hDevice);

	/**
	* @brief Get the size of device image.
	* @param[in] hDevice Device handle
	* @param[out] width Image width
	* @param[out] height Image height
	* @return Whether device image size is obtained.true: succeed in getting device image. false: failed to get device image.
	*/
	bool nl_GetPicSize(const HANDLEDEV hDevice, unsigned int* width, unsigned int* height);

	/**
	* @brief Get device image.
	* @param[in] hDevice Device handle
	* @param[out] imgBuf Image data
	* @param[in] imgBufLen Image data length
	* @return Whether device image is obtained.true: succeed in getting device image. false: failed to get device image.
	*/
    bool nl_GetPicData(const HANDLEDEV hDevice, unsigned char* imgBuf, int imgBufLen);

	/**
	* @brief Get device image.
	* @param[in] hDevice Device handle
	* @param[out] imgBuf Image data
	* @param[in] imgBufLen Image data length
	* @return Whether device image is obtained.true: succeed in getting device image. false: failed to get device image.
	*/
	bool  nl_GetPicDataByConfig(const HANDLEDEV hDevice, STImgParam imgParam, unsigned char* imgBuf, unsigned int *imgBufLen, STImgResolution* imgR = NULL);
	/**
	* @brief Update device.
	* @param[in] hDevice Device handle
	* @param[in] strFileName path of firmware file
	* @param[in] reserved Reserved field
	* @param[out] error Error number returned after the update failed.
	* @return Whether updating is successful.true: succeed in updating. false: failed to update.
	*/
	bool  nl_UpdateKernelDevice(const HANDLEDEV hDevice, const char* strFileName, unsigned int reserved, unsigned int* error);

	/**
	* @brief Close the device.
	* @param[in] hDevice Device handle
	* @return Whether the device is closed.true: succeed in closing the device. false: failed to close the device.
	*/
	bool nl_CloseDevice(HANDLEDEV* hDevice);

	/**
	* @brief Encapsulate the collected image data into BMP format and save it as a file.
	* @param[in] imgName bmp file name
	* @param[in] imgBuf Image buffer data
	* @param[in] width Image width
	* @param[in] height Image height
	* @param[in] flag if imgName is bmp: Image bit depth (8, 24),
					  if imgName is jpg: Image quality:
					  1.gray (10-Low, 11-Middle, 12-High, 13-Highest)
					  2.color(20-Low, 21-Middle, 22-High, 23-Highest)
	* @return Whether it is saved.true: saved. false: failed to save.
	*/
	bool  nl_SavePicDataToFile(const char* imgName, unsigned char* imgBuf, int width, int height, int flag);

	/**
	* @brief Get device status.
	* @param[in] hDevice Device handle
	* @return Device status
	*/
	T_DeviceStatus nl_GetDevStatus(const HANDLEDEV hDevice);

	/**
	* @brief Read the configuration from the device and save it to the xml file.
	* @param[in] hDevice Device handle
	* @param[in] cfgFilePath  Path of configuration file
	* @return Whether it is saved.true: saved. false: failed to save.
	*/
	bool nl_ReadDevCfgToXml(const HANDLEDEV hDevice, const char* cfgFilePath);

	/**
	* @brief Write the configuration file to the device.
	* @param[in] hDevice Device handle
	* @param[in] cfgFilePath Path of configuration file
	* @return Whether it is written.true: written. false: failed to write.
	*/
	bool nl_WriteCfgToDev(const HANDLEDEV hDevice, const char* cfgFilePath);

	/**
	* @brief Callback function when device status changes.
	* @param[in] hDevice Device handle
	* @param[in] isDevExisted Whether the device exists.
	*/
	typedef void(__stdcall*DevStatChgCallback)(const HANDLEDEV hDevice, bool isDevExisted);

	/**
	* @brief Set the callback function when device status changes.
	* @param[in] hDevice Device handle
	* @param[in] callback Callback function
	*/
	void  nl_SetCbDevStatusChanged(const HANDLEDEV hDevice, DevStatChgCallback callback);

	/**
	* @brief check device image data type (NV12 or BGR)
	* @param[in] hDevice Device handle
	* @param[out] imgResOut struct of width and height]
	* @param[out] imgLen image data length need to malloc 
	* * @return image type
	*/
	IMG_TYPE nl_GetDeviceImageColorType(const HANDLEDEV hDevice, STImgResolution* imgResOut, unsigned int * imgLen);

	/**
	* @brief Convert the color space of the image(NV12 to BGR).
	* @param[in] hDevice Device handle
	* @param[in] imgBufIn Image buffer data
	* @param[in] imgBufInLen Image buffer data length
	* @param[in] imgResIn struct of width and height
	* @param[out] imgBufOut Image buffer data
	* * @return Conversion result.
	*/
	bool nl_ConvertImageColorSpace(const HANDLEDEV hDevice, unsigned char* imgBufIn, long imgBufInLen, STImgResolution imgResIn, unsigned char* imgBufOut);

	/**
	* @brief get device info
	* @param[in] hDeviceList Device list handle
	* @param[in] index device index
	* @param[out] stDevInfo device info
	* * @return result
	*/
	bool nl_GetDeviceInfo(const HANDLEDEVLST hDeviceList, unsigned int index, STDeviceInfo* stNetDevInfo);

	/**
	* @brief device is open return true
	* @param[in] hDevice Device handle
	* * @return result
	*/
	bool nl_DeviceIsOpenByHandle(const HANDLEDEV hDevice);

	/**
	* @brief device is open return true
	* @param[in] hDeviceList Device list handle
	* @param[in] index device index
	* * @return result
	*/
	bool nl_DeviceIsOpenByList(const HANDLEDEVLST hDeviceList, unsigned int index);

	/**
	* @brief get error message
	* @param[out] errMsg error message
	* * @return error code
	*/
	char* nl_GetLastError();

	//tcp/ip/udp鐩稿叧鎺ュ彛

	/**
	* @brief continuously searching for network devices
	*/
	void nl_BeginEnumNetDevice();

	/**
	* @brief Stop searching for network devices
	*/
	void nl_StopEnumNetDevice();

	/**
	* @brief config net devices
	* @param[in] revTimeout single find net device timeout,second
	* @param[in] inData input data
	* @param[in] inDataLen is inData len
	* @param[out] outdata dev return data
	*/
	int nl_SetNetDeviceConfig(char* inData,int inDataLen,int recTimeout,char* outdata);

	typedef void(__stdcall* tcpServiceBack)(int clientSocket, char* clientIp);

	/**
	* @brief host is client,readData from devices services
	* @param[in] port is service port
	* @param[in] callback client callback
	*/
	int nl_CreateTcpService(int port, tcpServiceBack callback);

	/**
	* @brief host is client,readData from devices services
	* @param[in] sockets is all services accept client socket
	* @param[in] sockets_len socket count
	*/
	int nl_ExitTcpService();
	//int nl_exitTcpService(int* sockets, int sockets_len);

	/**
	* @brief close socket
	* @param[in] socket
	*/
	int nl_CloseClientSocket(int socket);


	/**
	* @brief host is client,connect to devices services
	* @param[in] serviceIp services ip
	* @param[in] port services port
	* @param[out] socket is client sockets
	*/
	int nl_connectToService(char* serviceIp, int port, int* socket);

	/**
	* @brief host is client,sendData to devices services
	* @param[in] socket is client sockets
	* @param[in] buf is senddata
	* @param[in] buf_len is senddata len
	*/
	int nl_sendDataToSocket(int socket, char* buf, int buf_len);

	/**
	* @brief host is client,receive data to devices services
	* @param[in] socket is client sockets
	* @param[in] nTimeout is receive timeout
	* @param[out] outbuf is reecive data
	* @param[out] buflen is reecive data len
	*/
	int nl_readFromSocket(int socket, int nTimeout, char* outbuf, int* buflen);

	/**
	* @brief host is client,receive data to devices services
	* @param[in] socket is client sockets
	* @param[in] T is image type
	* @param[in] R is image proportion
	* @param[in] F is image data type
	* @param[in] Q is jepg quality
	* @param[out] imgData is image data
	* @param[out] realLen is image data len
	* @param[out] imgtype is image type gray or color
	* @param[out] width is image width
	* @param[out] heigh is image heigh
	*/
	int nl_getNetImgData(int socket, int T, int R, int F, int Q, char* imgData, int* realLen, IMG_TYPE* imgtype, int* width, int* heigh);
#ifdef __cplusplus
}
#endif

#endif /* NLSDEVICEMASTER_H_ */
