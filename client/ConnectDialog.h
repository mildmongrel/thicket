#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

#include "Logging.h"

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    ConnectDialog( const QString&         defaultHost,
                   int                    defaultPort,
                   const QString&         defaultName,
                   const Logging::Config& loggingConfig = Logging::Config(),
                   QWidget*               parent = 0 );

    QString getHost() const;
    int getPort() const;
    QString getName() const;

private slots:
    void tryEnableConnectButton();

private:

    QLabel *mHostLabel;
    QLabel *mPortLabel;
    QLabel *mNameLabel;
    QLineEdit *mHostLineEdit;
    QLineEdit *mPortLineEdit;
    QLineEdit *mNameLineEdit;
    QPushButton *mConnectButton;
    QPushButton *mCancelButton;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CONNECTDIALOG_H
