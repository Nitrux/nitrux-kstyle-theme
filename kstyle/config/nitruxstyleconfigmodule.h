/*
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "nitruxstyleconfig.h"

#include <KCModule>

namespace Nitrux
{
//* configuration module
class ConfigurationModule : public KCModule
{
    Q_OBJECT

public:
    ConfigurationModule(QObject *parent, const KPluginMetaData &data);

public Q_SLOTS:

    void defaults() override;
    void load() override;
    void save() override;

private:
    //* configuration
    StyleConfig *m_config;
};

}
