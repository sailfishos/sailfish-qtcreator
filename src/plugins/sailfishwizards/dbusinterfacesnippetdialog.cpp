/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "dbusinterfacesnippetdialog.h"
#include "common.h"

#include <QFile>
#include <QMetaType>
#include <QtWidgets>
#include <QWidget>

#include <coreplugin/messagemanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/fileutils.h>
#include <utils/temporarydirectory.h>
#include <utils/qtcassert.h>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructor initializes dialogs fields with services data from JSON file.
 * \param parent Parent of the class instance.
 */
DBusInterfaceSnippetDialog::DBusInterfaceSnippetDialog(QWidget *parent): QDialog(parent)
{
    m_pageUi.setupUi(this);
    m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    validateInput(false, m_pageUi.changeService, tr("Select service"));
    m_busTypes = QStringList{"", "Session Bus", "System Bus"};
    m_pageUi.changeBusType->addItems(m_busTypes);
    m_methodsSelectingHint = m_pageUi.methodsTable->toolTip();
    m_signalsSelectingHint  = m_pageUi.signalsTable->toolTip();

    loadServicesFromJson();
    for (DBusService service : m_services) {
        if (service.getBusType() == "session"
                && !m_sessionServices.contains(service.getHumanizedServiceName()))
            m_sessionServices.append(service.getHumanizedServiceName());
        else if (service.getBusType() == "system"
                 && !m_systemServices.contains(service.getHumanizedServiceName()))
            m_systemServices.append(service.getHumanizedServiceName());
    }

    m_tableModel = new DBusServicesTableModel(m_servicesToPrint, m_checks, true, false);
    m_pageUi.methodsTable->setModel(m_tableModel);

    m_tableSignalsModel = new DBusServicesTableModel(m_signalsToPrint, m_signalsChecks, false, false);
    m_pageUi.signalsTable->setModel(m_tableSignalsModel);

    QStringList listWithoutEmptyString = m_systemServices;
    listWithoutEmptyString.removeFirst();
    if (m_pageUi.changeBusType->currentText().isEmpty())
        m_pageUi.changeService->addItems(m_sessionServices + listWithoutEmptyString);


    QObject::connect(m_pageUi.methodsTable, &QTableView::clicked, this,
                     &DBusInterfaceSnippetDialog::turnCheckOn);

    QObject::connect(m_pageUi.signalsTable, &QTableView::clicked, this,
                     &DBusInterfaceSnippetDialog::turnCheckOn);

    QObject::connect(m_tableModel, &DBusServicesTableModel::dataChanged, this,
                     &DBusInterfaceSnippetDialog::enableInsert);
    QObject::connect(m_tableSignalsModel, &DBusServicesTableModel::dataChanged, this,
                     &DBusInterfaceSnippetDialog::enableInsert);

    QObject::connect(m_pageUi.changeBusType, &QComboBox::currentTextChanged, this,
                     &DBusInterfaceSnippetDialog::updateServices);
    QObject::connect(m_pageUi.changeService, &QComboBox::currentTextChanged, this,
                     &DBusInterfaceSnippetDialog::updateServicePathsOrMethods);
    QObject::connect(m_pageUi.changeServicePath, &QComboBox::currentTextChanged, this,
                     &DBusInterfaceSnippetDialog::updateInterfaces);
    QObject::connect(m_pageUi.changeInterface, &QComboBox::currentTextChanged, this,
                     &DBusInterfaceSnippetDialog::updateMethods);
    QObject::connect(m_pageUi.advancedMode, &QCheckBox::stateChanged, this,
                     &DBusInterfaceSnippetDialog::updateInputFields);

    QObject::connect(m_pageUi.buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this,
                     &QDialog::reject);
    QObject::connect(m_pageUi.buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked, this,
                     &QDialog::accept);
    QObject::connect(this, &QDialog::accepted, this, &DBusInterfaceSnippetDialog::insert);

    QObject::connect(m_tableModel, &DBusServicesTableModel::checksChanged, this,
                     &DBusInterfaceSnippetDialog::updateListOfMethods);

    QObject::connect(this, &DBusInterfaceSnippetDialog::inputChanged, this,
                     &DBusInterfaceSnippetDialog::generateHint);
}

