/*
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "nitrux.h"

#include <QPropertyAnimation>
#include <QVariant>

namespace Nitrux
{
class Animation : public QPropertyAnimation
{
    Q_OBJECT

public:
    //* convenience
    using Pointer = WeakPointer<Animation>;

    //* constructor
    Animation(int duration, QObject *parent)
        : QPropertyAnimation(parent)
    {
        setDuration(qBound(0, duration, 60000));
    }

    //* true if running
    [[nodiscard]] bool isRunning() const
    {
        return state() == Animation::Running;
    }

    //* restart
    void restart()
    {
        if (isRunning()) {
            stop();
        }
        start();
    }
};

}
