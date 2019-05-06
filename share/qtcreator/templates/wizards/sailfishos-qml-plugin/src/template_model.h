#ifndef %{ModelHdrGuard}
#define %{ModelHdrGuard}

#include <QObject>

class %{ModelClassName} : public QObject
{
    Q_OBJECT
public:
    %{ModelClassName}(QObject *parent=nullptr);
};

#endif // %{ModelHdrGuard}