/*!
 * \brief Destructor clears the memory of model objects
 * after the completion of the dialog.
 */
DBusInterfaceSnippetDialog::~DBusInterfaceSnippetDialog()
{
    delete m_tableModel;
    delete m_tableSignalsModel;
}

/*!
 * \brief Change the status of the method selection
 * when clicking on the method name.
 */
void DBusInterfaceSnippetDialog::turnCheckOn(const QModelIndex &index)
{
    DBusServicesTableModel *tableModel;
    if (sender() == m_pageUi.methodsTable)
        tableModel = m_tableModel;
    else
        tableModel = m_tableSignalsModel;
    QList<bool> checks = tableModel->getChecks();
    if (checks[index.row()])
        checks[index.row()] = false;
    else
        checks[index.row()] = true;
    tableModel->setChecks(checks);
    emit tableModel->dataChanged(QModelIndex(), QModelIndex());
    updateListOfMethods();
    emit inputChanged();
}

/*!
 * \brief Filters a list of available methods and signals in the wizard's simple mode.
 * After selecting a method or signal, only the methods and signals of the
 * interface that have already been selected remain in the \c m_pageUi.methodsTable.
 */
void DBusInterfaceSnippetDialog::removeServicesWithWrongPaths(DBusServicesTableModel *firstModel,
                                                              DBusServicesTableModel *secondModel)
{
    QList<bool> firstChecks = firstModel->getChecks();
    QList<DBusService> listOfFirstModel = firstModel->getServices();
    QList<DBusService> listOfSecondModel = secondModel->getServices();
    QList<bool> secondChecks = secondModel->getChecks();
    DBusService trueService = listOfFirstModel.at(firstChecks.indexOf(true));
    QList<DBusService> tmpServices = listOfFirstModel;

    bool checksReseted = false;
    for (DBusService service : listOfFirstModel) {
        if (service.getInterface() != trueService.getInterface()) {
            if (!checksReseted) {
                checksReseted = true;
                std::fill(firstChecks.begin(), firstChecks.end(), false);
            }
            firstChecks.removeLast();
            tmpServices.removeOne(service);
        }
    }
    listOfFirstModel = tmpServices;

    checksReseted = false;
    tmpServices = listOfSecondModel;
    for (DBusService service : listOfSecondModel) {
        if (service.getInterface() != trueService.getInterface()) {
            if (!checksReseted) {
                checksReseted = true;
                std::fill(secondChecks.begin(), secondChecks.end(), false);
            }
            secondChecks.removeLast();
            tmpServices.removeOne(service);
        }
    }
    listOfSecondModel = tmpServices;

    firstChecks.replace(listOfFirstModel.indexOf(trueService), true);
    firstModel->update(listOfFirstModel, firstModel->getIsMethodsTable());
    firstModel->setChecks(firstChecks);

    secondModel->update(listOfSecondModel, secondModel->getIsMethodsTable());
    secondModel->setChecks(secondChecks);
    m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

/*!
 * \brief Keeps track of which model the choices of
 * methods and signals have changed.
 */
void DBusInterfaceSnippetDialog::updateListOfMethods()
{
    if (!m_pageUi.advancedMode->isChecked()) {
        m_checks = m_tableModel->getChecks();
        m_signalsChecks = m_tableSignalsModel->getChecks();
        if (m_checks.contains(true)) {
            removeServicesWithWrongPaths(m_tableModel, m_tableSignalsModel);
        } else if (m_signalsChecks.contains(true))
            removeServicesWithWrongPaths(m_tableSignalsModel, m_tableModel);
        else {
            m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
            updateServicePathsOrMethods();
        }
    }
    emit inputChanged();
}

/*!
 * \brief Generates hints for user input.
 */
void DBusInterfaceSnippetDialog::generateHint()
{
    if (m_pageUi.advancedMode->isChecked()) {
        if (m_pageUi.changeBusType->currentText().isEmpty()) {
            updateHintsWithout(m_pageUi.changeBusType);
            validateInput(false, m_pageUi.changeBusType, tr("Select bus type"));
        } else if (m_pageUi.changeService->currentText().isEmpty()) {
            updateHintsWithout(m_pageUi.changeService);
            validateInput(false, m_pageUi.changeService, tr("Select service"));
        } else if (m_pageUi.changeServicePath->currentText().isEmpty()) {
            updateHintsWithout(m_pageUi.changeServicePath);
            validateInput(false, m_pageUi.changeServicePath, tr("Select service path"));
        } else if (m_pageUi.changeInterface->currentText().isEmpty()) {
            updateHintsWithout(m_pageUi.changeInterface);
            validateInput(false, m_pageUi.changeInterface, tr("Select interface"));
        } else {
            validateInput(true, m_pageUi.changeInterface, "");
        }
    } else {
        if (m_pageUi.changeService->currentText().isEmpty()) {
            updateHintsWithout(m_pageUi.changeService);
            validateInput(false, m_pageUi.changeService, tr("Select service"));
        } else if (!m_tableModel->getChecks().contains(true) &&
                   !m_tableSignalsModel->getChecks().contains(true)) {
            updateHintsWithout(m_pageUi.methodsTable);
            m_pageUi.errorLabel->setText(tr("Select methods or signals you need"));
        } else
            m_pageUi.errorLabel->clear();
    }
}

/*!
 * \brief Discards unnecessary hints. Only one input hint is displayed
 * for the current field.
 * \param currentWidget Widget for which the hint will be shown.
 */
void DBusInterfaceSnippetDialog::updateHintsWithout(QWidget *currentWidget)
{
    for (QWidget *widget : getWidgets())
        if (widget != currentWidget)
            validateInput(true, widget, "");
}

/*!
 * \brief Returns the list of widgets of the page of the wizard.
 */
QList<QWidget *> DBusInterfaceSnippetDialog::getWidgets()
{
    QList<QWidget *> widgets;
    widgets.append(m_pageUi.changeBusType);
    widgets.append(m_pageUi.changeService);
    widgets.append(m_pageUi.changeServicePath);
    widgets.append(m_pageUi.changeInterface);
    return widgets;
}

/*!
 * \brief Highlights the field with incorrect input and displays an error message.
 * \param condition Field's correct status.
 * \param widget Verifiable widget.
 * \param errorMessage Hint for input.
 */
void DBusInterfaceSnippetDialog::validateInput(bool condition, QWidget *widget,
                                               const QString &errorMessage) const
{
    QPalette palette;
    if (condition) {
        widget->setPalette(QPalette());
        m_pageUi.errorLabel->clear();
    } else {
        palette.setColor(QPalette::Window, Qt::red);
        widget->setPalette(palette);
        m_pageUi.errorLabel->setText(errorMessage);
    }
}

/*!
 * \brief Parses JSON file with registered dbus services data
 *  and fills the list with found services.
 */
void DBusInterfaceSnippetDialog::loadServicesFromJson()
{
    QString value;
    QFile jsonFile(":/sailfishwizards/data/dbus-services.json");
    jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    value += QTextCodec::codecForName("UTF-8")->toUnicode(jsonFile.readAll());
    jsonFile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject jsonObject = jsonDoc.object();
    for (QJsonObject::Iterator iter = jsonObject.begin(); iter != jsonObject.end(); ++iter) {
        QJsonObject service = iter.value().toObject();
        QJsonObject servicePath = service["ServicePath"].toObject();

        for (QJsonObject::Iterator pathsIter = servicePath.begin(); pathsIter != servicePath.end();
                ++pathsIter) {
            QJsonObject paths = pathsIter.value().toObject();
            QJsonObject interface = paths["Interface"].toObject();

            for (QJsonObject::Iterator interfaceIter = interface.begin(); interfaceIter != interface.end();
                    ++interfaceIter) {
                QJsonObject interfacePath = interfaceIter.value().toObject();
                QJsonObject method = interfacePath["Methods"].toObject();
                QJsonObject signal = interfacePath["Signals"].toObject();

                for (QJsonObject::Iterator methodIter = method.begin(); methodIter != method.end(); ++methodIter) {
                    DBusService dbusService = DBusService(iter.key(), service["ServiceName"].toString(),
                                                          pathsIter.key(),
                                                          service["BusType"].toString(), interfaceIter.key(), methodIter.key());
                    m_services.append(dbusService);
                }

                for (QJsonObject::Iterator signalIter = signal.begin(); signalIter != signal.end(); ++signalIter) {
                    DBusService dbusService = DBusService(iter.key(), service["ServiceName"].toString(),
                                                          pathsIter.key(),
                                                          service["BusType"].toString(), interfaceIter.key(), "", signalIter.key());
                    m_services.append(dbusService);
                }
            }
        }
    }
}

/*!
 * \brief Controls the activation of the insert button.
 */
void DBusInterfaceSnippetDialog::enableInsert()
{
    if (!m_pageUi.advancedMode->isChecked() && (m_tableModel->getChecks().contains(true)
                                                || m_tableSignalsModel->getChecks().contains(true)))
        m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    else if (m_pageUi.advancedMode->isChecked() && !(m_pageUi.changeInterface->currentText().isEmpty()))
        m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    else
        m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

/*!
 * \brief Return a temp file pattern with extension or not.
 * \param prefix Name of file.
 * \param extension Extension of file.
 */
static inline QString tempFilePattern(const QString &prefix, const QString &extension)
{
    QString pattern = Utils::TemporaryDirectory::masterDirectoryPath();
    const QChar slash = QLatin1Char('/');
    if (!pattern.endsWith(slash))
        pattern.append(slash);
    pattern += prefix;
    pattern += QLatin1String("_XXXXXX.");
    pattern += extension;
    return pattern;
}

/*!
 * \brief Generates insert of DBusInterface and opens
 * a temporary file with this insert in the editor.
 */
void DBusInterfaceSnippetDialog::insert()
{
    QString content, filePrefix = "DBusInterfaceSnippet";
    if (m_pageUi.cppFileFlag->isChecked()) {
        content = generateCppSnippet();
    } else {
        content = generateQmlSnippet();
    }
    QByteArray byteContent = content.toUtf8();
    Utils::TempFileSaver saver(tempFilePattern(filePrefix, m_suffix));
    saver.setAutoRemove(false);
    saver.write(byteContent);
    if (!saver.finalize()) {
        Core::MessageManager::write(saver.errorString());
        return;
    }
    const QString fileName = saver.fileName();

    Core::IEditor *editor = Core::EditorManager::openEditor(fileName);
    QTC_ASSERT(editor, return);
    editor->document()->setUniqueDisplayName(filePrefix);
}

/*!
 * \brief Updates the list of methods.
 */
void DBusInterfaceSnippetDialog::updateMethods()
{
    if (m_pageUi.advancedMode->isChecked() && !(m_pageUi.changeInterface->currentText().isEmpty())) {
        m_servicesToPrint.clear();
        m_checks.clear();
        m_signalsChecks.clear();
        m_signalsToPrint.clear();
        for (DBusService service : m_services) {
            if (service.getInterface() == m_pageUi.changeInterface->currentText() &&
                    !service.getMethod().isEmpty()) {
                m_servicesToPrint.append(service);
                m_checks.append(false);
            }
            if (service.getInterface() == m_pageUi.changeInterface->currentText() &&
                    !service.getSignal().isEmpty()) {
                m_signalsToPrint.append(service);
                m_signalsChecks.append(false);
            }
        }
        m_tableSignalsModel->update(m_signalsToPrint, m_tableSignalsModel->getIsMethodsTable());
        m_tableModel->update(m_servicesToPrint, m_tableModel->getIsMethodsTable());
        m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        m_tableModel->clearTable();
        m_tableSignalsModel->clearTable();
        if (m_pageUi.changeInterface->currentText().isEmpty())
            m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
    emit inputChanged();
}

/*!
 * \brief Updates the list of interfaces.
 */
void DBusInterfaceSnippetDialog::updateInterfaces()
{
    if (m_pageUi.advancedMode->isChecked()) {
        m_pageUi.changeInterface->clear();
        QStringList interfaces = {""};
        for (DBusService service : m_services) {
            if (service.getServicePath() == m_pageUi.changeServicePath->currentText()
                    && !interfaces.contains(service.getInterface())
                    && service.getHumanizedServiceName() == m_pageUi.changeService->currentText()) {
                interfaces.append(service.getInterface());
            }
        }
        m_pageUi.changeInterface->addItems(interfaces);

        if (m_pageUi.changeServicePath->currentText().isEmpty()) {
            m_pageUi.changeInterface->setEnabled(false);
            m_pageUi.interfaceName->setEnabled(false);
        } else {
            m_pageUi.changeInterface->setEnabled(true);
            m_pageUi.interfaceName->setEnabled(true);
        }
        emit inputChanged();
    }
}

/*!
 * \brief Updates the list of service paths in advanced mode and
 * updates the list of methods in simple mode.
 */
void DBusInterfaceSnippetDialog::updateServicePathsOrMethods()
{
    if (m_pageUi.advancedMode->isChecked()) {
        m_pageUi.changeServicePath->clear();
        QStringList servicePaths = {""};
        for (DBusService service : m_services) {
            if (service.getHumanizedServiceName() == m_pageUi.changeService->currentText()
                    && !servicePaths.contains(service.getServicePath())) {
                servicePaths.append(service.getServicePath());
            }
        }
        m_pageUi.changeServicePath->addItems(servicePaths);

        if (m_pageUi.changeService->currentText().isEmpty()) {
            m_pageUi.changeServicePath->setEnabled(false);
            m_pageUi.servicePath->setEnabled(false);
        } else {
            m_pageUi.changeServicePath->setEnabled(true);
            m_pageUi.servicePath->setEnabled(true);
        }
    } else {
        m_servicesToPrint.clear();
        m_signalsToPrint.clear();
        m_signalsChecks.clear();
        m_checks.clear();
        for (DBusService service : m_services) {
            if (service.getHumanizedServiceName() == m_pageUi.changeService->currentText() &&
                    !service.getMethod().isEmpty()) {
                m_servicesToPrint.append(service);
                m_checks.append(false);
            }
            if (service.getHumanizedServiceName() == m_pageUi.changeService->currentText() &&
                    !service.getSignal().isEmpty()) {
                m_signalsToPrint.append(service);
                m_signalsChecks.append(false);
            }
        }
        m_tableSignalsModel->update(m_signalsToPrint, m_tableSignalsModel->getIsMethodsTable());
        m_tableModel->update(m_servicesToPrint, m_tableModel->getIsMethodsTable());
    }
    emit inputChanged();
}

/*!
 * \brief Updates the list of services.
 */
void DBusInterfaceSnippetDialog::updateServices()
{
    if (m_pageUi.changeBusType->currentText() == "Session Bus") {
        m_pageUi.changeService->setEnabled(true);
        m_pageUi.serviceName->setEnabled(true);
        m_pageUi.changeService->clear();
        m_pageUi.changeService->addItems(m_sessionServices);
    } else if (m_pageUi.changeBusType->currentText() == "System Bus") {
        m_pageUi.changeService->setEnabled(true);
        m_pageUi.serviceName->setEnabled(true);
        m_pageUi.changeService->clear();
        m_pageUi.changeService->addItems(m_systemServices);
    } else {
        m_pageUi.changeService->setCurrentIndex(0);
        if (m_pageUi.advancedMode->isChecked()) {
            m_pageUi.changeService->setEnabled(false);
            m_pageUi.serviceName->setEnabled(false);
        } else {
            m_pageUi.changeService->setEnabled(true);
            m_pageUi.serviceName->setEnabled(true);
        }
        QStringList listWithoutEmptyString = m_systemServices;
        listWithoutEmptyString.removeFirst();
        m_pageUi.changeService->clear();
        m_pageUi.changeService->addItems(m_sessionServices + listWithoutEmptyString);
    }
    emit inputChanged();
}

/*!
 * \brief Controls activation and cleaning of
 * fields when switching wizard's mode.
 */
void DBusInterfaceSnippetDialog::updateInputFields()
{
    if (m_pageUi.advancedMode->isChecked()) {
        m_pageUi.changeService->setCurrentIndex(0);
        m_pageUi.changeBusType->setCurrentIndex(0);
        m_pageUi.changeBusType->setEnabled(true);
        m_pageUi.changeService->setEnabled(false);
        m_pageUi.serviceName->setEnabled(false);
        m_pageUi.busType->setEnabled(true);
        if (m_pageUi.changeInterface->currentText().isEmpty())
            m_tableModel->clearTable();
        m_tableModel->setAdvancedMode(true);
        m_tableSignalsModel->setAdvancedMode(true);
        m_pageUi.methodsTable->setToolTip("");
        m_pageUi.signalsTable->setToolTip("");
    } else {
        m_pageUi.methodsTable->setToolTip(m_methodsSelectingHint);
        m_pageUi.signalsTable->setToolTip(m_signalsSelectingHint);
        m_pageUi.changeBusType->setCurrentIndex(0);
        m_pageUi.changeInterface->setCurrentIndex(0);
        m_pageUi.changeServicePath->setCurrentIndex(0);

        if (!m_tableModel->getChecks().contains(true))
            m_pageUi.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);;

        m_pageUi.serviceName->setEnabled(true);
        m_pageUi.changeService->setEnabled(true);
        m_pageUi.changeBusType->setEnabled(false);
        m_pageUi.changeInterface->setEnabled(false);
        m_pageUi.changeServicePath->setEnabled(false);
        m_pageUi.busType->setEnabled(false);
        m_pageUi.interfaceName->setEnabled(false);
        m_pageUi.servicePath->setEnabled(false);
        m_tableModel->setAdvancedMode(false);
        m_tableSignalsModel->setAdvancedMode(false);
    }
    emit inputChanged();
}

/*!
 * \brief Parses the fields of the wizard and inserts
 * the data into a cpp file template.
 */
QString DBusInterfaceSnippetDialog::generateCppSnippet()
{
    QHash <QString, QString> values;
    QString errorMessage("Error of template parsing");
    QString methodsString = "";
    QString signalsString = "";
    m_suffix = QLatin1String("cpp");
    DBusService service;
    if (!m_tableModel->getServices().isEmpty())
        service = m_tableModel->getServices().first();
    else
        service = m_tableSignalsModel->getServices().first();
    values.insert("serviceName", service.getServiceName());
    values.insert("servicePath", service.getServicePath());
    values.insert("interfaceName", service.getInterface());
    values.insert("busType", service.getBusType());

    for (DBusService service : m_tableModel->getServices()) {
        QStringList params;
        QString param = service.getMethod();
        QRegExp rx("(\\,)");
        param.remove(0, param.indexOf("(") + 1);
        param.remove(")");
        params = param.split(rx);
        param.clear();
        for (QString arg : params) {
            if (!arg.isEmpty() && params.indexOf(arg) == 0)
                param += ", QVariant::fromValue(QDBusVariant("  + arg.simplified() + ")),";
            else if (!arg.isEmpty() && params.indexOf(arg) != 0)
                param += "\n\t\tQVariant::fromValue(QDBusVariant("  + arg.simplified() + ")),";
        }
        QString callsMethodName = service.getMethod();
        callsMethodName.remove(callsMethodName.indexOf("("), callsMethodName.indexOf(")") + 1);

        if (m_tableModel->getChecks().at(m_tableModel->getServices().indexOf(service)) == true) {
            methodsString += "remoteApp.call(\"" + callsMethodName + "\"" + param.left(
                                 param.length() - 1) + ");\n";
        }

    }
    methodsString = methodsString.trimmed();
    if (!methodsString.isEmpty())
        methodsString.push_front("\n\n");
    values.insert("methods", methodsString);

    for (DBusService service : m_tableSignalsModel->getServices())
        if (m_tableSignalsModel->getChecks().at(m_tableSignalsModel->getServices().indexOf(
                                                    service)) == true)
            signalsString += "emit remoteApp." + service.getSignal() + ";\n";
    signalsString = signalsString.trimmed();
    if (!signalsString.isEmpty())
        signalsString.push_front("\n\n");
    values.insert("signals", signalsString);

    return Common::processTemplate("qdbusinterfacecodefragment.template", values, &errorMessage);
}

/*!
 * \brief Parses the fields of the wizard and inserts
 * the data into a qml file template.
 */
QString DBusInterfaceSnippetDialog::generateQmlSnippet()
{
    QHash <QString, QString> values;
    QString errorMessage("Error of template parsing");
    QString methodsString = "";
    QString signalsString = "";
    m_suffix = QLatin1String("qml");

    DBusService service;
    if (!m_tableModel->getServices().isEmpty())
        service = m_tableModel->getServices().first();
    else
        service = m_tableSignalsModel->getServices().first();
    values.insert("serviceName", service.getServiceName());
    values.insert("servicePath", service.getServicePath());
    values.insert("interfaceName", service.getInterface());
    QString bus = service.getBusType();
    values.insert("busType", bus.left(1).toUpper() + bus.mid(1));
    if (m_tableModel->getChecks().contains(true))
        values.insert("signalsMode", "\n\n    signalsEnabled: true");
    else
        values.insert("signalsMode", "");
    for (DBusService service : m_tableModel->getServices()) {
        QString params = service.getMethod();
        params.remove(0, params.indexOf("(") + 1);
        params.remove(")");
        if (!params.isEmpty())
            params = ", " + params;
        QString callsMethodName = service.getMethod();
        callsMethodName.remove(callsMethodName.indexOf("("), callsMethodName.indexOf(")") + 1);

        if (m_tableModel->getChecks().at(m_tableModel->getServices().indexOf(service)) == true) {
            methodsString += "    function " + service.getMethod() + " {\n"
                             "        call(\"" + callsMethodName + "\"" + params + ")\n    }\n\n";
        }
    }
    methodsString = methodsString.trimmed();
    if (!methodsString.isEmpty())
        methodsString.push_front("\n\n    ");
    values.insert("methods", methodsString);

    for (DBusService service : m_tableSignalsModel->getServices()) {
        QString signal = service.getSignal();

        if (m_tableSignalsModel->getChecks().at(m_tableSignalsModel->getServices().indexOf(
                                                    service)) == true) {
            signalsString += "dbusService." + signal + "\n";
        }
    }
    signalsString = signalsString.trimmed();
    if (!signalsString.isEmpty())
        signalsString.push_front("\n\n");
    values.insert("signals", signalsString);

    return Common::processTemplate("qdbusinterfacecodefragment.qml", values, &errorMessage);
}

} // namespace Internal
} // namespace SailfishWizards
