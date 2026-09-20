/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AUTOVENDORTRIGGER_H
#define PLAYERBOTS_AUTOVENDORTRIGGER_H

#include "Trigger.h"

class PlayerbotAI;

// Fires while the bot should walk to / use a nearby vendor (sell junk, restock food and drink, repair).
class AutoVendorNeededTrigger : public Trigger
{
public:
    AutoVendorNeededTrigger(PlayerbotAI* botAI) : Trigger(botAI, "auto vendor needed") {}

    bool IsActive() override;
};

#endif
