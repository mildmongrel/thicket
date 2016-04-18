#include "ConnectDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QIntValidator>
#include <QPushButton>
#include <QGridLayout>
#include <QHostInfo>
#include <QNetworkInterface>

ConnectDialog::ConnectDialog( const QString&         defaultHost,
                              int                    defaultPort,
                              const QString&         defaultName,
                              const Logging::Config& loggingConfig,
                              QWidget*               parent )
  : mLogger( loggingConfig.createLogger() )
{
    mHostLabel = new QLabel(tr("&Server name:"));
    mPortLabel = new QLabel(tr("Server &port:"));
    mNameLabel = new QLabel(tr("&Player name:"));

    mHostLineEdit = new QLineEdit;
    mHostLineEdit->setText( defaultHost );

    mPortLineEdit = new QLineEdit;
    mPortLineEdit->setText( QString::number( defaultPort ) );
    mPortLineEdit->setValidator(new QIntValidator(1, 65535, this));

    mNameLineEdit = new QLineEdit;
    mNameLineEdit->setText( defaultName );

    mHostLabel->setBuddy(mHostLineEdit);
    mPortLabel->setBuddy(mPortLineEdit);
    mNameLabel->setBuddy(mNameLineEdit);

    mConnectButton = new QPushButton(tr("Connect"));

    mCancelButton = new QPushButton(tr("Cancel"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(mConnectButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(mCancelButton, QDialogButtonBox::RejectRole);
    mConnectButton->setDefault(true);
    mConnectButton->setFocus();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(mHostLabel, 0, 0);
    mainLayout->addWidget(mHostLineEdit, 0, 1);
    mainLayout->addWidget(mPortLabel, 1, 0);
    mainLayout->addWidget(mPortLineEdit, 1, 1);
    mainLayout->addWidget(mNameLabel, 2, 0);
    mainLayout->addWidget(mNameLineEdit, 2, 1);
    mainLayout->addWidget(buttonBox, 3, 0, 1, 2);
    mainLayout->setColumnMinimumWidth( 1, 250 );
    setLayout(mainLayout);

    setWindowTitle(tr("Connect to Server"));

    connect(mHostLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(tryEnableConnectButton()));
    connect(mPortLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(tryEnableConnectButton()));
    connect(mConnectButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(mCancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));

    // This addresses a Qt oddity where a dialog forgets default buttons
    // after it finishes.  Since this dialog may be reused, reset the
    // default button.
    connect( this, &QDialog::finished,
             this, [this](int result) { mConnectButton->setDefault( true ); } );

    tryEnableConnectButton();
}


void
ConnectDialog::tryEnableConnectButton()
{
    mConnectButton->setEnabled(
            !mHostLineEdit->text().isEmpty() &&
            !mPortLineEdit->text().isEmpty() );
}


QString
ConnectDialog::getHost() const
{
    return mHostLineEdit->text();
}


int
ConnectDialog::getPort() const
{
    return mPortLineEdit->text().toInt();
}


QString
ConnectDialog::getName() const
{
    return mNameLineEdit->text();
}

