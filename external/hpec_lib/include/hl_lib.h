#ifndef __HL_LIB_H__
#define __HL_LIB_H__
#include<Windows.h>
#include<time.h>
#include "skf19_selfinfo.h"

#define UNINIT_DEVICE				0x00000FF7 //未初始化

#define W_REG_OK					0x00001021 //写寄存器成功
#define W_REG_ERROR					0x00001022 //写寄存器有误
#define R_REG_OK					0x00001031 //读寄存器成功
#define R_REG_ERROR					0x00001032 //读寄存器有误

#define RECFG_OK					0x00001041 //重构成功
#define RECFG_ERROR					0x00001042 //重构有误
#define LINK_ERROR					0x00001043 //pcie链路建立失败

#define ST_DATA_OK					0x00001111 //上行传输通道打开成功
#define ST_DATA_ERROR				0x00001112 //上行传输通道打开失败
#define SP_DATA_OK					0x00001121 //上行传输通道关闭成功
#define SP_DATA_ERROR				0x00001122 //上行传输通道关闭失败

#define RW_DATA_OK					0x00008000 //读/写数据成功
#define RW_DATA_TIMEOUT				0x00008001 //读/写数据超时
#define RW_DATA_ERROR				0x00008002 //读/写数据失败

#define REG_GROUP_1					0x00800000 //寄存器组基地址，该组寄存器大小为4MB
#define REG_GROUP_2					0x00c00000 //寄存器组基地址，该组寄存器大小为128B
#define REG_GROUP_3					0x00c01000 //寄存器组基地址，该组寄存器大小为128B
#define REG_GROUP_4					0x00c02000 //寄存器组基地址，该组寄存器大小为128B
#define REG_GROUP_5					0x00c03000 //寄存器组基地址，该组寄存器大小为128B
#define REG_GROUP_6					0x00c04000 //寄存器组基地址，该组寄存器大小为4KB

#pragma comment(lib, "setupapi.lib")

