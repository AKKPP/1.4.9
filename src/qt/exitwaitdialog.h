#ifndef EXITWAITDIALOG_H
#define EXITWAITDIALOG_H

#include <QDialog>

namespace Ui {
class ExitWaitDialog;
}

class ExitWaitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitWaitDialog(QWidget *parent = nullptr);
    ~ExitWaitDialog();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ExitWaitDialog *ui;
};

#endif // EXITWAITDIALOG_H
