/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef YAML_GLOBAL_H
#define YAML_GLOBAL_H

#include <qglobal.h>

#ifndef Q_DECL_EXPORT_OWN
#  if defined(Q_OS_WIN) || defined(Q_CC_NOKIAX86) || defined(Q_CC_RVCT)
#    define Q_DECL_EXPORT_OWN __declspec(dllexport)
#  elif defined(QT_VISIBILITY_AVAILABLE)
#    define Q_DECL_EXPORT_OWN __attribute__((visibility("default")))
#    define Q_DECL_HIDDEN_OWN __attribute__((visibility("hidden")))
#  endif
#  ifndef Q_DECL_EXPORT_OWN
#    define Q_DECL_EXPORT_OWN
#  endif
#endif
#ifndef Q_DECL_IMPORT_OWN
#  if defined(Q_OS_WIN) || defined(Q_CC_NOKIAX86) || defined(Q_CC_RVCT)
#    define Q_DECL_IMPORT_OWN __declspec(dllimport)
#  else
#    define Q_DECL_IMPORT_OWN
#  endif
#endif
#ifndef Q_DECL_HIDDEN_OWN
#  define Q_DECL_HIDDEN_OWN
#endif

#if defined(YAML_LIB)
#  define YAML_EXPORT Q_DECL_EXPORT_OWN
#elif defined(YAML_STATIC_LIB)
#  define YAML_EXPORT
#else
#  define YAML_EXPORT Q_DECL_IMPORT_OWN
#endif

#endif // YAML_GLOBAL_H
