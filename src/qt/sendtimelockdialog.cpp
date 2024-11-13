#include "sendtimelockdialog.h"
#include "ui_sendtimelockdialog.h"
#include "optionsmodel.h"
#include "bitcoinunits.h"
#include "addressbookpage.h"
#include "coincontroldialog.h"
#include "sendcoinsentry.h"

#include <QDoubleSpinBox>
#include <QMessageBox>

sendtimelockdialog::sendtimelockdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::sendtimelockdialog),
    wModel(0),
    cModel(0)
{
    ui->setupUi(this);

#if (defined (WIN32) || defined (WIN64))
    setMinimumSize(360, 280);
    resize(360, 280);
#elif (defined (LINUX) || defined (_linux_))
    setMinimumSize(420, 330);
    resize(420, 330);
#else
    setMinimumSize(400, 320);
    resize(400, 320);
#endif

    ui->radioHeight->setChecked(true);

    ui->dateTimelock->setDateTime(QDateTime::currentDateTimeUtc());

    on_buttonConvert_clicked();

    ui->radioHeight->hide();
    ui->radioStamp->hide();
    ui->dateTimelock->hide();
    ui->buttonConvert->hide();
    ui->lockStamp->hide();
    ui->buttonConvert2->hide();
}

sendtimelockdialog::~sendtimelockdialog()
{
    delete ui;
}

void sendtimelockdialog::setModels(WalletModel *mw, ClientModel *mc)
{
    wModel = mw;

    if(wModel)
    {
        setBalance(wModel->getBalance(), 0, 0, 0);
        connect(wModel, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));
    }

    cModel = mc;
    if (cModel)
    {
        setNumBlocks(cModel->getNumBlocks(), 0);
        connect(cModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));
    }
}

void sendtimelockdialog::on_radioHeight_toggled(bool checked)
{
    if (checked)
    {
        ui->lockHeight->show();
        ui->buttonGetHeight->show();
        ui->lockStamp->hide();
        ui->dateTimelock->hide();
        ui->buttonConvert->hide();
        ui->buttonConvert2->hide();
    }
}


void sendtimelockdialog::on_radioStamp_toggled(bool checked)
{
    if (checked)
    {
        ui->lockHeight->hide();
        ui->buttonGetHeight->hide();
        ui->lockStamp->show();
        ui->dateTimelock->show();
        ui->buttonConvert->show();
        ui->buttonConvert2->show();
    }
}

qint64 sendtimelockdialog::getUnixTimestampFromUTCTime(const QString& timeString)
{
    // Define the format of the input time string
    QString format = "dd/MM/yyyy hh:mm:ss 'UTC'";

    // Parse the time string into a QDateTime object
    QDateTime dateTime = QDateTime::fromString(timeString, format);

    // Set the time zone to UTC
    dateTime.setTimeSpec(Qt::UTC);

    // Convert the QDateTime to a Unix timestamp (seconds since epoch)
    qint64 unixTimestamp = dateTime.toSecsSinceEpoch();

    return unixTimestamp;
}

void sendtimelockdialog::on_buttonConvert_clicked()
{
    QString timeString = ui->dateTimelock->text();
    qint64 unixTimestamp = getUnixTimestampFromUTCTime(timeString);

    ui->lockStamp->setText(QString::number(unixTimestamp));
}


void sendtimelockdialog::on_buttonConvert2_clicked()
{
    qint64 unixTimestamp = ui->lockStamp->text().trimmed().toLongLong();

    // Create a QDateTime object from the Unix timestamp
    QDateTime dateTime = QDateTime::fromSecsSinceEpoch(unixTimestamp, Qt::UTC);

    // Set the parsed QDateTime into the QDateTimeEdit widget
    ui->dateTimelock->setDateTime(dateTime);
}

bool sendtimelockdialog::validate()
{
    // Check input validity
    bool retval = true;

    if (!ui->payTo->hasAcceptableInput() || (wModel && !wModel->validateAddress(ui->payTo->text())))
    {
        ui->payTo->setValid(false);
        retval = false;
    }


    if (!ui->payAmount->validate())
        retval = false;
    else
    {
        if (ui->payAmount->value() <= 0)
        {
            // Cannot send 0 coins or less
            ui->payAmount->setValid(false);
            retval = false;
        }
    }

    int uiHeight = ui->lockHeight->text().trimmed().toInt();
    if (uiHeight <= 0)
    {
        ui->lockHeight->setValid(false);
        retval = false;
    }

    return retval;
}

void sendtimelockdialog::setBlockHeight(int height)
{
    ui->labelHeight->setText(tr(" (Current Block Height: ") + QString::number(height) + tr(")"));
}

void sendtimelockdialog::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 reserveBalance)
{
    Q_UNUSED(stake);
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(reserveBalance);
    if (!wModel || !wModel->getOptionsModel())
        return;

    int unit = wModel->getOptionsModel()->getDisplayUnit();
    ui->labelBalance->setText(tr(" (max: ") +BitcoinUnits::formatWithUnit(unit, balance) + tr(")"));
}

