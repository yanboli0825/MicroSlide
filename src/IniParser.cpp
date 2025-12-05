#include "IniParser.h"
#include <QDebug>
#include <QSettings>

void IniParser::saveIniSettings(QString iniPath, QString groupName, QString key, QString value)
{
    qDebug() << "Save setting:" << key << ",Value is:" << value;

    QSettings settings(iniPath, QSettings::IniFormat); // 创建QSettings对象，指定.ini文件的路径和格式

    // 写入配置
    settings.beginGroup(groupName);
    settings.setValue(key, value);
    settings.endGroup();
}

QString IniParser::readIniSettings(QString iniPath, QString groupName, QString key)
{
    QSettings settings(iniPath, QSettings::IniFormat); // 创建QSettings对象，指定.ini文件的路径和格式

    // 读取配置
    settings.beginGroup(groupName);
    QString value = settings.value(key, "").toString(); // 使用空字符串作为默认值
    settings.endGroup();

    qDebug() << "Reading setting:" << key << "Value:" << value;

    return value;
}
