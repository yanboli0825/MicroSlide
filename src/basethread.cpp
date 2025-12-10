#include "BaseThread.h"

BaseThread::BaseThread(QObject* parent)
    : m_stopped(true)
    , m_closed(false)
{
}

bool BaseThread::is_stopped()
{
    return m_stopped;
}

bool BaseThread::is_closed()
{
    return m_closed;
}

void BaseThread::start()
{
    printf("start\n");
    m_stopped = false;
}

void BaseThread::stop()
{
    printf("stop\n");
    m_stopped = true;
}

void BaseThread::open()
{
    printf("open\n");
    m_closed = false;
}

void BaseThread::close()
{
    printf("close\n");
    m_closed = true;
}
