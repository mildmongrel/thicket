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

#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QTableWidget>
#include <QHeaderView>
#include "qtutils_core.h"

const QString CreateRoomDialog::CUBE_SET_CODE = QString( "***" );

CreateRoomDialog::CreateRoomDialog( const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : QDialog( parent ),
    mLogger( loggingConfig.createLogger() )
{
    QLabel* roomNameLabel = new QLabel(tr("Room &Name:"));
    QLabel* passwordLabel = new QLabel(tr("&Password:"));
    QLabel* draftTypeLabel = new QLabel(tr("&Draft Type:"));
    QLabel* cubeListLabel = new QLabel(tr("&Custom Cube List:"));

    mRoomNameLineEdit = new QLineEdit();
    connect( mRoomNameLineEdit, &QLineEdit::textChanged, this, &CreateRoomDialog::tryEnableCreateButton );

    mPasswordLineEdit = new QLineEdit();

    mDraftTypeComboBox = new QComboBox();
    mDraftTypeComboBox->addItem( "Booster Draft" );
    mDraftTypeComboBox->addItem( "Sealed Deck" );

    mImportCubeListNameLabel = new QLabel( "none" );
    QPushButton* importCubeListButton = new QPushButton( "Import..." );
    connect( importCubeListButton, &QPushButton::clicked, this, &CreateRoomDialog::handleImportCubeListButton );

    QHBoxLayout* cubeListLayout = new QHBoxLayout;
    cubeListLayout->addWidget( mImportCubeListNameLabel );
    cubeListLayout->addStretch();
    cubeListLayout->addWidget( importCubeListButton );

    mDraftConfigStack = new QStackedWidget();

    roomNameLabel->setBuddy( mRoomNameLineEdit );
    passwordLabel->setBuddy( mPasswordLineEdit );
    draftTypeLabel->setBuddy( mDraftTypeComboBox );
    cubeListLabel->setBuddy( importCubeListButton );

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
    mainLayout->addWidget( cubeListLabel,          3, 0 );
    mainLayout->addLayout( cubeListLayout,         3, 1, Qt::AlignLeft );
    mainLayout->addWidget( mDraftConfigStack,      4, 0, 1, 2 );
    mainLayout->addWidget( buttonBox,              5, 0, 1, 2 );
    mainLayout->setColumnStretch( 1, 1 );
    setLayout(mainLayout);

    setWindowTitle( tr("Create Room") );

    tryEnableCreateButton();
}


void
CreateRoomDialog::constructBoosterStackedWidget()
{
    QGroupBox* boosterConfigGroupBox = new QGroupBox( "Booster Draft Configuration" );
    QGridLayout* boosterConfigLayout = new QGridLayout( boosterConfigGroupBox );
    int row = 0;

    QLabel* chairCountLabel = new QLabel(tr("# of &Chairs:"));
    QLabel* botCountLabel = new QLabel(tr("# of &Bots:"));

    mBoosterChairCountComboBox = new QComboBox();
    for( int i = 2; i <= 8; ++i )
    {
        mBoosterChairCountComboBox->addItem( QString::number( i ) );
    }
    mBoosterChairCountComboBox->setCurrentIndex( 6 ); // 8 chairs

    mBoosterBotCountComboBox = new QComboBox();
    for( int i = 0; i <= 7; ++i )
    {
        mBoosterBotCountComboBox->addItem( QString::number( i ) );
    }

    chairCountLabel->setBuddy( mBoosterChairCountComboBox );
    botCountLabel->setBuddy( mBoosterBotCountComboBox );

    boosterConfigLayout->addWidget( chairCountLabel,            row,   0 );
    boosterConfigLayout->addWidget( mBoosterChairCountComboBox, row++, 1, Qt::AlignLeft );
    boosterConfigLayout->addWidget( botCountLabel,              row,   0 );
    boosterConfigLayout->addWidget( mBoosterBotCountComboBox,   row++, 1, Qt::AlignLeft );

    mBoosterPackComboBoxes.resize( 3 );
    for( int i = 0; i < mBoosterPackComboBoxes.size(); ++i )
    {
        QLabel* roundLabel = new QLabel( "Round " + QString::number(i+1) + ": " );

        mBoosterPackComboBoxes[i] = new QComboBox();

        // This makes sure the comboboxes always have enough size to show their text.
        mBoosterPackComboBoxes[i]->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

        boosterConfigLayout->addWidget( roundLabel,                row,   0 );
        boosterConfigLayout->addWidget( mBoosterPackComboBoxes[i], row++, 1 );
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

    boosterConfigLayout->addWidget( selectionTimeLabel,     row,   0 );
    boosterConfigLayout->addLayout( selectionTimeLayout,    row++, 1, Qt::AlignLeft );

    // Add a fake row that stretches to take up extra vertical space.
    boosterConfigLayout->setRowStretch( 5, 1 );

    // This default makes the dialog resizable to the current stack widget.
    boosterConfigGroupBox->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );

    mDraftConfigStack->addWidget( boosterConfigGroupBox );
}


void
CreateRoomDialog::constructSealedStackedWidget()
{
    QGroupBox* sealedConfigGroupBox = new QGroupBox( "Sealed Deck Configuration" );
    QGridLayout* sealedConfigLayout = new QGridLayout( sealedConfigGroupBox );
    int row = 0;

    QLabel* chairCountLabel = new QLabel(tr("# of &Chairs:"));

    mSealedChairCountComboBox = new QComboBox();
    for( int i = 1; i <= 8; ++i )
    {
        mSealedChairCountComboBox->addItem( QString::number( i ) );
    }
    mSealedChairCountComboBox->setCurrentIndex( 7 ); // 8 chairs

    chairCountLabel->setBuddy( mSealedChairCountComboBox );

    sealedConfigLayout->addWidget( chairCountLabel,           row,   0 );
    sealedConfigLayout->addWidget( mSealedChairCountComboBox, row++, 1, Qt::AlignLeft );

    mSealedPackComboBoxes.resize( 6 );
    for( int i = 0; i < mSealedPackComboBoxes.size(); ++i )
    {
        mSealedPackComboBoxes[i] = new QComboBox();

        // This makes sure the comboboxes always have enough size to show their text.
        mSealedPackComboBoxes[i]->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );

        QLabel* label = new QLabel( "Booster " + QString::number(i+1) + ": " );
        sealedConfigLayout->addWidget( label,                    row,   0 );
        sealedConfigLayout->addWidget( mSealedPackComboBoxes[i], row++, 1 );
    }

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
    switch( getDraftType() )
    {
        case DRAFT_BOOSTER: return mBoosterChairCountComboBox->currentText().toInt();
        case DRAFT_SEALED:  return mSealedChairCountComboBox->currentText().toInt();
        default:            return 0;
    }
}


