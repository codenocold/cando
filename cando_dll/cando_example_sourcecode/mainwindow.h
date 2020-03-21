#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QValidator>
#include <QMessageBox>
#include <stdlib.h>
#include "cando.h"
#include "CandoTiming.h"
#include "CanListener.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class HexIntegerValidator : public QValidator
{
    Q_OBJECT
public:
    explicit HexIntegerValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &) const;

    void setMaximum(uint maximum);

private:
    uint m_maximum = 0;
};

class HexStringValidator : public QValidator
{
    Q_OBJECT

public:
    explicit HexStringValidator(QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const;

    void setMaxLength(int maxLength);

private:
    int m_maxLength = 0;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnOpenClose_clicked();

    void on_btnSendMsg_clicked();

    void on_msgReceived(cando_frame_t msg);

    void on_btnClearRec_clicked();

private:
    bool open_device(void);
    bool close_device(void);

private:
    Ui::MainWindow *ui;
    cando_list_handle mCandoListHandle;
    cando_handle mCandoHandle;
    CanListener mCanListener;
    QList<CandoTiming> mTimings;
    HexIntegerValidator *mHexIntegerValidator = nullptr;
    HexStringValidator *mHexStringValidator = nullptr;

    uint64_t mRxFramesCnt = 0;
    uint64_t mTxFramesCnt = 0;
    uint64_t mOverrunCnt = 0;
};
#endif // MAINWINDOW_H
