#pragma once
#include <QString>

/*ini配置文件路径与GroupName*/
#define INIT_FILE_PATH "../../../assets/config/config.ini"; ///< 配置文件路径
#define GROUP_NAME "SystemConfiguration";                   ///< GroupName

namespace IniParser
{
/**
 * @brief 静态函数，用于保存系统设置
 *
 * @param[in] iniPath     写入的ini配置文件路径
 * @param[in] groupName   要写入的group名称
 * @param[in] key		 要写入的键名
 * @param[in] value		 要写入的值
 */
void saveIniSettings(const QString iniPath, const QString groupName, const QString key, const QString value);

/**
 * @brief 静态函数，用于读取系统设置
 *
 * @param[in] iniPath     读取的ini配置文件路径
 * @param[in] groupName   读取的group名称
 * @param[in] key		 读取的键名
 */
QString readIniSettings(const QString iniPath, const QString groupName, const QString key);
} // namespace IniParser