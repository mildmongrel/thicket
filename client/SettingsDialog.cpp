#include "SettingsDialog.h"
#include "ClientSettings.h"
#include "SizedSvgWidget.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>


// Presets for basic land images.
static const std::map<QString,BasicLandMuidMap> sBasicLandMuidPresetsMap = {
    { "Beta",   { { BASIC_LAND_PLAINS,   595 },
                  { BASIC_LAND_ISLAND,   592 },
                  { BASIC_LAND_SWAMP,    573 },
                  { BASIC_LAND_MOUNTAIN, 589 },
                  { BASIC_LAND_FOREST,   586 } } },
    { "BFZ",    { { BASIC_LAND_PLAINS,   401985 },
                  { BASIC_LAND_ISLAND,   401918 },
                  { BASIC_LAND_SWAMP,    402053 },
                  { BASIC_LAND_MOUNTAIN, 401953 },
                  { BASIC_LAND_FOREST,   401882 } } } };

static const QString CUSTOM_PRESET_NAME( QT_TR_NOOP("Custom") );

static const std::map<BasicLandType,QString> sSvgResourceMap = {
    { BASIC_LAND_PLAINS,   ":/white-mana-symbol.svg" },
    { BASIC_LAND_ISLAND,   ":/blue-mana-symbol.svg"  },
    { BASIC_LAND_SWAMP,    ":/black-mana-symbol.svg" },
    { BASIC_LAND_MOUNTAIN, ":/red-mana-symbol.svg"   },
    { BASIC_LAND_FOREST,   ":/green-mana-symbol.svg" } };

static const QSize BASIC_LAND_MANA_SYMBOL_SIZE( 15,15 );

SettingsDialog::SettingsDialog( ClientSettings*        settings,
                                const Logging::Config& loggingConfig,
                                QWidget*               parent )
  : QDialog( parent ),
    mSettings( settings ),
    mBasicLandMuidsResetting( false ),
    mLogger( loggingConfig.createLogger() )
{
    setWindowTitle(tr("Settings"));

    QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    connect( buttonBox, SIGNAL(accepted()), this, SLOT(accept()) );
    connect( buttonBox, SIGNAL(rejected()), this, SLOT(reject()) );

    mTabWidget = new QTabWidget();

    mTabWidget->addTab( buildGeneralWidget(),      tr("General") );
    mTabWidget->addTab( buildFactoryResetWidget(), tr("Reset") );

    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->addWidget( mTabWidget, 0, 0 );
    mainLayout->addWidget( buttonBox, 1, 0 );

    connect( this, SIGNAL(accepted()), this, SLOT(updateChangedSettings()) );
}


int
SettingsDialog::exec()
{
    // Sync the settings.
    resetValues();

    // Show the first tab.
    mTabWidget->setCurrentIndex( 0 );

    return QDialog::exec();
}


void
SettingsDialog::resetValues()
{
    mLogger->trace( "resetting values" );

    int imageCacheMaxSizeMB = mSettings->getImageCacheMaxSize() / (1024 * 1024);
    mImageCacheMaxSizeSpinBox->setValue( imageCacheMaxSizeMB );

    // Fetch the current basic land muid settings and update the line edit widgets.
    mBasicLandMuidsResetting = true;
    BasicLandMuidMap settingsMuids;
    const BasicLandMuidMap muidMap = mSettings->getBasicLandMultiverseIds();
    for( auto basic : gBasicLandTypeArray )
    {
        const int muid = muidMap.getMuid( basic );
        mBasicLandMuidLineEditMap[basic]->setText( QString::number( muid ) );
        settingsMuids.setMuid( basic, muid );
    }
    mBasicLandMuidsResetting = false;

    // Set custom preset to current settings.
    mCustomBasicLandMuidPreset = settingsMuids;

    // Reset the combobox selection to custom unless a preset matches.
    mBasicLandMuidSetComboBox->setCurrentIndex( mBasicLandMuidSetComboBox->count() - 1 );
    for( auto& kv : sBasicLandMuidPresetsMap )
    {
        if( kv.second == settingsMuids )
        {
            int idx = mBasicLandMuidSetComboBox->findText( kv.first );
            if( idx >= 0 ) mBasicLandMuidSetComboBox->setCurrentIndex( idx );
        }
    }
}


void
SettingsDialog::updateChangedSettings()
{
    mLogger->trace( "updating changed settings" );

    int settingsVal = mSettings->getImageCacheMaxSize();
    int dialogVal   = mImageCacheMaxSizeSpinBox->value() * 1024 * 1024;
    if( dialogVal != settingsVal )
    {
        mSettings->setImageCacheMaxSize( dialogVal );
        mLogger->debug( "image cache max size updated" );
    }

    const BasicLandMuidMap settingsMuidMap = mSettings->getBasicLandMultiverseIds();
    BasicLandMuidMap dialogMuidMap;
    for( auto basic : gBasicLandTypeArray )
    {
        dialogMuidMap.setMuid( basic, mBasicLandMuidLineEditMap[basic]->text().toInt() );
    }
    if( dialogMuidMap != settingsMuidMap )
    {
        mLogger->debug( "basic land muids updated" );
        mSettings->setBasicLandMultiverseIds( dialogMuidMap );
    }
}


