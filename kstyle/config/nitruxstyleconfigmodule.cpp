/*
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nitruxstyleconfigmodule.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(Nitrux::ConfigurationModule, "nitruxstyleconfig.json")

#include "nitruxstyleconfigmodule.moc"

namespace Nitrux
{
//_______________________________________________________________________
ConfigurationModule::ConfigurationModule(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    widget()->setLayout(new QVBoxLayout);
    widget()->layout()->addWidget(m_config = new StyleConfig(widget()));
    connect(m_config, &StyleConfig::changed, this, &KCModule::setNeedsSave);
}

//_______________________________________________________________________
void ConfigurationModule::defaults()
{
    m_config->defaults();
    KCModule::defaults();
}

//_______________________________________________________________________
void ConfigurationModule::load()
{
    m_config->load();
    KCModule::load();
}

//_______________________________________________________________________
void ConfigurationModule::save()
{
    m_config->save();
    KCModule::save();
}

}
