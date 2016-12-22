#ifndef CREATEROOMDIALOG_H
#define CREATEROOMDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;
class QStackedWidget;
QT_END_NAMESPACE

#include "clienttypes.h"
#include "Decklist.h"
#include "Logging.h"

class CreateRoomDialog : public QDialog
{
    Q_OBJECT

public:

    enum DraftType
    {
        DRAFT_BOOSTER,
        DRAFT_SEALED
    };

    static const QString CUBE_SET_CODE;

    CreateRoomDialog( const Logging::Config& loggingConfig = Logging::Config(),
                      QWidget*               parent = 0 );

    void setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets );

    DraftType getDraftType() const;
    QStringList getSetCodes() const;
    QString getCubeName() const { return mCubeName; }
    Decklist getCubeDecklist() const { return mCubeDecklist; }
    QString getRoomName() const;
    QString getPassword() const;
    int getChairCount() const;
    int getBotCount() const;
    int getSelectionTime() const;

protected:
    virtual void resizeEvent( QResizeEvent* event ) override;
    virtual void showEvent( QShowEvent * event ) override;

private slots:
    void tryEnableCreateButton();
    void handleDraftTypeComboBoxIndexChanged( int index );
    void handleSelectionTimeCheckBoxToggled( bool checked );
    void handleImportCubeListButton();

private:
    void constructBoosterStackedWidget();
    void constructSealedStackedWidget();

    QPoint mCenter;

    QLineEdit* mRoomNameLineEdit;
    QLineEdit* mPasswordLineEdit;

    QVector<QComboBox*> mBoosterPackComboBoxes;
    QVector<QComboBox*> mSealedPackComboBoxes;

    QComboBox* mBoosterChairCountComboBox;
    QComboBox* mBoosterBotCountComboBox;
    QComboBox* mSealedChairCountComboBox;
    QComboBox* mDraftTypeComboBox;

    QStackedWidget* mDraftConfigStack;

    QCheckBox* mSelectionTimeCheckBox;
    QComboBox* mSelectionTimeComboBox;

    QPushButton* mCreateButton;
    QPushButton* mCancelButton;

    QLabel*   mImportCubeListNameLabel;
    QString   mCubeName;
    Decklist  mCubeDecklist;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CONNECTDIALOG_H