int
CreateRoomDialog::getBotCount() const
{
    switch( getDraftType() )
    {
        case DRAFT_BOOSTER: return mBoosterBotCountComboBox->currentText().toInt();
        case DRAFT_SEALED:  return 0;  // no bots in sealed
        default:            return 0;
    }
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


void
CreateRoomDialog::handleImportCubeListButton()
{
    // Create an open file dialog.  Done explicitly rather than via the
    // QFileDialog static APIs to force a non-native dialog for windows.
    // Windows' native dialog halts the app event loop which causes
    // problems, most importantly pausing the QTimer sending a keep-
    // alive message to the server to keep the server from disconnecting us.
    QFileDialog dialog( this, tr("Open Cube Decklist File"), QString(), tr("Deck Files (*.dec *.mwdeck);;All Files (*)") );
    dialog.setOptions( QFileDialog::DontUseNativeDialog );
    dialog.setFileMode( QFileDialog::ExistingFile );

    int result = dialog.exec();

    if( result == QDialog::Rejected ) return;
    if( dialog.selectedFiles().empty() ) return;

    QString filename = dialog.selectedFiles().at(0);
    mLogger->debug( "loading cube file: {}", filename );

    // Turn file into string.
    QFile file( filename );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QMessageBox::warning( this, tr("Cube Decklist Import Error"),
                tr("Missing or invalid cube decklist file: %1").arg( filename ) );
        mLogger->notice( "unable to open cube list file: {}", filename );
        return;
    }
    QTextStream in( &file );
    QString deckStr;    
    deckStr = in.readAll();
    file.close();

    mCubeDecklist.clear();
    Decklist::ParseResult pr = mCubeDecklist.parse( deckStr.toStdString() );

    // Create a dialog to show import results.
    QDialog* dlg = new QDialog( this );
    dlg->setWindowTitle( tr("Cube Import") );
    QVBoxLayout* dlgLayout = new QVBoxLayout( dlg );

    QDialogButtonBox* dlgButtonBox = new QDialogButtonBox( QDialogButtonBox::Ok );
    connect( dlgButtonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept );

    const int totalQty = mCubeDecklist.getTotalQuantity( Decklist::ZONE_MAIN );
    QLabel* dlgTotalLabel = new QLabel( tr("Imported <b>%1</b> total cards.").arg( totalQty ) );
    dlgLayout->addWidget( dlgTotalLabel );

    QLineEdit * cubeNameLineEdit = nullptr;

    if( totalQty > 0 )
    {
        auto cards = mCubeDecklist.getCards( Decklist::ZONE_MAIN );

        QTableWidget* cardTable = new QTableWidget( cards.size(), 3, dlg );
        cardTable->setSelectionMode( QAbstractItemView::NoSelection );
        QStringList hdrLabels;
        hdrLabels << tr("Qty") << tr("Set") << tr("Name");
        cardTable->setHorizontalHeaderLabels( hdrLabels );
        cardTable->setShowGrid( false );
        cardTable->verticalHeader()->setVisible( false );
        cardTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
        cardTable->horizontalHeader()->setStretchLastSection( true );

        for( unsigned int i = 0; i < cards.size(); ++i )
        {
            cardTable->setItem( i, 0, new QTableWidgetItem(
                    QString::number( mCubeDecklist.getCardQuantity( cards[i], Decklist::ZONE_MAIN ) ) ) );
            cardTable->setItem( i, 1, new QTableWidgetItem( QString::fromStdString( cards[i].getSetCode() ) ) );
            cardTable->setItem( i, 2, new QTableWidgetItem( QString::fromStdString( cards[i].getName()  ) ) );
        }
        dlgLayout->addWidget( cardTable );

        // Add a strut 1/4 the width of the screen to make space for the card table.
        QRect rect = QApplication::desktop()->screenGeometry();
        dlgLayout->addStrut( rect.width() / 4 );

        QHBoxLayout* cubeNameLayout = new QHBoxLayout();
        QLabel* cubeNameLabel = new QLabel(tr("Cube Name:"));
        cubeNameLineEdit = new QLineEdit( tr("Custom Cube") );
        cubeNameLayout->addWidget( cubeNameLabel );
        cubeNameLayout->addWidget( cubeNameLineEdit );
        dlgLayout->addLayout( cubeNameLayout );
    }

    if( pr.hasErrors() )
    {
        dlgTotalLabel->setText( dlgTotalLabel->text() + tr("  Errors were detected.") );

        QPushButton* showErrorsButton = new QPushButton( tr("Show Errors...") );
        dlgButtonBox->addButton( showErrorsButton, QDialogButtonBox::ActionRole );
        connect( showErrorsButton, &QPushButton::clicked, this,
                [&pr,dlg]() {
                    // This is a subdialog to show errors during cube import.
                    QDialog* errorDlg = new QDialog( dlg );
                    errorDlg->setWindowTitle( tr("Cube Import Errors") );
                    QVBoxLayout* errorDlgLayout = new QVBoxLayout( errorDlg );

                    QTableWidget* errorTable = new QTableWidget( pr.errorCount(), 3, errorDlg );
                    errorTable->setSelectionMode( QAbstractItemView::NoSelection );
                    QStringList hdrLabels;
                    hdrLabels << tr("Line #") << tr("Error Message") << tr("Line Text");
                    errorTable->setHorizontalHeaderLabels( hdrLabels );
                    errorTable->setShowGrid( false );
                    errorTable->verticalHeader()->setVisible( false );
                    errorTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
                    errorTable->horizontalHeader()->setStretchLastSection( true );

                    unsigned int row = 0;
                    while( row < pr.errors.size() )
                    {
                        errorTable->setItem( row, 0, new QTableWidgetItem( QString::number( pr.errors[row].lineNum ) ) );
                        errorTable->setItem( row, 1, new QTableWidgetItem( QString::fromStdString( pr.errors[row].message ) ) );
                        errorTable->setItem( row, 2, new QTableWidgetItem( QString::fromStdString( pr.errors[row].line  ) ) );
                        row++;
                    }
                    errorDlgLayout->addWidget( errorTable );

                    // Add a strut 1/3 the width of the screen to make space for the error table.
                    QRect rect = QApplication::desktop()->screenGeometry();
                    errorDlgLayout->addStrut( rect.width() / 3 );

                    QDialogButtonBox* errorDlgButtonBox = new QDialogButtonBox( QDialogButtonBox::Ok );
                    connect( errorDlgButtonBox, &QDialogButtonBox::accepted, errorDlg, &QDialog::accept );

                    errorDlgLayout->addWidget( errorDlgButtonBox );

                    errorDlg->exec();
                    errorDlg->deleteLater();
                } );
    }

    dlgLayout->addWidget( dlgButtonBox );

    // Execute the dialog.
    dlg->exec();

    // Set the cube name if present.
    if( cubeNameLineEdit )
    {
        mCubeName = cubeNameLineEdit->text();
        mImportCubeListNameLabel->setText( mCubeName );
    }

    // Clean up the dialog.
    dlg->deleteLater();

    // When a new list is imported, add items to all comboboxes and switch
    // to them if this is the first time, otherwise just change names.
    QVector<QComboBox*> allComboBoxes;
    allComboBoxes += mBoosterPackComboBoxes;
    allComboBoxes += mSealedPackComboBoxes;
    for( auto comboBox : allComboBoxes )
    {
        if( comboBox->itemData( 0 ) != CUBE_SET_CODE )
        {
            comboBox->insertItem( 0, mCubeName, CUBE_SET_CODE );
            comboBox->setCurrentIndex( 0 );
        }
        else
        {
            comboBox->setItemText( 0, mCubeName );
        }
    }
}
