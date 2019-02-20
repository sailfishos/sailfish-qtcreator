#ifndef MERVIRTUALMACHINESETTINGSWIDGET_H
#define MERVIRTUALMACHINESETTINGSWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

namespace Ui {
class MerVirtualMachineSettingsWidget;
}

class MerVirtualMachineSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MerVirtualMachineSettingsWidget(QWidget *parent = nullptr);
    ~MerVirtualMachineSettingsWidget();
    void setMemorySizeMb(int sizeMb);
    void setCpuCount(int count);
    void setVdiCapacityMb(int vdiCapacityMb);
    void setVmOff(bool vmOff);
    QFormLayout *formLayout() const;

signals:
    void memorySizeMbChanged(int sizeMb);
    void cpuCountChanged(int count);
    void vdiCapacityMbChnaged(int sizeMb);

private:
    void initGui();
    Ui::MerVirtualMachineSettingsWidget *ui;
};

} // Internal
} // Mer

#endif // MERVIRTUALMACHINESETTINGSWIDGET_H
