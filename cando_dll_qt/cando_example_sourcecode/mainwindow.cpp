#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

enum {
    MaxStandardId = 0x7FF,
    MaxExtendedId = 0x10000000
};

HexIntegerValidator::HexIntegerValidator(QObject *parent) :
    QValidator(parent),
    m_maximum(MaxStandardId)
{
}

QValidator::State HexIntegerValidator::validate(QString &input, int &) const
{
    bool ok;
    uint value = input.toUInt(&ok, 16);

    if (input.isEmpty())
        return Intermediate;

    if (!ok || value > m_maximum)
        return Invalid;

    return Acceptable;
}

void HexIntegerValidator::setMaximum(uint maximum)
{
    m_maximum = maximum;
}

HexStringValidator::HexStringValidator(QObject *parent) :
    QValidator(parent),
    m_maxLength(8)
{
}

QValidator::State HexStringValidator::validate(QString &input, int &pos) const
{
    const int maxSize = 2 * m_maxLength;
    const QChar space = QLatin1Char(' ');
    QString data = input;
    data.remove(space);

    if (data.isEmpty())
        return Intermediate;

    // limit maximum size and forbid trailing spaces
    if ((data.size() > maxSize) || (data.size() == maxSize && input.endsWith(space)))
        return Invalid;

    // check if all input is valid
    const QRegularExpression re(QStringLiteral("^[[:xdigit:]]*$"));
    if (!re.match(data).hasMatch())
        return Invalid;

    // insert a space after every two hex nibbles
    const QRegularExpression insertSpace(QStringLiteral("(?:[[:xdigit:]]{2} )*[[:xdigit:]]{3}"));
    if (insertSpace.match(input).hasMatch()) {
        input.insert(input.size() - 1, space);
        pos = input.size();
    }

    return Acceptable;
}

