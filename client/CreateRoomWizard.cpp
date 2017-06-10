#include "CreateRoomWizard.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QStringBuilder>
#include <QTableWidget>
#include <QTextStream>
#include <QTextBrowser>
#include <QToolButton>
#include <QWizardPage>

#include "messages.pb.h"

#include "qtutils_core.h"
#include "qtutils_widget.h"

const QString CreateRoomWizard::CUBE_SET_CODE = QString( "***" );

CreateRoomWizard::CreateRoomWizard( const Logging::Config& loggingConfig,
                                    QWidget*               parent )
  : QWizard( parent ),
    mCubeLoaded( false ),
    mLogger( loggingConfig.createLogger() )
{
    setWindowTitle( tr("Create Draft Room") );

    mTypePage = new CreateRoomTypeWizardPage( mLogger );
    mConfigPage = new CreateRoomConfigWizardPage( mTypePage, mLogger );
    mCubePage = new CreateRoomCubeWizardPage( mLogger );
    mPacksPage = new CreateRoomPacksWizardPage( mTypePage, mLogger );
    mSummaryPage = new CreateRoomSummaryWizardPage( mTypePage, mConfigPage, mCubePage, mPacksPage, mLogger );

    setPage( PAGE_TYPE, mTypePage );
    setPage( PAGE_CONFIG, mConfigPage );
    setPage( PAGE_CUBE, mCubePage );
    setPage( PAGE_PACKS, mPacksPage );
    setPage( PAGE_SUMMARY, mSummaryPage );
}


void
CreateRoomWizard::setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets )
{
    mPacksPage->setRoomCapabilitySets( sets );
}


