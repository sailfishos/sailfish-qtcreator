#ifndef EXISTPROJECTPAGE_H
#define EXISTPROJECTPAGE_H

#include <utils/wizardpage.h>
#include <QVector>
#include <QAbstractItemDelegate>

class QVBoxLayout;

namespace ProjectExplorer {
namespace Internal {

class ItemDelegate;

class ExistProjectPage : public Utils::WizardPage
{
    Q_OBJECT
public:
    explicit ExistProjectPage(const QString &basePath, QWidget *parent = nullptr);

    QVector<QString> projects() const;
private slots:
    void selectPrejectPath();
    void removePrejectPath(const QString &path);

private:
    void createElements();
    void addProject(const QString &path);

    QVector<QString> addingProjects;
    QVBoxLayout *projectsLayout;
    const QString basePath;
};

class ExistProjectItem : public QWidget
{
    Q_OBJECT
public:
    ExistProjectItem(const QString &path, QWidget *parent = 0);
signals:
    void removed(const QString &path);
protected:
    virtual void paintEvent(QPaintEvent *paintEvent);
private:
    const QString path;
};

}
}
#endif // EXISTPROJECTPAGE_H
