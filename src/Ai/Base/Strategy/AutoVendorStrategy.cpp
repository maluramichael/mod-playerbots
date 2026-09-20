/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AutoVendorStrategy.h"
#include "Playerbots.h"

void AutoVendorStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    // Relevance sits between the loot actions (6.0 - 8.0, so looting always finishes first) and "follow" (1.0), above
    // the eat/drink actions (3.0): a vendor run is a utility errand that must beat plain following (and keeps beating
    // it on every tick of the walk) but never gets in the way of loot or of anything with a higher relevance.
    triggers.push_back(new TriggerNode("auto vendor needed", { NextAction("auto vendor", 5.5f) }));
}
