#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

#include "Logging.h"

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    ConnectDialog( const Logging::Config& loggingConfig = Logging::Config(),
                   QWidget*               parent = 0 );

    // Set/add servers stored by the dialog for easy recall by user.
    void setKnownServers( const QStringList& servers );
    void addKnownServer( const QString& server );

    void setLastGoodServer( QString server );
    void setLastGoodUsername( QString name );

    QString getServer() const;
    QString getServerHost() const;
    int getServerPort() const;
    QString getUsername() const;

private slots:
    void tryEnableConnectButton();

private:

    QComboBox *mServerComboBox;
    QLineEdit *mUsernameLineEdit;
    QPushButton *mConnectButton;
    QPushButton *mCancelButton;

    QUrl mUrl;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CONNECTDIALOG_H
