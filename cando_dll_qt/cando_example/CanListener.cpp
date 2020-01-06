#include "CanListener.h"
#include <QThread>

CanListener::CanListener(QObject *parent)
  : QObject(parent)
{
    _thread = new QThread();
    qRegisterMetaType<cando_frame_t>("cando_frame_t");
}

CanListener::~CanListener()
{
    delete _thread;
}

void CanListener::run()
{
    cando_frame_t msg;
    while (_shouldBeRunning) {
        if (cando_frame_read(mHandle, &msg, 10)) {
            emit messageReceived(msg);
        }
    }
    _thread->quit();
}

void CanListener::startThread(cando_handle handle)
{
    mHandle = handle;
    _shouldBeRunning = true;
    moveToThread(_thread);
    connect(_thread, SIGNAL(started()), this, SLOT(run()));
    _thread->start();
}

void CanListener::requestStop()
{
    _shouldBeRunning = false;
}

void CanListener::waitFinish()
{
    requestStop();
    _thread->wait();
}
