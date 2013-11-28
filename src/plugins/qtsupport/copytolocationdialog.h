#ifndef COPYTOLOCATIONWIDGET_H
#define COPYTOLOCATIONWIDGET_H

#include <QDialog>


namespace QtSupport {
namespace Internal {

namespace Ui {
class CopyToLocationDialog;
}

//Pasive view

class CopyToLocationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CopyToLocationDialog(QWidget *parent = 0);
    void setSourcePath(const QString& path);
    QString sourcePath() const;
    void setDestinationPath(const QString& path);
    QString destinationPath() const;
private:
    Ui::CopyToLocationDialog *m_ui;
};

}
}
#endif // COPYTOLOCATIONWIDGET_H
