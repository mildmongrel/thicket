#include "CreateRoomDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QStackedWidget>

#include <QApplication>
#include <QDesktopWidget>

#include <QStandardItemModel>
#include <QTreeView>

CreateRoomDialog::CreateRoomDialog( const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : QDialog( parent ),
    mLogger( loggingConfig.createLogger() )
{
    QLabel* roomNameLabel = new QLabel(tr("Room &Name:"));
    QLabel* passwordLabel = new QLabel(tr("&Password:"));
    QLabel* draftTypeLabel = new QLabel(tr("&Draft Type:"));
    QLabel* chairCountLabel = new QLabel(tr("# of &Chairs:"));
    QLabel* botCountLabel = new QLabel(tr("# of &Bots:"));

    mRoomNameLineEdit = new QLineEdit();
    connect( mRoomNameLineEdit, &QLineEdit::textChanged, this, &CreateRoomDialog::tryEnableCreateButton );

    mPasswordLineEdit = new QLineEdit();

    mDraftTypeComboBox = new QComboBox();

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

    mDraftConfigStack = new QStackedWidget();
    mDraftTypeComboBox->addItem( "Booster Draft" );
    mDraftTypeComboBox->addItem( "Sealed Deck" );

    roomNameLabel->setBuddy( mRoomNameLineEdit );
    passwordLabel->setBuddy( mPasswordLineEdit );
    draftTypeLabel->setBuddy( mDraftTypeComboBox );
    chairCountLabel->setBuddy( mChairCountComboBox );
    botCountLabel->setBuddy( mBotCountComboBox );

    constructBoosterStackedWidget();
    constructSealedStackedWidget();

    // Current widget expands to fill the dialog area as needed.
    mDraftConfigStack->currentWidget()->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );

    connect( mDraftTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(handleDraftTypeComboBoxIndexChanged(int)) );

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
    mainLayout->addWidget( draftTypeLabel,         2, 0 );
    mainLayout->addWidget( mDraftTypeComboBox,     2, 1, Qt::AlignLeft );
    mainLayout->addWidget( chairCountLabel,        3, 0 );
    mainLayout->addWidget( mChairCountComboBox,    3, 1, Qt::AlignLeft );
    mainLayout->addWidget( botCountLabel,          4, 0 );
    mainLayout->addWidget( mBotCountComboBox,      4, 1, Qt::AlignLeft );
    mainLayout->addWidget( mDraftConfigStack,      5, 0, 1, 2 );
    mainLayout->addWidget( buttonBox,              6, 0, 1, 2 );
    mainLayout->setColumnStretch( 1, 1 );
    setLayout(mainLayout);

    setWindowTitle( tr("Create Room") );

    tryEnableCreateButton();
}


void
CreateRoomDialog::constructBoosterStackedWidget()
{
    QGridLayout* boosterConfigLayout = new QGridLayout();

    mBoosterPackComboBoxes.resize( 3 );
    for( int i = 0; i < mBoosterPackComboBoxes.size(); ++i )
    {
        QLabel* roundLabel = new QLabel( "Round " + QString::number(i+1) + ": " );

        mBoosterPackComboBoxes[i] = new QComboBox();

        // This makes sure the comboboxes always have enough size to show their text.
        mBoosterPackComboBoxes[i]->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

        boosterConfigLayout->addWidget( roundLabel,                i, 0 );
        boosterConfigLayout->addWidget( mBoosterPackComboBoxes[i], i, 1 );
    }

    QLabel* selectionTimeLabel = new QLabel(tr("Selection Timer:"));

    mSelectionTimeCheckBox = new QCheckBox( "Enabled" );
    mSelectionTimeCheckBox->setChecked( true );
    connect( mSelectionTimeCheckBox, &QCheckBox::toggled, this, &CreateRoomDialog::handleSelectionTimeCheckBoxToggled );

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

    selectionTimeLabel->setBuddy( mSelectionTimeComboBox );

    QHBoxLayout* selectionTimeLayout = new QHBoxLayout;
    selectionTimeLayout->addWidget( mSelectionTimeCheckBox );
    selectionTimeLayout->addWidget( mSelectionTimeComboBox );

    boosterConfigLayout->addWidget( selectionTimeLabel,     3, 0 );
    boosterConfigLayout->addLayout( selectionTimeLayout,    3, 1, Qt::AlignLeft );

    // Add a fake row that stretches to take up extra vertical space.
    boosterConfigLayout->setRowStretch( 5, 1 );

    QGroupBox* boosterConfigGroupBox = new QGroupBox( "Booster Draft Configuration" );
    boosterConfigGroupBox->setLayout( boosterConfigLayout );

    // This default makes the dialog resizable to the current stack widget.
    boosterConfigGroupBox->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    mDraftConfigStack->addWidget( boosterConfigGroupBox );
}