/*
* 描述：获取设备总线号
* 输入：第几个设备对应总线号（0，1，2，...)
* 返回值
* -1：不存在，其它：总线号
*/
int bus_num_get(int dev_num);
/*
* 描述：判断设备是否存在
* 返回值
* 1：存在，其它：不存在
*/
int dev_exist_chk();
/*
* 描述：初始化上层接口资源
* 输入：设备对应总线号（从设备管理器内读取）
* 返回值：
* 0：成功，其它：失败
*/
int init_app(int bus_num);
/*
* 描述：销毁上层接口资源
* 输入：设备对应总线号（从设备管理器内读取）
* 返回值：
* 无
*/
void close_app(int bus_num);
/*
* 描述：打开设备
* 输入：设备对应总线号（从设备管理器内读取）
* 返回值：设备句柄
* 小于0：失败，其它：成功
*/
int open_dev(int bus_num);
/*
* 描述：关闭设备
* 输入：设备句柄，来自于open_dev
* 返回值：
* 无
*/
void close_dev(int hDev);
/*
* 描述：初始化设备
* 输入：设备句柄，来自于open_dev
* 返回值：
* 无
*/
void init_dev(int hDev);
/*
* 描述：重置设备
* 输入：设备句柄，来自于open_dev
* 返回值：
* 无
*/
void reset_dev(int hDev);
/*
* 描述：写115寄存器
* 输入：
* hDev：设备句柄，来自于open_dev
* fpga_num：115编号，0：1号115，1：2号115
* reg_num：寄存器编号，写这一组寄存器中第几个寄存器，范围[0~该组寄存器大小/4]
* reg_data：寄存器数值，[0，0xFFFFFFFF]
* reg_base：寄存器组的基地址，REG_GROUP_1 ~ REG_GROUP_6中的一个
* 返回值：
* W_REG_OK					0x00001021 //写寄存器成功
* W_REG_ERROR				0x00001022 //写寄存器有误
* 注意：寄存器使用需要和硬件开发人员配合
*/
unsigned int write_fpga_reg(int hDev, unsigned int fpga_num, unsigned int reg_num, unsigned int reg_data, unsigned int reg_base);
/*
* 描述：读115寄存器
* 输入：
* hDev：设备句柄，来自于open_dev
* fpga_num：115编号，0：1号115，1：2号115
* reg_num：寄存器编号，读这一组寄存器中第几个寄存器，范围[0~该组寄存器大小/4]
* reg_base：寄存器组的基地址，REG_GROUP_1 ~ REG_GROUP_6中的一个
* 输出：
* reg_data：寄存器数值，[0，0xFFFFFFFF]
* 返回值：
* R_REG_OK					0x00001031 //读寄存器成功
* R_REG_ERROR				0x00001032 //读寄存器有误
* 注意：寄存器使用需要和硬件开发人员配合
*/
unsigned int read_fpga_reg(int hDev, unsigned int fpga_num, unsigned int reg_num, unsigned int& reg_data, unsigned int reg_base);
/*
* 描述：上传重构
* 输入：
* hDev：设备句柄，来自于open_dev
* fpga_num：fpga 115编号 -> 0或者1
* buffer：重构数据所在缓冲区首地址
* size：重构数据长度
* 返回值：
* UNINIT_DEVICE				0x00000FF7 //未初始化
* RECFG_OK					0x00001041 //重构成功
* RECFG_ERROR				0x00001042 //重构有误
* LINK_ERROR				0x00001043 //pcie链路建立失败
*/
unsigned int reconfig_with_bin(int hDev, unsigned int fpga_num, char* buffer, unsigned int size);
/*
* 描述：开启上行传输
* 输入：
* hDev：设备句柄，来自于open_dev
* fpga_num：fpga 115编号 -> 0或者1
* 返回值：
* UNINIT_DEVICE				0x00000FF7 //未初始化
* ST_DATA_OK				0x00001111 //上行传输通道打开成功
* ST_DATA_ERROR				0x00001112 //上行传输通道打开失败
*/
unsigned int start_transfer_data_from_fpga(int hDev, unsigned int fpga_num);
/*
* 描述：关闭上行传输
* 输入：
* hDev：设备句柄，来自于open_dev
* fpga_num：fpga 115编号 -> 0或者1
* 返回值：
* UNINIT_DEVICE				0x00000FF7 //未初始化
* SP_DATA_OK				0x00001121 //上行传输通道关闭成功
* SP_DATA_ERROR				0x00001122 //上行传输通道关闭失败
*/
unsigned int stop_transfer_data_from_fpga(int hDev, unsigned int fpga_num);
/*
* 描述：读取上行数据
* 输入：
* hDev：设备句柄，来自于open_dev
* buffer：接受缓冲区，分配4MB大小
* wait_ms：超时时间，单位ms，0xFFFFFFFF为永久等待
* 输出：
* fpga_num：数据来源，fpga 115编号 -> 0或者1
* data_type：数据类型编号
* n_get：接收数据有效长度
* 返回值：
* UNINIT_DEVICE				0x00000FF7 //未初始化
* RW_DATA_OK				0x00008000 //读/写数据成功
* RW_DATA_TIMEOUT			0x00008001 //读/写数据超时
* RW_DATA_ERROR				0x00008002 //读/写数据失败
*/
unsigned int read_data_from_fpga(int hDev, char* buffer, unsigned int& fpga_num, unsigned int& data_type, unsigned int& n_get, unsigned int wait_ms);
/*
* 描述：传输下行数据
* 输入：
* hDev：设备句柄，来自于open_dev
* buffer：发送缓冲区
* data_len：发送缓冲区数据大小
* fpga_num：数据目的地，fpga 115编号 -> 0或者1
* data_type：数据类型编号
* wait_ms：超时时间，单位ms，0xFFFFFFFF为永久等待
* 输出：
* n_send：发送数据有效长度
* 返回值：
* UNINIT_DEVICE				0x00000FF7 //未初始化
* RW_DATA_OK				0x00008000 //读/写数据成功
* RW_DATA_TIMEOUT			0x00008001 //读/写数据超时
* RW_DATA_ERROR				0x00008002 //读/写数据失败
*/
unsigned int write_data_to_fpga(int hDev, char* buffer, unsigned int data_len, unsigned int fpga_num, unsigned int data_type, unsigned int& n_send, unsigned int wait_ms);
/*
* 描述：获取用户中断
* 输入：
* hDev：设备句柄，来自于open_dev
* wait_ms：超时时间，单位ms，0xFFFFFFFF为永久等待
* 返回值：
* 0：等待到中断
* -1：等待超时
*/
int wait_user_irq(int hDev, unsigned int wait_ms);
/*
* 描述：读取来自于ARM的513、117以及flash的板级自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_board_info(int hDev, BOARDINFO& buffer);
/*
* 描述：读取19eg fpga的自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_fpga_info(int hDev, FPGAINFO& buffer);
/*
* 描述：读取115 sysmanager的自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_sysmon_info(int hDev, unsigned int fpga_num, SYSMONINFO& buffer);
/*
* 描述：读取AD DA链路状态温度的自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_ad_info(int hDev, ADINFO& buffer);
/*
* 描述：读取eeprom的自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_eeprom_info(int hDev, EEPROMINFO& buffer);
/*
* 描述：读取cpu的自检信息
* 输入：
* hDev：设备句柄，来自于open_dev
* 输出：
* buffer：输入缓冲区
* 返回值：
* 0：读取自检信息成功
* -1：读取自检信息失败
*/
int get_cpu_info(int hDev, CPUINFO& buffer);
/*
* 描述：ad内外标频切换
* 输入：
* hDev：设备句柄，来自于open_dev
* ref：内外标频，0：内标频；1：外标频
* 返回值：
* 0：成功
* 其它：失败
*/
int ad_ref_switch(int hDev, int ref);

/*
* 描述：AD中心频率设置
* 输入：
* hDev：设备句柄，来自于open_dev
* freq：中心频率，单位MHz，freq1为通道1，freq2为通道2，当值为0时代表不进行配置
* 返回值：
* 0：成功
* 其它：失败
*/
int ad_freq_set(int hDev, unsigned int freq1, unsigned int freq2);

/*
* 描述：DA中心频率设置
* 输入：
* hDev：设备句柄，来自于open_dev
* freq：中心频率，单位MHz，freq1为通道1，freq2为通道2，当值为0时代表不进行配置
* 返回值：
* 0：成功
* 其它：失败
*/
int da_freq_set(int hDev, unsigned int freq1, unsigned int freq2);

unsigned int write_fpga_reg_priv(int hDev, unsigned int fpga_num, unsigned int reg_num, unsigned int reg_data);
unsigned int read_fpga_reg_priv(int hDev, unsigned int fpga_num, unsigned int reg_num, unsigned int& reg_data);

unsigned int write_19eg_reg(int hDev, unsigned int reg_offset, unsigned int reg_data);
unsigned int read_19eg_reg(int hDev, unsigned int reg_offset, unsigned int& reg_data);

#endif