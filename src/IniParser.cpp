#include "IniParser.h"

#include "Logger.h"

#include <QDebug>
#include <QSettings>

void IniParser::saveIniSettings(const QString iniPath, const QString groupName, const QString key, const QString value)
{
    LOGGER_INFO("Saving setting: {}, Value is: {}", key.toStdString(), value.toStdString());

    // 创建QSettings对象，指定.ini文件的路径和格式
    QSettings settings(iniPath, QSettings::IniFormat);

    // 写入配置
    settings.beginGroup(groupName);
    settings.setValue(key, value);
    settings.endGroup();
}

QString IniParser::readIniSettings(const QString iniPath, const QString groupName, const QString key)
{
    // 创建QSettings对象，指定.ini文件的路径和格式
    QSettings settings(iniPath, QSettings::IniFormat);

    // 读取配置
    settings.beginGroup(groupName);
    QString value = settings.value(key, "").toString(); // 使用空字符串作为默认值
    settings.endGroup();

    LOGGER_DEBUG("Reading setting: {}, Value: {}", key.toStdString(), value.toStdString());

    return value;
}
