#include "exitwaitdialog.h"
#include "ui_exitwaitdialog.h"
#include <QCloseEvent>

ExitWaitDialog::ExitWaitDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExitWaitDialog)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    if (parent != nullptr)
    {
        // Center shutdown window at where main window was
        const QPoint global = parent->mapToGlobal(parent->rect().center());
        move(global.x() - width() / 2, global.y() - height() / 2);
    }
}

ExitWaitDialog::~ExitWaitDialog()
{
    delete ui;
}

void ExitWaitDialog::closeEvent(QCloseEvent *event)
{
    event->ignore();
}