QWidget*
SettingsDialog::buildGeneralWidget()
{
    QWidget* widget = new QWidget();

    mImageCacheMaxSizeSpinBox = new QSpinBox();
    mImageCacheMaxSizeSpinBox->setMinimum( 0 );
    mImageCacheMaxSizeSpinBox->setMaximum( 9999 );
    mImageCacheMaxSizeSpinBox->setSuffix( " MB" );

    QHBoxLayout* cacheMaxLayout = new QHBoxLayout();
    cacheMaxLayout->addStretch();
    cacheMaxLayout->addWidget( new QLabel( tr("Card image max cache size:") ) );
    cacheMaxLayout->addWidget( mImageCacheMaxSizeSpinBox );

    mBasicLandMuidSetComboBox = new QComboBox();

    // Add all entries to the preset combobox, custom last.
    for( auto& kv : sBasicLandMuidPresetsMap )
    {
        mBasicLandMuidSetComboBox->addItem( kv.first );
    }
    mBasicLandMuidSetComboBox->addItem( CUSTOM_PRESET_NAME );

    // Set up handler for combobox selection.
    connect( mBasicLandMuidSetComboBox, &QComboBox::currentTextChanged, this, [this](const QString& newPresetName) {
            const BasicLandMuidMap* muidMapPtr = nullptr;
            if( newPresetName == CUSTOM_PRESET_NAME )
            {
                muidMapPtr = &mCustomBasicLandMuidPreset;
            }
            else
            {
                auto iter = sBasicLandMuidPresetsMap.find( newPresetName );
                if( iter != sBasicLandMuidPresetsMap.end() )
                {
                    muidMapPtr = &(iter->second);
                }
            }

            // Safety check - should never happen.
            if( muidMapPtr == nullptr ) return;

            // Set items to preset values.
            mBasicLandMuidsResetting = true;
            for( auto basic : gBasicLandTypeArray )
            {
                mBasicLandMuidLineEditMap[basic]->setText( QString::number( muidMapPtr->getMuid( basic ) ) );
            }
            mBasicLandMuidsResetting = false;
        });

    QIntValidator* validator = new QIntValidator( this );
    validator->setBottom( 0 );
    for( auto basic : gBasicLandTypeArray )
    {
        mBasicLandMuidLineEditMap[basic] = new QLineEdit();
        mBasicLandMuidLineEditMap[basic]->setValidator( validator );

        // When any of the values change, save to custom setting and change
        // combobox preset to custom (always last).
        connect( mBasicLandMuidLineEditMap[basic], &QLineEdit::textChanged, this, [this,basic](const QString& text) {
                mCustomBasicLandMuidPreset.setMuid( basic, text.toInt() );
                if( !mBasicLandMuidsResetting )
                {
                    mBasicLandMuidSetComboBox->setCurrentIndex( mBasicLandMuidSetComboBox->count() - 1 );
                }
            });
    }

    QGroupBox* basicLandGroupBox = new QGroupBox( tr("Basic Land Multiverse IDs") );
    {
        QVBoxLayout* gbLayout = new QVBoxLayout( basicLandGroupBox );

        QHBoxLayout* presetLayout = new QHBoxLayout();
        presetLayout->addStretch();
        presetLayout->addWidget( new QLabel( tr("Presets:") ) );
        presetLayout->addWidget( mBasicLandMuidSetComboBox );
        presetLayout->addStretch();

        QHBoxLayout *basicLandLayout = new QHBoxLayout();
        for( auto basic : gBasicLandTypeArray )
        {
            // Put space before every item but the first.
            if( basic != gBasicLandTypeArray[0] ) basicLandLayout->addSpacing( 10 );

            SizedSvgWidget *manaWidget = new SizedSvgWidget( BASIC_LAND_MANA_SYMBOL_SIZE );
            manaWidget->load( sSvgResourceMap.at( basic ) );
            basicLandLayout->addWidget( manaWidget );

            mBasicLandMuidLineEditMap[basic]->setFixedWidth( 60 );
            basicLandLayout->addWidget( mBasicLandMuidLineEditMap[basic] );
        }

        gbLayout->addLayout( presetLayout );
        gbLayout->addLayout( basicLandLayout );
    }

    QVBoxLayout* layout = new QVBoxLayout( widget );
    layout->addLayout( cacheMaxLayout );
    layout->addWidget( basicLandGroupBox );


    /* TODO future feature
    QCheckBox* beepCb = new QCheckBox( tr("Beep on new pack available") );
    layout->addWidget( beepCb );
    */

    return widget;
}


QWidget*
SettingsDialog::buildFactoryResetWidget()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout( widget );

    QPushButton* button = new QPushButton( tr("Reset all settings") );
    layout->addWidget( button );

    connect( button, &QPushButton::clicked, this, [this](bool) {
            QMessageBox::StandardButton ret;
            ret = QMessageBox::warning( this, tr("Reset Settings"),
                                        tr("Reset all settings to installation defaults?"),
                                        QMessageBox::Ok | QMessageBox::Cancel,
                                        QMessageBox::Cancel );
            if( ret == QMessageBox::Ok )
            {
                mSettings->reset();
                resetValues();
            }
        } );

    return widget;
}

