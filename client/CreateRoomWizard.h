#ifndef CREATEROOMWIZARD_H
#define CREATEROOMWIZARD_H

#include <QMap>
#include <QWizard>
#include <QWizardPage>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListWidget;
class QRadioButton;
class QTableWidget;
class QTextBrowser;
class QVBoxLayout;
QT_END_NAMESPACE

namespace proto {
    class RoomConfig;
}

class ImageLoaderFactory;
class ExpSymImageLoader;

// Forward-declare pages; classes defined below
class CreateRoomTypeWizardPage;
class CreateRoomConfigWizardPage;
class CreateRoomCubeWizardPage;
class CreateRoomPacksWizardPage;
class CreateRoomSummaryWizardPage;

#include "clienttypes.h"
#include "Decklist.h"
#include "Logging.h"

class CreateRoomWizard : public QWizard
{
    Q_OBJECT

public:

    enum DraftType
    {
        DRAFT_TYPE_BOOSTER,
        DRAFT_TYPE_SEALED,
        DRAFT_TYPE_GRID
    };

    enum Page
    {
        PAGE_TYPE,
        PAGE_CONFIG,
        PAGE_CUBE,
        PAGE_PACKS,
        PAGE_SUMMARY
    };

    static const QString CUBE_SET_CODE;

    CreateRoomWizard( ImageLoaderFactory*    imageLoaderFactory,
                      const Logging::Config& loggingConfig = Logging::Config(),
                      QWidget*               parent = 0 );

    void setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets );

    bool fillRoomConfig( proto::RoomConfig* roomConfig ) const;
    QString getPassword() const;

    virtual int nextId() const override;

private:

    bool      mCubeLoaded;
    QString   mCubeName;
    Decklist  mCubeDecklist;

    CreateRoomTypeWizardPage*     mTypePage;
    CreateRoomConfigWizardPage*   mConfigPage;
    CreateRoomCubeWizardPage*     mCubePage;
    CreateRoomPacksWizardPage*    mPacksPage;
    CreateRoomSummaryWizardPage*  mSummaryPage;

    std::shared_ptr<spdlog::logger> mLogger;
};



class CreateRoomTypeWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    CreateRoomTypeWizardPage( std::shared_ptr<spdlog::logger>& logger,
                              QWidget*                         parent = 0 );

    CreateRoomWizard::DraftType getDraftType() const;
    bool isCube() const;

private:
    CreateRoomWizard::DraftType mType;
    bool                        mIsCube;

    QRadioButton* mBoosterButton;
    QRadioButton* mSealedButton;
    QRadioButton* mCubeBoosterButton;
    QRadioButton* mCubeSealedButton;
    QRadioButton* mGridButton;

    std::shared_ptr<spdlog::logger> mLogger;
};



class CreateRoomConfigWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    CreateRoomConfigWizardPage( CreateRoomTypeWizardPage const * createRoomTypeWizardPage,
                                std::shared_ptr<spdlog::logger>& logger,
                                QWidget*                         parent = 0 );

    QString getName() const;
    QString getPassword() const;
    int getChairCount() const;
    int getBotCount() const;
    int getSelectionTime() const;

    virtual void initializePage() override;
    virtual bool isComplete() const override;

private:

    CreateRoomTypeWizardPage const * mCreateRoomTypeWizardPage;

    QGridLayout* mLayout;

    QLabel* mNameLabel;
    QLabel* mPasswordLabel;
    QLabel* mChairCountLabel;
    QLabel* mBotCountLabel;
    QLabel* mGridBotLabel;
    QLabel* mSelectionTimeLabel;

    QLineEdit* mNameLineEdit;
    QLineEdit* mPasswordLineEdit;

    QComboBox* mChairCountComboBox;
    QComboBox* mBotCountComboBox;
    QCheckBox* mGridBotCheckBox;

    QCheckBox* mSelectionTimeCheckBox;
    QComboBox* mSelectionTimeComboBox;

    std::shared_ptr<spdlog::logger> mLogger;
};


class CreateRoomCubeWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    CreateRoomCubeWizardPage( std::shared_ptr<spdlog::logger>& logger,
                              QWidget*                         parent = 0 );

    QString getCubeName() const;
    const Decklist& getCubeDecklist() const { return mCubeDecklist; }

    virtual bool isComplete() const override;

private:

    void handleImportButton();

    QLabel* mCardCountLabel;
    QTableWidget* mCardTable;
    QLineEdit* mCubeNameLineEdit;

    Decklist mCubeDecklist;

    std::shared_ptr<spdlog::logger> mLogger;
};


class CreateRoomPacksWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    CreateRoomPacksWizardPage( CreateRoomTypeWizardPage const * createRoomTypeWizardPage,
                               ImageLoaderFactory*              imageLoaderFactory,
                               const Logging::Config&           loggingConfig,
                               QWidget*                         parent = 0 );

    void setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets );
    QStringList getPackSetCodes() const;

    virtual void initializePage() override;
    virtual bool isComplete() const override;

private slots:

    void handleExpSymImageLoaded( const QString& setCode, const QImage& image );

private:

    const unsigned int BOOSTER_PACK_COUNT = 3;
    const unsigned int SEALED_PACK_COUNT  = 6;
    const unsigned int MAX_PACK_COUNT     = SEALED_PACK_COUNT;

    void updatePackLabels();
    unsigned int getPackCount() const;

    CreateRoomTypeWizardPage const * mCreateRoomTypeWizardPage;
    ExpSymImageLoader*               mExpSymImageLoader;

    QVBoxLayout* mLayout;
    QListWidget* mListWidget;
    QVector<QLabel*> mPackLabels;

    QMap<QString,QString> mSetCodeToNameMap;
    QStringList mPackSetCodes;

    std::shared_ptr<spdlog::logger> mLogger;
};


class CreateRoomSummaryWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    CreateRoomSummaryWizardPage( CreateRoomTypeWizardPage*        typePage,
                                 CreateRoomConfigWizardPage*      configPage,
                                 CreateRoomCubeWizardPage*        cubePage,
                                 CreateRoomPacksWizardPage*       packsPage,
                                 std::shared_ptr<spdlog::logger>& logger,
                                 QWidget*                         parent = 0 );

    virtual void initializePage() override;

private:

    CreateRoomTypeWizardPage*   mTypePage;
    CreateRoomConfigWizardPage* mConfigPage;
    CreateRoomCubeWizardPage*   mCubePage;
    CreateRoomPacksWizardPage*  mPacksPage;

    QTextBrowser* mTextBrowser;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CREATEROOMWIZARD_H
