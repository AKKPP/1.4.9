#ifndef QUTCDATETIMEEDIT_H
#define QUTCDATETIMEEDIT_H

#include <QDateTimeEdit>
#include <QDateTime>
#include <QVBoxLayout>
#include <QWidget>

class QUTCDateTimeEdit : public QDateTimeEdit
{
    Q_OBJECT

public:
    explicit QUTCDateTimeEdit(QWidget *parent = nullptr)
        : QDateTimeEdit(parent)
    {
        // Set the current date/time in UTC
        setDateTime(QDateTime::currentDateTimeUtc());

        // Set display format without manually appending "UTC"
        setDisplayFormat("dd/MM/yyyy HH:mm:ss");

        // Lock the time zone to UTC so that time is always in UTC
        setTimeSpec(Qt::UTC);
    }

protected:
    // Override textFromDateTime to append " UTC"
    QString textFromDateTime(const QDateTime &datetime) const override
    {
        return QDateTimeEdit::textFromDateTime(datetime) + " UTC";
    }
};

#endif // QUTCDATETIMEEDIT_H