void HexStringValidator::setMaxLength(int maxLength)
{
    m_maxLength = maxLength;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mCandoListHandle = nullptr;
    mCandoHandle = nullptr;

    mTimings
        // sample point: 50.0%
        << CandoTiming(48000000,    5000, 500, 960, 3, 5)
        << CandoTiming(48000000,   10000, 500, 300, 6, 8)
        << CandoTiming(48000000,   20000, 500, 150, 6, 8)
        << CandoTiming(48000000,   50000, 500,  60, 6, 8)
        << CandoTiming(48000000,   83333, 500,  36, 6, 8)
        << CandoTiming(48000000,  100000, 500,  30, 6, 8)
        << CandoTiming(48000000,  125000, 500,  24, 6, 8)
        << CandoTiming(48000000,  250000, 500,  12, 6, 8)
        << CandoTiming(48000000,  500000, 500,   6, 6, 8)
        << CandoTiming(48000000,  800000, 500,   3, 8, 9)
        << CandoTiming(48000000, 1000000, 500,   3, 6, 8)

        // sample point: 62.5%
        << CandoTiming(48000000,    5000, 625, 738, 6, 5)
        << CandoTiming(48000000,   10000, 625, 300, 8, 6)
        << CandoTiming(48000000,   20000, 625, 150, 8, 6)
        << CandoTiming(48000000,   50000, 625,  60, 8, 6)
        << CandoTiming(48000000,   83333, 625,  36, 8, 6)
        << CandoTiming(48000000,  100000, 625,  30, 8, 6)
        << CandoTiming(48000000,  125000, 625,  24, 8, 6)
        << CandoTiming(48000000,  250000, 625,  12, 8, 6)
        << CandoTiming(48000000,  500000, 625,   6, 8, 6)
        << CandoTiming(48000000,  800000, 600,   4, 7, 6)
        << CandoTiming(48000000, 1000000, 625,   3, 8, 6)

        // sample point: 75.0%
        << CandoTiming(48000000,    5000, 750, 800, 7, 3)
        << CandoTiming(48000000,   10000, 750, 300, 10, 4)
        << CandoTiming(48000000,   20000, 750, 150, 10, 4)
        << CandoTiming(48000000,   50000, 750,  60, 10, 4)
        << CandoTiming(48000000,   83333, 750,  36, 10, 4)
        << CandoTiming(48000000,  100000, 750,  30, 10, 4)
        << CandoTiming(48000000,  125000, 750,  24, 10, 4)
        << CandoTiming(48000000,  250000, 750,  12, 10, 4)
        << CandoTiming(48000000,  500000, 750,   6, 10, 4)
        << CandoTiming(48000000,  800000, 750,   3, 13, 5)
        << CandoTiming(48000000, 1000000, 750,   3, 10, 4)

        // sample point: 87.5%
        << CandoTiming(48000000,    5000, 875, 600, 12, 2)
        << CandoTiming(48000000,   10000, 875, 300, 12, 2)
        << CandoTiming(48000000,   20000, 875, 150, 12, 2)
        << CandoTiming(48000000,   50000, 875,  60, 12, 2)
        << CandoTiming(48000000,   83333, 875,  36, 12, 2)
        << CandoTiming(48000000,  100000, 875,  30, 12, 2)
        << CandoTiming(48000000,  125000, 875,  24, 12, 2)
        << CandoTiming(48000000,  250000, 875,  12, 12, 2)
        << CandoTiming(48000000,  500000, 875,   6, 12, 2)
        << CandoTiming(48000000,  800000, 867,   4, 11, 2)
        << CandoTiming(48000000, 1000000, 875,   3, 12, 2);

    for(int i=0; i<11; i++){
        ui->comboBaudRate->addItem(QString::number(mTimings[i].getBitrate()));
    }
    ui->comboBaudRate->setCurrentIndex(8);  // Set default baudrate 500K

    for(int i=0; i<4; i++){
        ui->comboSamplePoint->addItem(QString::number(static_cast<double>(mTimings[i*11].getSamplePoint())/10)+"%");
    }
    ui->comboSamplePoint->setCurrentIndex(3);   // Set default samplepoing 87.5%

    cando_list_malloc(&mCandoListHandle);
    cando_list_scan(mCandoListHandle);
    uint8_t cnt;
    cando_list_num(mCandoListHandle, &cnt);
    for(int i=0; i<cnt; i++){
        QString name = "Cando " + QString::number(i);
        ui->comboDevList->addItem(name);
    }
    if(cnt > 0){    // Device found
        ui->btnOpenClose->setEnabled(true);
    }else{          // No device connect
        ui->btnOpenClose->setEnabled(false);
        ui->textEditReceivedMessage->append("No found Cando devices! Please plugin Cando and reopen the software.");
    }

    mHexIntegerValidator = new HexIntegerValidator(this);
    ui->frameIdEdit->setValidator(mHexIntegerValidator);
    mHexStringValidator = new HexStringValidator(this);
    ui->payloadEdit->setValidator(mHexStringValidator);

    connect(ui->checkBoxExtended, &QCheckBox::toggled, [this](bool set) {
        mHexIntegerValidator->setMaximum(set ? MaxExtendedId : MaxStandardId);
    });

    auto frameIdTextChanged = [this]() {
        const bool hasFrameId = !ui->frameIdEdit->text().isEmpty();
        ui->btnSendMsg->setEnabled(hasFrameId);
        ui->btnSendMsg->setToolTip(hasFrameId ? QString() : tr("Cannot send because no Frame ID was given."));
    };
    connect(ui->frameIdEdit, &QLineEdit::textChanged, frameIdTextChanged);
    frameIdTextChanged();
}

MainWindow::~MainWindow()
{
    if(mCandoHandle != nullptr){
        close_device();
    }

    if(mCandoListHandle != nullptr){
        cando_list_free(mCandoListHandle);
    }

    delete ui;
}