bool
CreateRoomWizard::fillRoomConfig( proto::RoomConfig* roomConfig ) const
{
    const CreateRoomWizard::DraftType draftType = mTypePage->getDraftType();
    const int selectionTime = mConfigPage->getSelectionTime();
    const int chairCount = mConfigPage->getChairCount();

    roomConfig->set_name( mConfigPage->getName().toStdString() );
    roomConfig->set_password_protected( !mConfigPage->getPassword().isEmpty() );
    roomConfig->set_bot_count( mConfigPage->getBotCount() );

    proto::DraftConfig* draftConfig = roomConfig->mutable_draft_config();
    draftConfig->set_chair_count( mConfigPage->getChairCount() );

    // If this is a cube-based draft, need to init the custom card list.
    if( mTypePage->isCube() )
    {
        proto::DraftConfig::CustomCardList* ccl = draftConfig->add_custom_card_lists();
        ccl->set_name( mCubePage->getCubeName().toStdString() );
        proto::DraftConfig::CustomCardList::CardQuantity* cardQty;

        const Decklist& cubeDecklist = mCubePage->getCubeDecklist();
        auto cards = cubeDecklist.getCards( Decklist::ZONE_MAIN );
        for( auto c : cards )
        {
            const unsigned int qty = cubeDecklist.getCardQuantity( c, Decklist::ZONE_MAIN );
            mLogger->debug( "custom list: adding {} of {}", qty, c.getName() );
            cardQty = ccl->add_card_quantities();
            cardQty->set_name( c.getName() );
            cardQty->set_set_code( c.getSetCode() );
            cardQty->set_quantity( qty );
        }
    }


    // Map to track booster set codes to dispenser indices.
    QMap<QString,unsigned int> setCodeToDispIdxMap;

    // Set up dispensers.
    if( mTypePage->isCube() )
    {
        proto::DraftConfig::CardDispenser* dispenser = draftConfig->add_dispensers();
        dispenser->set_source_custom_card_list_index( 0 );
    }
    else
    {
        unsigned int dispIdx = 0;
        for( auto setCode : mPacksPage->getPackSetCodes() )
        {
            if( !setCodeToDispIdxMap.contains( setCode ) )
            {
                proto::DraftConfig::CardDispenser* dispenser = draftConfig->add_dispensers();
                dispenser->add_source_booster_set_codes( setCode.toStdString() );
                setCodeToDispIdxMap[setCode] = dispIdx++;
            }
        }
        // Sanity check to make sure at least one dispenser was added.
        if( dispIdx == 0 )
        {
            mLogger->warn( "fill room config failed! (dispIdx == 0)" );
            return false;
        }
    }

    // Common functionality for initializing cube dispensations.
    auto fillCubeDispensationFn = [this,chairCount]( proto::DraftConfig::CardDispensation* disp ) {

                disp->set_dispenser_index( 0 );

                // Add all chairs to the dispensation.
                for( int i = 0; i < chairCount; ++i )
                {
                    disp->add_chair_indices( i );
                }

                // This is a cube round, so must specify number of cards to dispense.
                disp->set_quantity( 15 );
            };

    // Common functionality for initializing booster dispensations.
    auto fillBoosterDispensationFn = [this,&setCodeToDispIdxMap,chairCount]( proto::DraftConfig::CardDispensation* disp, const QString& setCode ) {

                // Ensure the set code has a dispenser index, then set the dispensation.
                if( !setCodeToDispIdxMap.contains( setCode ) )
                {
                    mLogger->warn( "fill room config failed! (no dispenser set code)" );
                    return;
                }
                const unsigned int dispIdx = setCodeToDispIdxMap[setCode];
                disp->set_dispenser_index( dispIdx );

                // Add all chairs to the dispensation.
                for( int i = 0; i < chairCount; ++i )
                {
                    disp->add_chair_indices( i );
                }

                // This is a booster dispensation - dispense all cards.
                disp->set_dispense_all( true );
            };

    if( draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER )
    {
        // Currently this is hardcoded for three booster rounds.
        for( int i = 0; i < 3; ++i )
        {
            proto::DraftConfig::Round* round = draftConfig->add_rounds();
            proto::DraftConfig::BoosterRound* boosterRound = round->mutable_booster_round();
            boosterRound->set_selection_time( selectionTime );
            boosterRound->set_pass_direction( (i%2) == 0 ? proto::DraftConfig::DIRECTION_CLOCKWISE :
                                                           proto::DraftConfig::DIRECTION_COUNTER_CLOCKWISE );

            proto::DraftConfig::CardDispensation* dispensation = boosterRound->add_dispensations();
            if( mTypePage->isCube() )
            {
                fillCubeDispensationFn( dispensation );
            }
            else
            {
                fillBoosterDispensationFn( dispensation, mPacksPage->getPackSetCodes().value( i ) );
            }
        }
    }
    else if( draftType == CreateRoomWizard::DRAFT_TYPE_SEALED )
    {
        proto::DraftConfig::Round* round = draftConfig->add_rounds();
        proto::DraftConfig::SealedRound* sealedRound = round->mutable_sealed_round();

        // Currently hardcoded for 6 boosters.
        for( int i = 0; i < 6; ++i )
        {
            proto::DraftConfig::CardDispensation* dispensation = sealedRound->add_dispensations();
            if( mTypePage->isCube() )
            {
                fillCubeDispensationFn( dispensation );
            }
            else
            {
                fillBoosterDispensationFn( dispensation, mPacksPage->getPackSetCodes().value( i ) );
            }
        }
    }
    else if( draftType == CreateRoomWizard::DRAFT_TYPE_GRID )
    {
        // Currently this is hardcoded for 18 grid rounds, using one dispenser.
        for( int i = 0; i < 18; ++i )
        {
            proto::DraftConfig::Round* round = draftConfig->add_rounds();
            proto::DraftConfig::GridRound* gridRound = round->mutable_grid_round();
            gridRound->set_selection_time( selectionTime );
            gridRound->set_initial_chair( i%2 );
            gridRound->set_dispenser_index( 0 );

            // Set a short post-round timer to review opponent selections.
            round->set_post_round_timer( 5 );
        }
    }
    else
    {
        mLogger->debug( "fill room config failed (invalid draft type)" );
        return false;
    }

    return true;
}


QString
CreateRoomWizard::getPassword() const
{
    return mConfigPage->getPassword();
}


int
CreateRoomWizard::nextId() const
{
    switch( currentId() )
    {
    case PAGE_TYPE:
        return PAGE_CONFIG;
    case PAGE_CONFIG:
        return mTypePage->isCube() ? PAGE_CUBE : PAGE_PACKS;
    case PAGE_CUBE:
    case PAGE_PACKS:
        return PAGE_SUMMARY;
    default:
        return -1;
    }
}


/**********************************************************************
                      CreateRoomTypeWizardPage
**********************************************************************/