void
CreateRoomDialog::constructSealedStackedWidget()
{
    QGridLayout* sealedConfigLayout = new QGridLayout();

    mSealedPackComboBoxes.resize( 6 );
    for( int i = 0; i < mSealedPackComboBoxes.size(); ++i )
    {
        mSealedPackComboBoxes[i] = new QComboBox();

        // This makes sure the comboboxes always have enough size to show their text.
        mSealedPackComboBoxes[i]->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

        QLabel* boosterLabel = new QLabel( "Booster " + QString::number(i+1) + ": " );
        sealedConfigLayout->addWidget( boosterLabel,             i+1, 0 );
        sealedConfigLayout->addWidget( mSealedPackComboBoxes[i], i+1, 1 );
    }

    QGroupBox* sealedConfigGroupBox = new QGroupBox( "Sealed Deck Configuration" );
    sealedConfigGroupBox->setLayout( sealedConfigLayout );

    // This default makes the dialog resizable to the current stack widget.
    sealedConfigGroupBox->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    mDraftConfigStack->addWidget( sealedConfigGroupBox );
}


void
CreateRoomDialog::setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets )
{
    QVector<QComboBox*> allComboBoxes;
    allComboBoxes += mBoosterPackComboBoxes;
    allComboBoxes += mSealedPackComboBoxes;

    for( auto comboBox : allComboBoxes )
    {
        comboBox->clear();
    }

    for( auto set : sets )
    {
        if( set.boosterGen )
        {
            QString code = QString::fromStdString( set.code );
            QString name = QString::fromStdString( set.name );
            for( auto comboBox : allComboBoxes )
            {
                comboBox->addItem( QString( "%1 - %2" ).arg( code ).arg( name ), code );
            }
        }
    }
}


CreateRoomDialog::DraftType
CreateRoomDialog::getDraftType() const
{
    if( mDraftTypeComboBox->currentIndex() == 1 ) return DRAFT_SEALED;
    return DRAFT_BOOSTER;
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
    QVector<QComboBox*> comboBoxes;
    switch( getDraftType() )
    {
        case DRAFT_BOOSTER: comboBoxes = mBoosterPackComboBoxes; break;
        case DRAFT_SEALED:  comboBoxes = mSealedPackComboBoxes; break;
        default:            break;
    }

    QStringList setCodes;
    for( auto comboBox : comboBoxes )
    {
        setCodes.append( comboBox->itemData( comboBox->currentIndex() ).toString() );
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


void
CreateRoomDialog::resizeEvent( QResizeEvent* event )
{
    QDialog::resizeEvent( event );

    // When resized, recenter the dialog window.
    QPoint offset( frameGeometry().width() / 2, frameGeometry().height() / 2 );
    move( mCenter - offset );
}


void
CreateRoomDialog::showEvent(QShowEvent * event)
{
    QDialog::showEvent( event );

    // When shown, recenter the dialog on the parent if available.
    if( parentWidget() != nullptr )
    {
        mCenter = parentWidget()->frameGeometry().center();
    }
    else
    {
        // No parent, use the desktop center.
        mCenter = QApplication::desktop()->availableGeometry().center();
    }
}


void
CreateRoomDialog::tryEnableCreateButton()
{
    mCreateButton->setEnabled( !mRoomNameLineEdit->text().isEmpty() );
}


void
CreateRoomDialog::handleDraftTypeComboBoxIndexChanged( int index )
{
    // Retain our current center, will be used when size is adjusted.
    mCenter = frameGeometry().center();

    // Making all other stack widgets size policies "Ignored" and the
    // current widget "Preferred" allows resize to fit to current stack
    // widget.  (see https://wiki.qt.io/Qt_project_org_faq)
    if( mDraftConfigStack->currentWidget() != nullptr )
    {
        mDraftConfigStack->currentWidget()->setSizePolicy(
                QSizePolicy::Ignored, QSizePolicy::Ignored );
    }

    mDraftConfigStack->setCurrentIndex( index );
    mDraftConfigStack->currentWidget()->setSizePolicy(
            QSizePolicy::Preferred, QSizePolicy::Preferred );

    adjustSize();
}


void
CreateRoomDialog::handleSelectionTimeCheckBoxToggled( bool checked )
{
    mSelectionTimeComboBox->setEnabled( checked );
}
