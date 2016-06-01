#include "ConnectDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>

ConnectDialog::ConnectDialog( const QString&         defaultHost,
                              int                    defaultPort,
                              const QString&         defaultName,
                              const Logging::Config& loggingConfig,
                              QWidget*               parent )
  : mLogger( loggingConfig.createLogger() )
{
    mHostLabel = new QLabel(tr("&Server:"));
    mNameLabel = new QLabel(tr("&Username:"));

    mHostLineEdit = new QLineEdit;
    mHostLineEdit->setText( defaultHost + ":" + QString::number( defaultPort ) );

    const QString hostToolTipStr( "Specify server as <i>host:port</i>" );
    mHostLabel->setToolTip( hostToolTipStr );
    mHostLineEdit->setToolTip( hostToolTipStr );

    mNameLineEdit = new QLineEdit;
    mNameLineEdit->setText( defaultName );

    mHostLabel->setBuddy(mHostLineEdit);
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
    mainLayout->addWidget(mNameLabel, 1, 0);
    mainLayout->addWidget(mNameLineEdit, 1, 1);
    mainLayout->addWidget(buttonBox, 2, 0, 1, 2);
    mainLayout->setColumnMinimumWidth( 1, 250 );
    setLayout(mainLayout);

    setWindowTitle(tr("Connect to Server"));

    connect(mHostLineEdit, SIGNAL(textChanged(QString)),
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
    mUrl.setAuthority( mHostLineEdit->text() );
    mConnectButton->setEnabled( !mUrl.host().isEmpty() && (mUrl.port() > 0) );
}


QString
ConnectDialog::getHost() const
{
    return mUrl.host();
}


int
ConnectDialog::getPort() const
{
    return mUrl.port();
}


QString
ConnectDialog::getName() const
{
    return mNameLineEdit->text();
}

