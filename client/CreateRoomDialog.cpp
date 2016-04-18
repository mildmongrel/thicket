#include "CreateRoomDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QGroupBox>

#include <QStandardItemModel>
#include <QTreeView>

CreateRoomDialog::CreateRoomDialog( const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : mLogger( loggingConfig.createLogger() )
{
    QLabel* roomNameLabel = new QLabel(tr("Room &Name:"));
    QLabel* passwordLabel = new QLabel(tr("&Password:"));
    QLabel* chairCountLabel = new QLabel(tr("# of &Chairs:"));
    QLabel* botCountLabel = new QLabel(tr("# of &Bots:"));
    QLabel* selectionTimeLabel = new QLabel(tr("Selection Timer:"));

    mChairCountComboBox = new QComboBox();
    for( int i = 4; i <= 8; ++i )
    {
        mChairCountComboBox->addItem( QString::number( i ) );
    }

    mBotCountComboBox = new QComboBox();
    for( int i = 0; i <= 7; ++i )
    {
        mBotCountComboBox->addItem( QString::number( i ) );
    }

    mRoomNameLineEdit = new QLineEdit();
    connect( mRoomNameLineEdit, &QLineEdit::textChanged, this, &CreateRoomDialog::tryEnableCreateButton );

    mPasswordLineEdit = new QLineEdit();

    mSelectionTimeCheckBox = new QCheckBox( "Enabled" );
    mSelectionTimeCheckBox->setChecked( true );
    connect( mSelectionTimeCheckBox, &QCheckBox::toggled, this, &CreateRoomDialog::handleselectionTimeCheckBoxToggled );

    mSelectionTimeComboBox = new QComboBox();
    mSelectionTimeComboBox->setEditable( true );
    mSelectionTimeComboBox->setInsertPolicy( QComboBox::NoInsert );
    mSelectionTimeComboBox->setValidator(new QIntValidator(1, 600, this));
    std::vector<int> selectionTimes = { 30, 60, 90, 120 };
    for( auto t : selectionTimes )
    {
        mSelectionTimeComboBox->addItem( QString::number( t ) );
    }
    mSelectionTimeComboBox->setCurrentIndex( 1 ); // 60s

    QHBoxLayout* selectionTimeLayout = new QHBoxLayout;
    selectionTimeLayout->addWidget( mSelectionTimeCheckBox );
    selectionTimeLayout->addWidget( mSelectionTimeComboBox );

    roomNameLabel->setBuddy( mRoomNameLineEdit );
    passwordLabel->setBuddy( mPasswordLineEdit );
    chairCountLabel->setBuddy( mChairCountComboBox );
    botCountLabel->setBuddy( mBotCountComboBox );
    selectionTimeLabel->setBuddy( mSelectionTimeComboBox );

    QGridLayout* packLayout = new QGridLayout;
    for( int i = 0; i < 3; ++i )
    {
        QLabel* roundLabel = new QLabel( "Round " + QString::number(i+1) + ": " );
        mPackComboBox[i] = new QComboBox();
        packLayout->addWidget( roundLabel,       i, 0 );
        packLayout->addWidget( mPackComboBox[i], i, 1 );
    }

    QGroupBox* packGroupBox = new QGroupBox( "Booster Packs" );
    packGroupBox->setLayout( packLayout );

    mCreateButton = new QPushButton( tr("Create") );
    connect( mCreateButton, &QPushButton::clicked, this, &CreateRoomDialog::accept );

    QPushButton* cancelButton = new QPushButton( tr("Cancel") );
    connect( cancelButton, &QPushButton::clicked, this, &CreateRoomDialog::reject );

    QDialogButtonBox* buttonBox = new QDialogButtonBox;
    buttonBox->addButton( mCreateButton, QDialogButtonBox::ActionRole );
    buttonBox->addButton( cancelButton, QDialogButtonBox::RejectRole );
    mCreateButton->setDefault( true );
    mCreateButton->setFocus();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget( roomNameLabel,          0, 0 );
    mainLayout->addWidget( mRoomNameLineEdit,      0, 1 );
    mainLayout->addWidget( passwordLabel,          1, 0 );
    mainLayout->addWidget( mPasswordLineEdit,      1, 1 );
    mainLayout->addWidget( chairCountLabel,        2, 0 );
    mainLayout->addWidget( mChairCountComboBox,    2, 1, Qt::AlignLeft );
    mainLayout->addWidget( botCountLabel,          3, 0 );
    mainLayout->addWidget( mBotCountComboBox,      3, 1, Qt::AlignLeft );
    mainLayout->addWidget( packGroupBox,           4, 0, 1, 2 );
    mainLayout->addWidget( selectionTimeLabel,     5, 0 );
    mainLayout->addLayout( selectionTimeLayout,    5, 1, Qt::AlignLeft );
    mainLayout->addWidget( buttonBox,              6, 0, 1, 2 );
    mainLayout->setColumnStretch( 1, 1 );
    setLayout(mainLayout);

    setWindowTitle( tr("Create Room") );

    tryEnableCreateButton();
}


void
CreateRoomDialog::setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets )
{
    for( int i = 0; i < 3; ++i )
    {
        mPackComboBox[i]->clear();
        for( auto set : sets )
        {
            if( set.boosterGen )
            {
                QString code = QString::fromStdString( set.code );
                QString name = QString::fromStdString( set.name );
                mPackComboBox[i]->addItem( QString( "%1 - %2" ).arg( code ).arg( name ), code );
            }
        }
    }
}


void
CreateRoomDialog::tryEnableCreateButton()
{
    mCreateButton->setEnabled( !mRoomNameLineEdit->text().isEmpty() );
}


void
CreateRoomDialog::handleselectionTimeCheckBoxToggled( bool checked )
{
    mSelectionTimeComboBox->setEnabled( checked );
}


QString
CreateRoomDialog::getRoomName() const
{
    return mRoomNameLineEdit->text();
}


QString
CreateRoomDialog::getPassword() const
{
    return mPasswordLineEdit->text();
}


QStringList
CreateRoomDialog::getSetCodes() const
{
    QStringList setCodes;
    for( int i = 0; i < 3; ++i )
    {
        setCodes.append( mPackComboBox[i]->itemData( mPackComboBox[i]->currentIndex() ).toString() );
    }
    return setCodes;
}


int
CreateRoomDialog::getChairCount() const
{
    return mChairCountComboBox->currentText().toInt();
}


int
CreateRoomDialog::getBotCount() const
{
    return mBotCountComboBox->currentText().toInt();
}

int
CreateRoomDialog::getSelectionTime() const
{
    return mSelectionTimeCheckBox->isChecked() ? mSelectionTimeComboBox->currentText().toInt() : 0;
}
