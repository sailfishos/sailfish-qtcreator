#include "mersigninguserselectiondialog.h"

#include <coreplugin/messagemanager.h>
#include <sfdk/utils.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QInputDialog>
#include <QMessageBox>

using namespace Sfdk;

namespace {

QList<GpgKeyInfo> getAvailableKeys()
{
    QList<GpgKeyInfo> availableKeys;
    bool ok;
    QString error;
    execAsynchronous(std::tie(ok, availableKeys, error), Sfdk::availableGpgKeys);
    if (!ok)
        Core::MessageManager::write(error, Core::MessageManager::Flash);

    return availableKeys;
}

} // namespace
namespace Mer {
namespace Internal {

MerSigningUserSelectionDialog::MerSigningUserSelectionDialog(QObject *parent)
    : QObject(parent)
{}

GpgKeyInfo MerSigningUserSelectionDialog::selectSigningUser()
{
    QString gpgErrorString;
    if (!Sfdk::isGpgAvailable(&gpgErrorString)) {
        QMessageBox::warning(nullptr, tr("No GPG GPG utility!"),
                gpgErrorString);
        return {};
    }
    const QList<GpgKeyInfo> availableKeys = getAvailableKeys();
    const QStringList items = Utils::transform(availableKeys, &GpgKeyInfo::toString);

    if (items.isEmpty()) {
        QMessageBox::warning(nullptr, tr("No GPG keys found!"),
                tr("Use GPG to generate a new key or export an existing key"));
        return {};
    }

    bool ok;
    const QString selected = QInputDialog::getItem(nullptr,
            tr("Select the GPG key"),
            tr("Select the GPG key to sign packages"),
            items, 0, false, &ok);
    if (!ok || selected.isEmpty())
        return {};

    const int index = items.indexOf(selected);
    QTC_ASSERT(index > -1 && index < availableKeys.count(), return {});
    return availableKeys.at(index);
}

} // namespace Internal
} // namespace Mer