CreateRoomTypeWizardPage::CreateRoomTypeWizardPage( std::shared_ptr<spdlog::logger>& logger,
                                                    QWidget*                         parent )
  : QWizardPage( parent ),
    mType( CreateRoomWizard::DRAFT_TYPE_BOOSTER ),
    mIsCube( false ),
    mLogger( logger )
{
    setTitle( tr("Choose Draft Type") );
    setSubTitle( tr("Start by choosing the draft type.") );

    setStyleSheet( "QRadioButton { font: bold; }" );

    mBoosterButton = new QRadioButton( tr("Booster Draft") );
    connect( mBoosterButton, &QRadioButton::toggled, [this](bool toggled) {
            mLogger->debug( "booster toggled={}", toggled );
        });
    QLabel* boosterLabel = new QLabel( tr("For three rounds, players choose a card from a pack, pass the pack to an adjacent player (alternating direction in each round), and repeat until the entire pack is selected.  (2-8 players)") );
    boosterLabel->setWordWrap( true );
    mSealedButton = new QRadioButton( tr("Sealed Deck") );
    QLabel* sealedLabel = new QLabel( tr("Players build decks from a set of booster packs.  (2-8 players)") );
    sealedLabel->setWordWrap( true );
    mCubeBoosterButton = new QRadioButton( tr("Cube Booster Draft") );
    QLabel* cubeBoosterLabel = new QLabel( tr("Booster Draft using packs randomly built from a 'cube' list.  (2-8 players)") );
    cubeBoosterLabel->setWordWrap( true );
    mCubeSealedButton = new QRadioButton( tr("Cube Sealed Deck") );
    QLabel* cubeSealedLabel = new QLabel( tr("Sealed Deck using packs randomly built from a 'cube' list.  (2-8 players)") );
    cubeSealedLabel->setWordWrap( true );
    mGridButton = new QRadioButton( tr("Grid Draft") );
    QLabel* gridLabel = new QLabel( tr("Each round, random cards from a 'cube' list are laid out in a 9x9 grid.  One player chooses a single row or column, followed by the next player.  (2 players)") );
    gridLabel->setWordWrap( true );

    mBoosterButton->setChecked( true );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget( mBoosterButton );
    layout->addWidget( boosterLabel );
    layout->addSpacing( 10 );
    layout->addWidget( mSealedButton );
    layout->addWidget( sealedLabel );
    layout->addSpacing( 10 );
    layout->addWidget( mCubeBoosterButton );
    layout->addWidget( cubeBoosterLabel );
    layout->addSpacing( 10 );
    layout->addWidget( mCubeSealedButton );
    layout->addWidget( cubeSealedLabel );
    layout->addSpacing( 10 );
    layout->addWidget( mGridButton );
    layout->addWidget( gridLabel );
    setLayout( layout );
}


CreateRoomWizard::DraftType
CreateRoomTypeWizardPage::getDraftType() const
{
    if( mBoosterButton->isChecked() || mCubeBoosterButton->isChecked() ) return CreateRoomWizard::DRAFT_TYPE_BOOSTER;
    if( mSealedButton->isChecked() || mCubeSealedButton->isChecked() ) return CreateRoomWizard::DRAFT_TYPE_SEALED;
    if( mGridButton->isChecked() ) return CreateRoomWizard::DRAFT_TYPE_GRID;

    mLogger->error( "Unexpected draft type!" );
    return CreateRoomWizard::DRAFT_TYPE_BOOSTER;
}


bool
CreateRoomTypeWizardPage::isCube() const
{
    return( mCubeBoosterButton->isChecked() || mCubeSealedButton->isChecked() || mGridButton->isChecked() );
}


/**********************************************************************
                     CreateRoomConfigWizardPage
**********************************************************************/


