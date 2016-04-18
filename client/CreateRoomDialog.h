#ifndef CREATEROOMDIALOG_H
#define CREATEROOMDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QCheckBox;
class QComboBox;
QT_END_NAMESPACE

#include "clienttypes.h"
#include "Logging.h"

class CreateRoomDialog : public QDialog
{
    Q_OBJECT

public:

    CreateRoomDialog( const Logging::Config& loggingConfig = Logging::Config(),
                      QWidget*               parent = 0 );

    void setRoomCapabilitySets( const std::vector<RoomCapabilitySetItem>& sets );

    QStringList getSetCodes() const;
    QString getRoomName() const;
    QString getPassword() const;
    int getChairCount() const;
    int getBotCount() const;
    int getSelectionTime() const;

private slots:
    void tryEnableCreateButton();
    void handleselectionTimeCheckBoxToggled( bool checked );

private:

    QLineEdit* mRoomNameLineEdit;
    QLineEdit* mPasswordLineEdit;

    QComboBox* mPackComboBox[3];

    QComboBox* mChairCountComboBox;
    QComboBox* mBotCountComboBox;

    QCheckBox* mSelectionTimeCheckBox;
    QComboBox* mSelectionTimeComboBox;

    QPushButton* mCreateButton;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // CONNECTDIALOG_H
