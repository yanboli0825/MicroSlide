#ifndef __HL_SKF19_SELF_INFO__
#define __HL_SKF19_SELF_INFO__


/*来自于ARM的513 117以及flash*/
typedef struct _board_info_
{
	char board_num[256];							//版号
	double temp_513;								//513本地温度(摄氏度)
	double bus_vol_513;								//513总线电压(V)
	double para_vol_513;							//513并联电压(mV)
	double para_curr_513;							//513电流(A)
	double power_513;								//513功率(W)
	double temp_117[4];								//117温度(摄氏度)
	double temp_sys[3];								//sysmonitar温度(摄氏度)
	double vol_sys[27];								//sysmonitar电压(V)
}BOARDINFO;

typedef struct _fpga_self_info_
{
	unsigned int fpga_num;							//fpga编号
	unsigned int hw_version;						//fpga批次
	unsigned int sw_version;						//fpga工程版本
	unsigned int ad_cfg_status;						//ad配置状态
	unsigned int f1_pcie_init_status;				//f1 pcie初始化状态
	unsigned int f2_pcie_init_status;				//f2 pcie初始化状态
}FPGAINFO;

/*来自AD的eeprom*/
typedef struct _eeprom_status_
{
	char front[16];									//前置码
	char boardname[32];								//板子名字
	char edition[16];								//版本
	char batch[16];									//批次
	char date[16];									//日期
	char encode[16];								//版号
	char pcb[32];									
	char welding[32];
	char other[80];
}EEPROMINFO;

/*AD DA链路状态温度*/
typedef struct _ad_self_info_
{
	int ad_temp;								//温度
	int link_type;								//内外标频
	int ad_link[8];								//AD子卡端链路状态
	int ad_fpga_link[8];						//AD Fpga端链路状态
	int da_link[8];								//DA子卡端链路状态
	int da_fpga_link[8];						//DA Fpga端链路状态
}ADINFO;

/*sysmanager信息*/
typedef struct _sysmon_status_
{
	unsigned int fpga_num;						//fpga编号
	float temp;									//温度（摄氏度）
	float vccint;								//int电压(V)
	float vccaux;								//aux电压(V)
}SYSMONINFO;

/*软件相关信息*/
typedef struct _cpu_info_
{	
	unsigned int sw_version;						//软件库版本
	unsigned int driver_version;					//驱动版本
	unsigned int temp_cpu[12];						//cpu温度(摄氏度)
}CPUINFO;

#endif
