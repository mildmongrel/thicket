#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;
QT_END_NAMESPACE

#include "BasicLand.h"
#include "BasicLandMuidMap.h"
#include "Logging.h"

class ClientSettings;


class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog( ClientSettings*        settings,
                    const Logging::Config& loggingConfig = Logging::Config(),
                    QWidget*               parent = 0 );

public slots:
    virtual int exec() override;

private slots:
    void updateChangedSettings();

private:

    QWidget* buildGeneralWidget();
    QWidget* buildFactoryResetWidget();
    void resetValues();

    ClientSettings* mSettings;

    QTabWidget* mTabWidget;
    QSpinBox*   mImageCacheMaxSizeSpinBox;

    QComboBox*  mBasicLandMuidSetComboBox;
    std::map<BasicLandType,QLineEdit*>  mBasicLandMuidLineEditMap;

    BasicLandMuidMap mCustomBasicLandMuidPreset;
    bool mBasicLandMuidsResetting;

    std::shared_ptr<spdlog::logger> mLogger;
};

#endif  // SETTINGSDIALOG_H