CreateRoomConfigWizardPage::CreateRoomConfigWizardPage( CreateRoomTypeWizardPage const * createRoomTypeWizardPage,
                                                        std::shared_ptr<spdlog::logger>& logger,
                                                        QWidget*                         parent )
  : QWizardPage( parent ),
    mCreateRoomTypeWizardPage( createRoomTypeWizardPage ),
    mLogger( logger )
{
    setTitle( tr("Configure the Room") );
    setSubTitle( tr("Choose settings for the room.") );

    mNameLabel = new QLabel( tr("&Name:") );
    mPasswordLabel = new QLabel( tr("&Password (optional):") );
    mChairCountLabel = new QLabel( tr("# of &Chairs:") );
    mBotCountLabel = new QLabel( tr("# of &Bots:") );
    mGridBotLabel = new QLabel( tr("Practice Bot:") );
    mSelectionTimeLabel = new QLabel( tr("Selection Timer:") );

    mNameLineEdit = new QLineEdit();
    connect( mNameLineEdit, &QLineEdit::textChanged, this, [this](const QString&) {
            emit completeChanged();
        } );
    mPasswordLineEdit = new QLineEdit();

    // Initialize with 1-8 players, but may be adjusted based on draft type.
    mChairCountComboBox = new QComboBox();
    for( int i = 1; i <= 8; ++i )
    {
        mChairCountComboBox->addItem( QString::number( i ) );
    }
    mChairCountComboBox->setCurrentIndex( 7 ); // 8 chairs

    mBotCountComboBox = new QComboBox();
    for( int i = 0; i <= 7; ++i )
    {
        mBotCountComboBox->addItem( QString::number( i ) );
    }

    // Checkbox for a grid bot (just for practicing, it's stupid)
    mGridBotCheckBox = new QCheckBox( "Enabled" );
    mGridBotCheckBox->setChecked( false );

    mSelectionTimeCheckBox = new QCheckBox( "Enabled" );
    mSelectionTimeCheckBox->setChecked( true );

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

    mNameLabel->setBuddy( mNameLineEdit );
    mPasswordLabel->setBuddy( mPasswordLineEdit );
    mChairCountLabel->setBuddy( mChairCountComboBox );
    mBotCountLabel->setBuddy( mBotCountComboBox );
    mGridBotLabel->setBuddy( mGridBotCheckBox );
    mSelectionTimeLabel->setBuddy( mSelectionTimeCheckBox );

    mLayout = new QGridLayout( this );
}


QString
CreateRoomConfigWizardPage::getName() const
{
    return mNameLineEdit->text();
}


QString
CreateRoomConfigWizardPage::getPassword() const
{
    return mPasswordLineEdit->text();
}


int
CreateRoomConfigWizardPage::getChairCount() const
{
    switch( mCreateRoomTypeWizardPage->getDraftType() )
    {
        case CreateRoomWizard::DRAFT_TYPE_BOOSTER:
        case CreateRoomWizard::DRAFT_TYPE_SEALED:
            return mChairCountComboBox->currentText().toInt();
        case CreateRoomWizard::DRAFT_TYPE_GRID:
            return 2;
        default:
            assert( false );
    }
}


int
CreateRoomConfigWizardPage::getBotCount() const
{
    switch( mCreateRoomTypeWizardPage->getDraftType() )
    {
        case CreateRoomWizard::DRAFT_TYPE_BOOSTER: return mBotCountComboBox->currentText().toInt();
        case CreateRoomWizard::DRAFT_TYPE_SEALED:  return 0;  // no bots in sealed
        case CreateRoomWizard::DRAFT_TYPE_GRID:    return mGridBotCheckBox->isChecked() ? 1 : 0;
        default:                                   assert( false );
    }
}


int
CreateRoomConfigWizardPage::getSelectionTime() const
{
    switch( mCreateRoomTypeWizardPage->getDraftType() )
    {
        case CreateRoomWizard::DRAFT_TYPE_BOOSTER:
        case CreateRoomWizard::DRAFT_TYPE_GRID:
          return mSelectionTimeCheckBox->isChecked() ? mSelectionTimeComboBox->currentText().toInt() : 0;
        default:
          return 0;
    }
}


