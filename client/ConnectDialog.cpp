#include "ConnectDialog.h"

#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QGridLayout>

static const int DEFAULT_PORT = 53333;

ConnectDialog::ConnectDialog( const Logging::Config& loggingConfig,
                              QWidget*               parent )
  : mLogger( loggingConfig.createLogger() )
{
    QLabel* serverLabel = new QLabel(tr("&Server:"));
    QLabel* usernameLabel = new QLabel(tr("&Username:"));

    mServerComboBox = new QComboBox();
    mServerComboBox->setEditable( true );
    mServerComboBox->setInsertPolicy( QComboBox::NoInsert );
    const QString serverToolTipStr( "Specify server as <i>host:port</i>" );
    serverLabel->setToolTip( serverToolTipStr );
    mServerComboBox->setToolTip( serverToolTipStr );

    mUsernameLineEdit = new QLineEdit;

    serverLabel->setBuddy(mServerComboBox);
    usernameLabel->setBuddy(mUsernameLineEdit);

    mConnectButton = new QPushButton(tr("Connect"));

    mCancelButton = new QPushButton(tr("Cancel"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->addButton(mConnectButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(mCancelButton, QDialogButtonBox::RejectRole);
    mConnectButton->setDefault(true);
    mConnectButton->setFocus();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(serverLabel, 0, 0);
    mainLayout->addWidget(mServerComboBox, 0, 1);
    mainLayout->addWidget(usernameLabel, 1, 0);
    mainLayout->addWidget(mUsernameLineEdit, 1, 1);
    mainLayout->addWidget(buttonBox, 2, 0, 1, 2);
    mainLayout->setColumnMinimumWidth( 1, 250 );
    setLayout(mainLayout);

    setWindowTitle(tr("Connect to Server"));

    connect(mServerComboBox, SIGNAL(currentTextChanged(QString)),
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
ConnectDialog::setKnownServers( const QStringList& servers )
{
    while( mServerComboBox->count() > 0 ) mServerComboBox->removeItem( 0 );
    mServerComboBox->insertItems( 0, servers );
}


void
ConnectDialog::addKnownServer( const QString& server )
{
    mServerComboBox->insertItem( 0, server );
}


void
ConnectDialog::setLastGoodServer( QString server )
{
    if( !server.isEmpty() )
    {
        mServerComboBox->setCurrentText( server );
    }
}


void
ConnectDialog::setLastGoodUsername( QString name )
{
    mUsernameLineEdit->setText( name );
}


QString
ConnectDialog::getServer() const
{
    return mServerComboBox->currentText();
}


QString
ConnectDialog::getServerHost() const
{
    return mUrl.host();
}


int
ConnectDialog::getServerPort() const
{
    return (mUrl.port() > 0) ? mUrl.port() : DEFAULT_PORT;
}


QString
ConnectDialog::getUsername() const
{
    return mUsernameLineEdit->text();
}


void
ConnectDialog::tryEnableConnectButton()
{
    mUrl.setAuthority( mServerComboBox->currentText() );
    mConnectButton->setEnabled( !mUrl.host().isEmpty() );
}


