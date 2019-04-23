#ifndef %{UpperClassName}_GLOBAL_H
#define %{UpperClassName}_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(%{UpperClassName}_LIBRARY)
#  define %{UpperClassName}SHARED_EXPORT Q_DECL_EXPORT
#else
#  define %{UpperClassName}SHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // %{UpperClassName}_GLOBAL_H