void
CreateRoomConfigWizardPage::initializePage()
{
    static QSet<QWidget*> reusableWidgets = {
        mNameLabel,
        mNameLineEdit,
        mPasswordLabel,
        mPasswordLineEdit,
        mChairCountLabel,
        mChairCountComboBox,
        mBotCountLabel,
        mBotCountComboBox,
        mGridBotLabel,
        mGridBotCheckBox,
        mSelectionTimeLabel,
        mSelectionTimeCheckBox,
        mSelectionTimeComboBox
    };

    // Clear out the layout, saving the reusable widgets.
    qtutils::clearLayoutSaveWidgets( mLayout, reusableWidgets );

    // Hide reusable widgets - they will be explicitly shown at the end.
    for( QWidget* w : reusableWidgets ) w->hide();

    const CreateRoomWizard::DraftType draftType = mCreateRoomTypeWizardPage->getDraftType();

    int row = 0;

    mLayout->addWidget( mNameLabel,          row, 0 );
    mLayout->addWidget( mNameLineEdit,       row++, 1 );
    mLayout->addWidget( mPasswordLabel,      row, 0 );
    mLayout->addWidget( mPasswordLineEdit,   row++, 1 );

    // Add spacing row.
    mLayout->setRowMinimumHeight( row++, 20 );

    if( (draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER) ||
        (draftType == CreateRoomWizard::DRAFT_TYPE_SEALED) )
    {
        mLayout->addWidget( mChairCountLabel,    row, 0 );
        mLayout->addWidget( mChairCountComboBox, row++, 1, Qt::AlignLeft );

        // Add or remove 1-player option depending on booster/sealed
        const QString onePlayerStr = QString::number( 1 );
        int idx = mChairCountComboBox->findText( onePlayerStr );
        if( (draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER) && (idx >= 0) )
        {
            mChairCountComboBox->removeItem( idx );
        }
        else if( (draftType == CreateRoomWizard::DRAFT_TYPE_SEALED) && (idx < 0) )
        {
            mChairCountComboBox->insertItem( idx, onePlayerStr );
        }
    }

    if( draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER )
    {
        mLayout->addWidget( mBotCountLabel,      row, 0 );
        mLayout->addWidget( mBotCountComboBox,   row++, 1, Qt::AlignLeft );
    }

    if( draftType == CreateRoomWizard::DRAFT_TYPE_GRID )
    {
        mLayout->addWidget( mGridBotLabel,      row, 0 );
        mLayout->addWidget( mGridBotCheckBox,   row++, 1, Qt::AlignLeft );
    }

    // Add spacing row.
    mLayout->setRowMinimumHeight( row++, 20 );

    if( (draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER) ||
        (draftType == CreateRoomWizard::DRAFT_TYPE_GRID) )
    {
        QHBoxLayout* selectionTimeLayout = new QHBoxLayout;
        selectionTimeLayout->addWidget( mSelectionTimeCheckBox );
        selectionTimeLayout->addWidget( mSelectionTimeComboBox );
        mLayout->addWidget( mSelectionTimeLabel,    row, 0 );
        mLayout->addLayout( selectionTimeLayout,    row++, 1, Qt::AlignLeft );
    }

    // Show all the widgets that were added to the layout.
    qtutils::showWidgetsInLayout( mLayout );
}


bool
CreateRoomConfigWizardPage::isComplete() const
{
    return !mNameLineEdit->text().isEmpty();
}


/**********************************************************************
                      CreateRoomCubeWizardPage
**********************************************************************/


CreateRoomCubeWizardPage::CreateRoomCubeWizardPage( std::shared_ptr<spdlog::logger>& logger,
                                                    QWidget*                         parent )
  : QWizardPage( parent ),
    mLogger( logger )
{
    setTitle( tr("Import Cube") );
    setSubTitle( tr("Import the cube decklist.") );

    QVBoxLayout* layout = new QVBoxLayout( this );

    QPushButton* importButton = new QPushButton( "Import..." );
    connect( importButton, &QPushButton::clicked, this, &CreateRoomCubeWizardPage::handleImportButton );
    layout->addWidget( importButton );

    mCardCountLabel = new QLabel( tr("0 total cards") );
    layout->addWidget( mCardCountLabel );

    mCardTable = new QTableWidget( 0, 3 );
    mCardTable->setSelectionMode( QAbstractItemView::NoSelection );
    QStringList hdrLabels;
    hdrLabels << tr("Qty") << tr("Set") << tr("Name");
    mCardTable->setHorizontalHeaderLabels( hdrLabels );
    mCardTable->setShowGrid( false );
    mCardTable->verticalHeader()->setVisible( false );
    mCardTable->horizontalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    mCardTable->horizontalHeader()->setStretchLastSection( true );
    layout->addWidget( mCardTable );

    QHBoxLayout* cubeNameLayout = new QHBoxLayout();
    cubeNameLayout->addWidget( new QLabel( tr("Cube Name:") ) );
    mCubeNameLineEdit = new QLineEdit( tr("Draft Cube") );
    cubeNameLayout->addWidget( mCubeNameLineEdit );

    layout->addLayout( cubeNameLayout );
}


