/*
 * Qt Creator plugin with wizards of the Sailfish OS projects.
 * Copyright © 2020 FRUCT LLC.
 */

#ifndef SAILFISHOSWIZARDS_GLOBAL_H
#define SAILFISHOSWIZARDS_GLOBAL_H

#include <QtGlobal>

#if defined(SAILFISHOSWIZARDS_LIBRARY)
#  define SAILFISHOSWIZARDSSHARED_EXPORT Q_DECL_EXPORT
#else
#  define SAILFISHOSWIZARDSSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // SAILFISHOSWIZARDS_GLOBAL_H
