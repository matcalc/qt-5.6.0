/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtwebengineglobal.h"

namespace QtWebEngineCore
{
    extern void initialize();
}

QT_BEGIN_NAMESPACE

namespace QtWebEngine {

/*!
    \namespace QtWebEngine
    \inmodule QtWebEngine
    \ingroup qtwebengine-namespaces
    \keyword QtWebEngine Namespace

    \brief Helper functions for the Qt WebEngine (Qt Quick) module

    The \l[CPP]{QtWebEngine} namespace is part of the Qt WebEngine module.
*/

/*!
    \fn QtWebEngine::initialize()

    Sets up an OpenGL Context that can be shared between processes. This has to be done after
    QGuiApplication is created, but before a Qt Quick window is created.

    This has the same effect as passing Qt::AA_ShareOpenGLContexts to the QGuiApplication
    constructor.
*/
void initialize()
{
    QtWebEngineCore::initialize();
}
} // namespace QtWebEngine

QT_END_NAMESPACE