QString
CreateRoomCubeWizardPage::getCubeName() const
{
    return mCubeNameLineEdit->text();
}


void
CreateRoomCubeWizardPage::handleImportButton()
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

    Decklist decklist;

    Decklist::ParseResult pr = decklist.parse( deckStr.toStdString() );

    // Create a dialog to show import results.
    QDialog* dlg = new QDialog( this );
    dlg->setWindowTitle( tr("Cube Import") );
    QVBoxLayout* dlgLayout = new QVBoxLayout( dlg );

    QDialogButtonBox* dlgButtonBox = new QDialogButtonBox( QDialogButtonBox::Ok );
    connect( dlgButtonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept );

    const int totalQty = decklist.getTotalQuantity( Decklist::ZONE_MAIN );
    QLabel* dlgTotalLabel = new QLabel( tr("Imported <b>%1</b> total cards.").arg( totalQty ) );
    dlgLayout->addWidget( dlgTotalLabel );

    if( totalQty > 0 )
    {
        mCubeDecklist = decklist;

        auto cards = mCubeDecklist.getCards( Decklist::ZONE_MAIN );

        mCardTable->clearContents();
        mCardTable->setRowCount( cards.size() );

        for( unsigned int i = 0; i < cards.size(); ++i )
        {
            mCardTable->setItem( i, 0, new QTableWidgetItem(
                    QString::number( mCubeDecklist.getCardQuantity( cards[i], Decklist::ZONE_MAIN ) ) ) );
            mCardTable->setItem( i, 1, new QTableWidgetItem( QString::fromStdString( cards[i].getSetCode() ) ) );
            mCardTable->setItem( i, 2, new QTableWidgetItem( QString::fromStdString( cards[i].getName()  ) ) );
        }

        mCardCountLabel->setText( tr("<b>%1</b> total cards").arg( totalQty ) );
        mCubeNameLineEdit->setText( QFileInfo(filename).completeBaseName() );

        emit completeChanged();
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
    dlg->deleteLater();
}


bool
CreateRoomCubeWizardPage::isComplete() const
{
    return mCubeDecklist.getTotalQuantity( Decklist::ZONE_MAIN ) > 0;
}


/**********************************************************************
                      CreateRoomPacksWizardPage
**********************************************************************/


CreateRoomPacksWizardPage::CreateRoomPacksWizardPage( CreateRoomTypeWizardPage const * createRoomTypeWizardPage,
                                                      std::shared_ptr<spdlog::logger>& logger,
                                                      QWidget*                         parent )
  : QWizardPage( parent ),
    mCreateRoomTypeWizardPage( createRoomTypeWizardPage ),
    mLogger( logger )
{
    setTitle( tr("Choose Booster Packs") );
    setSubTitle( tr("Set booster packs for drafting.") );

    mLayout = new QVBoxLayout( this );
    mListWidget = new QListWidget();

    // Initialize set codes to empty.
    for( unsigned int i = 0; i < MAX_PACK_COUNT; ++i ) mPackSetCodes.append( QString() );
}


void
CreateRoomPacksWizardPage::setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets )
{
    mSetCodeToNameMap.clear();
    mListWidget->clear();

    for( auto set : sets )
    {
        if( set.boosterGen )
        {
            QString code = QString::fromStdString( set.code );
            QString name = QString::fromStdString( set.name );

            mSetCodeToNameMap.insert( code, name );

            QListWidgetItem* item = new QListWidgetItem( QString( "%1 - %2" ).arg( code ).arg( name ) );
            item->setData( Qt::UserRole, code );
            mListWidget->addItem( item );
        }
    }

    // Clear out any set codes that are now invalid, then update pack labels in case of changes.
    for( int i = 0; i < mPackSetCodes.size(); ++i )
    {
        if( !mSetCodeToNameMap.contains( mPackSetCodes[i] ) )
        {
            mPackSetCodes[i].clear();
        }
    }
    updatePackLabels();
}


QStringList
CreateRoomPacksWizardPage::getPackSetCodes() const
{
    return mPackSetCodes.mid( 0, getPackCount() );
}


