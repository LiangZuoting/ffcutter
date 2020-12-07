#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(FCCORE_LIB)
#  define FCCORE_EXPORT Q_DECL_EXPORT
# else
#  define FCCORE_EXPORT Q_DECL_IMPORT
# endif
#else
# define FCCORE_EXPORT
#endif
