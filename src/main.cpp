#include "Logger.h"
#include "MicroSlideWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[])
{
    // 初始化日志系统
#ifdef NDEBUG
    Logger::init("./log", "info");
#else
    Logger::init("./log", "debug");
#endif
    LOGGER_INFO("========== Application Started ==========");

    QApplication a(argc, argv);
    std::setbuf(stdout, NULL);

    {
        // 使用作用域确保主窗口在 a.exec() 之前完全析构
        MicroSlideWindow mw;
        mw.setWindowTitle("MicroSlide");
        mw.setWindowIcon(QIcon("../../../assets/icons/bitbug_favicon.ico"));
        mw.move(0, 100);
        mw.show();

        a.exec();
        // 离开作用域时，mw 以及所有子对象会完全析构
    }

    LOGGER_INFO("========== Application Exited ==========");
    Logger::shutdown();

    return 0;
}
