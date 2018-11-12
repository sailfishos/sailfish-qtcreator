#include "existprojectpage.h"
#include "utils/wizard.h"
#include "utils/fileutils.h"
#include "utils/detailswidget.h"
#include <QHBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QFontMetrics>
#include <QScrollArea>
#include <QLabel>
#include <QPainter>

template<class T>
int getWidthAdjustText(const T &w)
{
    QFontMetrics fontMetrics = w->fontMetrics();
    return fontMetrics.width(w->text()) + 10;
}

namespace ProjectExplorer {
namespace Internal {

ExistProjectPage::ExistProjectPage(const QString &basePath, QWidget *parent) : WizardPage(parent),
    basePath(basePath)
{
    setTitle(tr("Add existing project"));
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Details"));
    createElements();
}

QVector<QString> ExistProjectPage::projects() const
{
    return addingProjects;
}

void ExistProjectPage::selectPrejectPath()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Add Existing Files"),
                                                    basePath,
                                                    "Pro file (*.pro)");
    if (fileName.isEmpty())
        return;
    addProject(fileName);
}

void ExistProjectPage::removePrejectPath(const QString &path)
{
    addingProjects.removeOne(path);
}

void ExistProjectPage::createElements()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QScrollArea *scrollArea = new QScrollArea(this);
    projectsLayout = new QVBoxLayout(this);

    projectsLayout->setAlignment(Qt::AlignTop);
    scrollArea->setLayout(projectsLayout);

    QPushButton *addButton = new QPushButton(tr("Add.."), this);
    addButton->setFixedWidth(getWidthAdjustText(addButton));
    connect(addButton, &QPushButton::clicked, this, &ExistProjectPage::selectPrejectPath);

    layout->addWidget(addButton);
    layout->addWidget(scrollArea);

    setLayout(layout);
}

void ExistProjectPage::addProject(const QString &path)
{
    if (addingProjects.contains(path))
        return;

    ExistProjectItem *project = new ExistProjectItem(path, this);
    connect(project, &ExistProjectItem::removed, this, &ExistProjectPage::removePrejectPath);
    projectsLayout->addWidget(project);

    addingProjects.append(path);
}

ExistProjectItem::ExistProjectItem(const QString &path, QWidget *parent) : QWidget(parent), path(path)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    QVBoxLayout *textLayout = new QVBoxLayout(this);

    Utils::FileName file = Utils::FileName ::fromString(path);
    QLabel *fileNameLabel = new QLabel(file.fileName());
    QFont font = fileNameLabel->font();
    font.setPointSize(12);
    fileNameLabel->setFont(font);

    QLabel *filePathLabel = new QLabel(file.toString());
    font = filePathLabel->font();
    font.setPointSize(8);
    filePathLabel->setFont(font);

    QPushButton *removeButton = new QPushButton(tr("Remove"), this);
    removeButton->setFixedWidth(getWidthAdjustText(removeButton));

    connect(removeButton, &QPushButton::clicked, [this]() {
        emit removed(this->path);
        this->deleteLater();
    });

    textLayout->addWidget(fileNameLabel);
    textLayout->addWidget(filePathLabel);

    layout->addLayout(textLayout);
    layout->addWidget(removeButton);
    setLayout(layout);

    setMaximumHeight(60);
}

void ExistProjectItem::paintEvent(QPaintEvent *paintEvent)
{
    QWidget::paintEvent(paintEvent);

    QPainter p(this);
    QPoint topLeft(geometry().left() - 8, contentsRect().top());
    QRect paintArea(topLeft, contentsRect().bottomRight());
    p.drawPixmap(paintArea, Utils::DetailsWidget::createBackground(paintArea.size(), 0, this));
}

}
}
