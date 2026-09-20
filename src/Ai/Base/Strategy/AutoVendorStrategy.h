/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AUTOVENDORSTRATEGY_H
#define PLAYERBOTS_AUTOVENDORSTRATEGY_H

#include "Strategy.h"

class PlayerbotAI;

// Bots with a real player master walk to a vendor near the group to sell junk, restock food/drink and repair.
class AutoVendorStrategy : public Strategy
{
public:
    AutoVendorStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    std::string const getName() override { return "auto vendor"; }
};

#endif
