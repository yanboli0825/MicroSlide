#include <QApplication>
#include <QIcon>
#include "bgCamera.h"
#include "microslidewindow.h"

int main(int argc, char* argv[])
{
    // 适应高分辨率，似乎已经不再生效
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QApplication a(argc, argv);
    std::setbuf(stdout, NULL);

    MicroSlideWindow mw;
    mw.setWindowTitle("MicroSlide");
    mw.setWindowIcon(QIcon("../../../assets/icons/bitbug_favicon.ico"));
    mw.move(0, 100);
    mw.show();

    return a.exec();
}
