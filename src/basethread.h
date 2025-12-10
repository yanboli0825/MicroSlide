#pragma once

#include <QObject>
#include <windows.h>

class BaseThread : public QObject
{
    Q_OBJECT
public:
    explicit BaseThread(QObject* parent = nullptr);
    bool is_stopped();
    bool is_closed();

private:
    volatile bool m_stopped;
    volatile bool m_closed;

public slots:
    virtual void working() = 0;
    void start();
    void stop();
    void open();
    void close();
};