void sendtimelockdialog::setNumBlocks(int count, int countOfPeers)
{
    Q_UNUSED(countOfPeers);
    setBlockHeight(count);
}

void sendtimelockdialog::on_addrButton_clicked()
{
    if(!wModel)
        return;
    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
    dlg.setModel(wModel->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void sendtimelockdialog::on_sendButton_clicked()
{
    if (!validate())
        return;

    uint32_t locktime = 0;

    if (ui->radioHeight->isChecked())
    {
        if (!cModel)
            return;

        qint64 maxBalance = wModel->getBalance();
        qint64 sendBalance = ui->payAmount->value();
        if (sendBalance > maxBalance)
        {
            ui->payAmount->setValid(false);
            return;
        }

        int latestBlockHeight = cModel->getNumBlocks();
        int uiHeight = ui->lockHeight->text().trimmed().toInt();
        if (uiHeight <= latestBlockHeight)
        {
            ui->lockHeight->setValid(false);
            return;
        }

        locktime = (unsigned int)uiHeight;


        QString msg = tr("After latest block height reaches %1, the transaction can only be confirmed.<br><br>").arg(QString::number(uiHeight));
        QString formatted = tr("Are you sure you want to send <b>%1</b> to %2?")
                            .arg(BitcoinUnits::formatWithUnit(wModel->getOptionsModel()->getDisplayUnit(), ui->payAmount->value()), ui->payTo->text());
        msg.append(formatted);

        QMessageBox::StandardButton ret = QMessageBox::question(this, tr("Confirm send coins with timelock"),
                                                                msg,
                                                                QMessageBox::Yes|QMessageBox::Cancel,
                                                                QMessageBox::Cancel);

        if (ret != QMessageBox::Yes)
            return;

    }
    else if (ui->radioStamp->isChecked())
    {
       QString timeString = ui->dateTimelock->text();
       qint64 unixTimestamp1 = getUnixTimestampFromUTCTime(timeString);
       qint64 unixTimestamp2 = ui->lockStamp->text().trimmed().toLongLong();       
       if (unixTimestamp1 != unixTimestamp2)
       {
           ui->lockStamp->setValid(false);
           return;
       }

       locktime = (unsigned int)unixTimestamp2;

       QString msg = tr("After %1, the transaction can only be confirmed.<br><br>").arg(timeString);
       QString formatted = tr("Are you sure you want to send <b>%1</b> to %2?")
                           .arg(BitcoinUnits::formatWithUnit(wModel->getOptionsModel()->getDisplayUnit(), ui->payAmount->value()), ui->payTo->text());
       msg.append(formatted);

       QMessageBox::StandardButton ret = QMessageBox::question(this, tr("Confirm send coins with timelock"),
                                                               msg,
                                                               QMessageBox::Yes|QMessageBox::Cancel,
                                                               QMessageBox::Cancel);

       if (ret != QMessageBox::Yes)
           return;
    }


    WalletModel::UnlockContext ctx(wModel->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    WalletModel::SendCoinsReturn sendstatus;

    QList<SendCoinsRecipient> recipients;

    SendCoinsRecipient rv;
    rv.address = ui->payTo->text();
    rv.label = "";
    rv.amount = ui->payAmount->value();
    recipients.append(rv);

    if (!wModel->getOptionsModel() || !wModel->getOptionsModel()->getCoinControlFeatures())
        sendstatus = wModel->sendCoins(recipients, NULL, locktime);
    else
        sendstatus = wModel->sendCoins(recipients, CoinControlDialog::coinControl, locktime);

    QString title = tr("Send coins with timelock");

    switch (sendstatus.status)
    {
    case WalletModel::InvalidAddress:
        QMessageBox::warning(this, title, tr("The recipient address is not valid, please recheck."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::InvalidAmount:
        QMessageBox::warning(this, title, tr("The amount to pay must be larger than 0."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountExceedsBalance:
        QMessageBox::warning(this, title, tr("The amount exceeds your balance."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        QMessageBox::warning(this, title, tr("The total exceeds your balance when the %1 transaction fee is included.").
            arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee)),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::DuplicateAddress:
        QMessageBox::warning(this, title, tr("Duplicate address found, can only send to each address once per send operation."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCreationFailed:
        QMessageBox::warning(this, title, tr("Error: Transaction creation failed."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::TransactionCommitFailed:
        QMessageBox::warning(this, title,
            tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."),
            QMessageBox::Ok, QMessageBox::Ok);
        break;
    case WalletModel::BadBurningCoins: // there is no such thing as BadBurningCoins when burning coins
      break;
    case WalletModel::Aborted: // User aborted, nothing to do
        break;
    case WalletModel::OK:
        accept();
        break;
    }
}


void sendtimelockdialog::on_buttonGetHeight_clicked()
{
    if (!cModel)
        return;

    int height = cModel->getNumBlocks();
    ui->lockHeight->setText(QString::number(height));
}

