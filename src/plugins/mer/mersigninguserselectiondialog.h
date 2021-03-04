#ifndef MERSIGNINGUSERSELECTIONDIALOG_H
#define MERSIGNINGUSERSELECTIONDIALOG_H

#include <sfdk/utils.h>
#include <QObject>

namespace Mer {
namespace Internal {

class MerSigningUserSelectionDialog : public QObject
{
    Q_OBJECT
public:
    explicit MerSigningUserSelectionDialog(QObject *parent = nullptr);
    static Sfdk::GpgKeyInfo selectSigningUser();
};

} // namespace Internal
} // namespace Mer

#endif // MERSIGNINGUSERSELECTIONDIALOG_H