void MainWindow::on_btnOpenClose_clicked()
{
    if(mCandoHandle == nullptr){
        if(open_device()){
            uint32_t sw_u, hw_u;
            cando_get_dev_info(mCandoHandle, &sw_u, &hw_u);
            double sw_f, hw_f;
            sw_f = sw_u / 10.0;
            hw_f = hw_u / 10.0;
            QString sw_str = QString::number(sw_f, 'f', 1);
            QString hw_str = QString::number(hw_f, 'f', 1);
            ui->labelDeviceInfo->setText("fw: " + sw_str + " hw: " + hw_str);
            ui->btnOpenClose->setText("Close");
            ui->comboDevList->setEnabled(false);
            ui->comboBaudRate->setEnabled(false);
            ui->comboSamplePoint->setEnabled(false);
            ui->checkBoxSilent->setEnabled(false);
            ui->checkBoxLoopBack->setEnabled(false);
            ui->checkBoxDisableRetrans->setEnabled(false);
        }else{
            ui->textEditReceivedMessage->append("Failed to open select device, is it in use?");
        }
    }else{
        close_device();
        ui->labelDeviceInfo->setText("");
        ui->btnOpenClose->setText("Open");
        ui->comboDevList->setEnabled(true);
        ui->comboBaudRate->setEnabled(true);
        ui->comboSamplePoint->setEnabled(true);
        ui->checkBoxSilent->setEnabled(true);
        ui->checkBoxLoopBack->setEnabled(true);
        ui->checkBoxDisableRetrans->setEnabled(true);
    }
}

void MainWindow::on_btnSendMsg_clicked()
{
    if(mCandoHandle == nullptr){
        ui->textEditReceivedMessage->append("Failed to send msg! Please open it first.");
        return;
    }

    const uint32_t frameId = ui->frameIdEdit->text().toUInt(nullptr, 16);
    QString data = ui->payloadEdit->text();
    const QByteArray payload = QByteArray::fromHex(data.remove(QLatin1Char(' ')).toLatin1());

    cando_frame_t frame;
    frame.can_id = frameId;

    if(ui->checkBoxExtended->isChecked()){
        frame.can_id |= CANDO_ID_EXTENDED;
    }

    if(ui->checkBoxRemote->isChecked()){
        frame.can_id |= CANDO_ID_RTR;
    }

    frame.can_dlc = static_cast<uint8_t>(payload.length());
    memcpy(frame.data, payload.data(), frame.can_dlc);

    if(!cando_frame_send(mCandoHandle, &frame)){
        ui->textEditReceivedMessage->append("Failed to send msg! Please check the device connection.");
    }
}

void MainWindow::on_msgReceived(cando_frame_t msg)
{
    if(msg.can_id & CANDO_ID_ERR){
        uint32_t err_code;
        uint8_t err_tx, err_rx;
        cando_parse_err_frame(&msg, &err_code, &err_tx, &err_rx);

        if(err_code & CAN_ERR_BUSOFF){
            ui->radioButtonBusOff->setChecked(true);
        }else{
            ui->radioButtonBusOff->setChecked(false);
        }

        if(err_code & CAN_ERR_RX_TX_WARNING){
            ui->radioButtonBusWarning->setChecked(true);
        }else{
            ui->radioButtonBusWarning->setChecked(false);
        }

        if(err_code & CAN_ERR_RX_TX_PASSIVE){
            ui->radioButtonBusPassive->setChecked(true);
        }else{
            ui->radioButtonBusPassive->setChecked(false);
        }

        if(err_code & (CAN_ERR_BUSOFF | CAN_ERR_RX_TX_WARNING | CAN_ERR_RX_TX_PASSIVE)){
            ui->radioButtonBusErrNone->setChecked(false);
        }else{
            ui->radioButtonBusErrNone->setChecked(true);
        }

        if(err_code & CAN_ERR_OVERLOAD){
            ui->labelOverRunsCnt->setText(QString::number(++mOverrunCnt));
        }

        ui->labelTxErrCnt->setText(QString::number(err_tx));
        ui->labelRxErrCnt->setText(QString::number(err_rx));
    }else{
        QString timestamp_str;
        QString dir_str;
        QString id_str;
        QString dlc_str;
        QString data_str;

        // Timestamp
        uint32_t s = msg.timestamp_us/1000000;
        uint32_t ms = (msg.timestamp_us % 1000000) / 1000;
        uint32_t us = msg.timestamp_us % 1000;
        timestamp_str = QString::fromLatin1("%1.%2.%3    ")
                .arg(s,  4, 10, QLatin1Char(' '))
                .arg(ms, 3, 10, QLatin1Char('0'))
                .arg(us, 3, 10, QLatin1Char('0'));

        // Dir
        if(msg.echo_id == 0xFFFFFFFF){
            dir_str = "Rx    ";
            ui->labelRxFramesCnt->setText(QString::number(++mRxFramesCnt));
        }else{
            dir_str = "Tx    ";
            ui->labelTxFramesCnt->setText(QString::number(++mTxFramesCnt));
        }

        // ID
        uint32_t id = msg.can_id & CANDO_ID_MASK;
        if(msg.can_id & CANDO_ID_EXTENDED){
            id_str = QString::fromLatin1("%1    ").arg(id, 7, 16, QLatin1Char('0')).toUpper();
        }else{
            id_str = QString::fromLatin1("%1    ").arg(id, 7, 16, QLatin1Char(' ')).toUpper();
        }

        // DLC
        dlc_str =  QString::fromLatin1("%1    ").arg(msg.can_dlc, 1, 10);

        // Data
        if(msg.can_id & CANDO_ID_RTR){
            data_str = "remote request";
        }else{
            for(int i=0; i<msg.can_dlc; i++){
                data_str += QString::fromLatin1("%1    ").arg(msg.data[i], 2, 16, QLatin1Char('0')).toUpper();
            }
        }

        ui->textEditReceivedMessage->append(timestamp_str + dir_str + id_str + dlc_str + data_str);
    }
}

