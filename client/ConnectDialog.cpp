#include "ConnectDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>

static const int DEFAULT_PORT = 53333;

ConnectDialog::ConnectDialog( const QString&         defaultHost,
                              int                    defaultPort,
                              const QString&         defaultName,
                              const Logging::Config& loggingConfig,
                              QWidget*               parent )
  : mLogger( loggingConfig.createLogger() )
{
    mServerLabel = new QLabel(tr("&Server:"));
    mNameLabel = new QLabel(tr("&Username:"));

    mServerLineEdit = new QLineEdit;
    QString serverText = defaultHost;
    if( defaultPort != DEFAULT_PORT )
    {
        serverText.append( ":" + QString::number( defaultPort ) );
    }
    mServerLineEdit->setText( serverText );

    const QString serverToolTipStr( "Specify server as <i>host:port</i>" );
    mServerLabel->setToolTip( serverToolTipStr );
    mServerLineEdit->setToolTip( serverToolTipStr );

    mNameLineEdit = new QLineEdit;
    mNameLineEdit->setText( defaultName );

    mServerLabel->setBuddy(mServerLineEdit);
    mNameLabel->setBuddy(mNameLineEdit);

    mConnectButton = new QPushButton(tr("Connect"));

    mCancelButton = new QPushButton(tr("Cancel"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(mConnectButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(mCancelButton, QDialogButtonBox::RejectRole);
    mConnectButton->setDefault(true);
    mConnectButton->setFocus();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(mServerLabel, 0, 0);
    mainLayout->addWidget(mServerLineEdit, 0, 1);
    mainLayout->addWidget(mNameLabel, 1, 0);
    mainLayout->addWidget(mNameLineEdit, 1, 1);
    mainLayout->addWidget(buttonBox, 2, 0, 1, 2);
    mainLayout->setColumnMinimumWidth( 1, 250 );
    setLayout(mainLayout);

    setWindowTitle(tr("Connect to Server"));

    connect(mServerLineEdit, SIGNAL(textChanged(QString)),
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
    mUrl.setAuthority( mServerLineEdit->text() );
    mConnectButton->setEnabled( !mUrl.host().isEmpty() );
}


QString
ConnectDialog::getHost() const
{
    return mUrl.host();
}


int
ConnectDialog::getPort() const
{
    return (mUrl.port() > 0) ? mUrl.port() : DEFAULT_PORT;
}


QString
ConnectDialog::getName() const
{
    return mNameLineEdit->text();
}

