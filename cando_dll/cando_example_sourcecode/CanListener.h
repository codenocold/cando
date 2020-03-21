#pragma once

#include <QObject>
#include "cando.h"

class CanListener : public QObject
{
    Q_OBJECT

public:
    explicit CanListener(QObject *parent = nullptr);
    virtual ~CanListener();

signals:
    void messageReceived(cando_frame_t msg);

public slots:
    void run();

    void startThread(cando_handle handle);
    void requestStop();
    void waitFinish();

private:
    bool _shouldBeRunning;
    QThread *_thread;
    cando_handle mHandle;
};
