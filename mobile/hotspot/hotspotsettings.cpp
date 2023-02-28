/*
    SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "hotspotsettings.h"

#include <KLocalizedString>
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(HotspotSettings, "hotspotsettings.json")

HotspotSettings::HotspotSettings(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, metaData, args)
{
}

HotspotSettings::~HotspotSettings()
{
}

#include "hotspotsettings.moc"
