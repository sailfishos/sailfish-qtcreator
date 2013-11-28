#include "copytolocationdialog.h"
#include "ui_copytolocationdialog.h"


namespace QtSupport {
namespace Internal {

CopyToLocationDialog::CopyToLocationDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::CopyToLocationDialog)
{
    m_ui->setupUi(this);
}

void CopyToLocationDialog::setSourcePath(const QString& path)
{
    m_ui->locationLabel->setText(QString::fromLatin1("<blockquote>%1</blockquote>").arg(path));
}

QString CopyToLocationDialog::sourcePath() const
{
    return m_ui->locationLabel->text();
}

void CopyToLocationDialog::setDestinationPath(const QString& path)
{
    m_ui->pathChooser->setPath(path);
}

QString CopyToLocationDialog::destinationPath() const
{
    return m_ui->pathChooser->path();
}

}
}