void MainWindow::on_btnClearRec_clicked()
{
    ui->textEditReceivedMessage->clear();

    mTxFramesCnt = 0;
    mRxFramesCnt = 0;
    mOverrunCnt = 0;
    ui->labelTxFramesCnt->setText(QString::number(mTxFramesCnt));
    ui->labelRxFramesCnt->setText(QString::number(mRxFramesCnt));
    ui->labelOverRunsCnt->setText(QString::number(mOverrunCnt));

    ui->radioButtonBusErrNone->setChecked(true);
    ui->radioButtonBusOff->setChecked(false);
    ui->radioButtonBusPassive->setChecked(false);
    ui->radioButtonBusWarning->setChecked(false);
    ui->labelRxErrCnt->setText(QString::number(0));
    ui->labelTxErrCnt->setText(QString::number(0));
}

bool MainWindow::open_device()
{
    bool ret;
    int index;
    uint32_t mode = CANDO_MODE_NORMAL;
    cando_bittiming_t timing;

    // Open dev
    cando_malloc(mCandoListHandle, static_cast<uint8_t>(ui->comboDevList->currentIndex()), &mCandoHandle);
    ret = cando_open(mCandoHandle);
    if(ret != true){
        goto OPEN_ERR;
    }

    // Set timing
    index = ui->comboBaudRate->currentIndex() + ui->comboSamplePoint->currentIndex() * 11;
    timing = mTimings[index].getTiming();
    ret = cando_set_timing(mCandoHandle, &timing);
    if(ret != true){
        goto COMM_ERR;
    }

    if(ui->checkBoxSilent->isChecked()){
        mode |= CANDO_MODE_LISTEN_ONLY;
    }
    if(ui->checkBoxLoopBack->isChecked()){
        mode |= CANDO_MODE_LOOP_BACK;
    }
    if(ui->checkBoxDisableRetrans->isChecked()){
        mode |= CANDO_MODE_ONE_SHOT;
    }

    ret = cando_start(mCandoHandle, mode);
    if(ret != true){
        goto COMM_ERR;
    }

    // Start listener
    connect(&mCanListener, &CanListener::messageReceived, this, &MainWindow::on_msgReceived);
    mCanListener.startThread(mCandoHandle);

    return true;

COMM_ERR:
    cando_close(mCandoHandle);
OPEN_ERR:
    cando_free(mCandoHandle);
    mCandoHandle = nullptr;
    return false;
}

bool MainWindow::close_device()
{
    mCanListener.waitFinish();
    disconnect(&mCanListener, &CanListener::messageReceived, this, &MainWindow::on_msgReceived);
    cando_stop(mCandoHandle);
    cando_close(mCandoHandle);
    cando_free(mCandoHandle);
    mCandoHandle = nullptr;

    return true;
}