void
CreateRoomPacksWizardPage::initializePage()
{
    static QSet<QWidget*> reusableWidgets = {
        mListWidget
    };

    // Clear out the layout, saving the reusable widgets.
    qtutils::clearLayoutSaveWidgets( mLayout, reusableWidgets );

    // Clear pack labels list, will be regenerated below.
    mPackLabels.clear();

    // Show all items in the list widget in case filtering happened in a previous session.
    for( int row = 0; row < mListWidget->count(); row++ )
    {
        mListWidget->item( row )->setHidden( false );
    }

    QHBoxLayout* controlLayout = new QHBoxLayout();
    controlLayout->addWidget( new QLabel( tr("Filter:") ) );
    QLineEdit* filterLineEdit = new QLineEdit();
    connect( filterLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
            QList<QListWidgetItem*> matches = mListWidget->findItems( text, Qt::MatchContains );
            for( int row = 0; row < mListWidget->count(); row++ ) {
                QListWidgetItem* item = mListWidget->item( row );
                item->setHidden( !matches.contains( item ) );
            }
        } );
    controlLayout->addWidget( filterLineEdit );
    controlLayout->addSpacing( 10 );

    QPushButton* setAllButton = new QPushButton( tr("Assign All") );
    connect( setAllButton, &QPushButton::clicked, this, [this]() {
            QString setStr = mListWidget->currentItem()->text();
            for( int idx = 0; idx < mPackLabels.size(); ++idx ) {
                const QString setCode = mListWidget->currentItem()->data( Qt::UserRole ).toString();
                mPackSetCodes[idx] = setCode;
            }
            mLogger->debug( "setcodes: {}", mPackSetCodes.join(',') );
            updatePackLabels();
            emit completeChanged();
        } );
    controlLayout->addWidget( setAllButton );

    QGridLayout* packLayout = new QGridLayout();

    const CreateRoomWizard::DraftType draftType = mCreateRoomTypeWizardPage->getDraftType();

    unsigned int packCount = getPackCount();
    mPackLabels.resize( packCount );

    for( unsigned int idx = 0; idx < packCount; ++idx )
    {
        QToolButton* button = new QToolButton();
        button->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
        button->setText( (draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER) ? tr("Assign Pack %1").arg(idx)
                                                                             : tr("Assign Pack") );
        button->setArrowType( Qt::DownArrow );
        button->setIconSize( button->iconSize() / 2 );
        mPackLabels[idx] = new QLabel();
        mPackLabels[idx]->setAlignment( Qt::AlignCenter );
        mPackLabels[idx]->setWordWrap( true );

        connect( button, &QToolButton::clicked, this, [this,idx]() {
                const QString setCode = mListWidget->currentItem()->data( Qt::UserRole ).toString();
                mPackSetCodes[idx] = setCode;
                mLogger->debug( "setcodes: {}", mPackSetCodes.join(',') );
                updatePackLabels();
                emit completeChanged();
            } );

        packLayout->addWidget( button,           (idx/3)*2,     idx%3, 1, 1, Qt::AlignCenter );
        packLayout->addWidget( mPackLabels[idx], (idx/3)*2 + 1, idx%3 );
    }
    updatePackLabels();

    mLayout->addWidget( mListWidget );
    mLayout->addLayout( controlLayout );
    mLayout->addSpacing( 10 );
    mLayout->addLayout( packLayout );
}


bool
CreateRoomPacksWizardPage::isComplete() const
{
    // Pack set codes must be set up to the number of pack labels
    for( int idx = 0; idx < mPackLabels.size(); ++idx )
    {
        if( mPackSetCodes[idx].isEmpty() ) return false;
    }
    return true;
}


void
CreateRoomPacksWizardPage::updatePackLabels()
{
    for( int idx = 0; idx < mPackSetCodes.size(); ++idx )
    {
        if( idx < mPackLabels.size() )
        {
            const QString setCode = mPackSetCodes[idx];
            if( mSetCodeToNameMap.contains( setCode ) )
            {
                const QString setName = mSetCodeToNameMap[setCode];
                mPackLabels[idx]->setText( QString("<b>%1 - %2</b>").arg(setCode).arg(setName) );
            }
            else
            {
                mPackLabels[idx]->setText( "<font color=\"red\">(not yet selected)</font>" );
            }
        }
    }
}


unsigned int
CreateRoomPacksWizardPage::getPackCount() const
{
    const CreateRoomWizard::DraftType draftType = mCreateRoomTypeWizardPage->getDraftType();
    return (draftType == CreateRoomWizard::DRAFT_TYPE_BOOSTER) ? BOOSTER_PACK_COUNT :
           (draftType == CreateRoomWizard::DRAFT_TYPE_SEALED)  ? SEALED_PACK_COUNT :
                                                                 0;
}


/**********************************************************************
                     CreateRoomSummaryWizardPage
**********************************************************************/


CreateRoomSummaryWizardPage::CreateRoomSummaryWizardPage( CreateRoomTypeWizardPage*        typePage,
                                                          CreateRoomConfigWizardPage*      configPage,
                                                          CreateRoomCubeWizardPage*        cubePage,
                                                          CreateRoomPacksWizardPage*       packsPage,
                                                          std::shared_ptr<spdlog::logger>& logger,
                                                          QWidget*                         parent )
  : QWizardPage( parent ),
    mTypePage( typePage ),
    mConfigPage( configPage ),
    mCubePage( cubePage ),
    mPacksPage( packsPage ),
    mLogger( logger )
{
    setTitle( tr("Review") );
    setSubTitle( tr("Review the room configuration.") );

    mTextBrowser = new QTextBrowser();
    //mTextBrowser->setStyleSheet( "background-color: gray;" );

    QVBoxLayout* layout = new QVBoxLayout( this );
    layout->addWidget( mTextBrowser );
}


void
CreateRoomSummaryWizardPage::initializePage()
{
    mTextBrowser->clear();

    const QString tabStr( "&nbsp;&nbsp;&nbsp;&nbsp;" );

    const int numChairs =  mConfigPage->getChairCount();
    const int numBots =  mConfigPage->getBotCount();
    const int numHumans = numChairs - numBots;
    const int selectionTime = mConfigPage->getSelectionTime();

    QString typeStr;
    bool showSelectionTime = true;
    switch( mTypePage->getDraftType() )
    {
    case CreateRoomWizard::DRAFT_TYPE_BOOSTER:
        typeStr = tr("Booster Draft");
        break;
    case CreateRoomWizard::DRAFT_TYPE_SEALED:
        typeStr = tr("Sealed Draft");
        showSelectionTime = false;
        break;
    case CreateRoomWizard::DRAFT_TYPE_GRID:
        typeStr = tr("Grid Draft");
        break;
    default:
        typeStr = tr("Custom");
        break;
    }

    QString chairConfigStr = QString::number( numChairs ) % tr(" chairs (") % QString::number( numHumans ) % ((numHumans > 1) ? tr(" humans") : tr(" human"));
    if( numBots > 0 )
    {
        chairConfigStr += ", " % QString::number( numBots ) % ((numBots > 1) ? tr(" bots") : tr(" bot"));
    }
    chairConfigStr += ")";

    QString packConfigStr;
    if( mTypePage->isCube() )
    {
        const int cubeQty = mCubePage->getCubeDecklist().getTotalQuantity( Decklist::ZONE_MAIN );
        packConfigStr = tr("Cube - ") % mCubePage->getCubeName() % " (" % QString::number( cubeQty ) % tr(" cards)");
    }
    else
    {
        QStringList packSetCodes = mPacksPage->getPackSetCodes();
        packConfigStr = tr("Boosters - ") % packSetCodes.join(", ");
    }

    QString selectionTimeStr = (selectionTime > 0) ? QString::number( selectionTime ) + tr(" seconds") : tr("unlimited");

    QString summaryStr;
    summaryStr = tr("Room name: <br>") % tabStr % "<b>" % mConfigPage->getName() % "</b><p>" %
                 tr("Password protected: <br>") % tabStr % "<b>" % (mConfigPage->getPassword().isEmpty()? tr("no") : tr("yes")) % "</b><p>" %
                 tr("Draft type: <br>") % tabStr % "<b>" % typeStr % "</b><p>" %
                 tr("Chair configuration: <br>") % tabStr % "<b>" % chairConfigStr % "</b><p>" %
                 tr("Cards: <br>") % tabStr % "<b>" % packConfigStr % "</b><p>" %
                 (showSelectionTime ? tr("Selection time: <br>") % tabStr % "<b>" % selectionTimeStr % "</b><p>"
                                    : QString() );
    mTextBrowser->append( summaryStr );
}
