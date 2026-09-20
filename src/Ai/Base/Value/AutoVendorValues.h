/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AUTOVENDORVALUES_H
#define PLAYERBOTS_AUTOVENDORVALUES_H

#include "ObjectGuid.h"
#include "Value.h"
#include <ctime>

class Creature;
class Player;
class PlayerbotAI;

// Per-bot bookkeeping shared by the "auto vendor needed" trigger and the "auto vendor" action.
struct AutoVendorState
{
    bool active = false;        // a vendor trip is in progress
    ObjectGuid vendor;          // vendor the bot is walking to
    time_t tripStart = 0;       // when the current trip started
    time_t lastProgress = 0;    // last time the bot got measurably closer to the vendor
    float bestDistance = 0.0f;  // closest distance to the vendor seen on the current trip
    time_t nextScan = 0;        // trigger throttle: earliest time of the next (expensive) need/vendor scan
    time_t cooldownUntil = 0;   // no new trip before this time
};

class AutoVendorStateValue : public ManualSetValue<AutoVendorState>
{
public:
    AutoVendorStateValue(PlayerbotAI* botAI)
        : ManualSetValue<AutoVendorState>(botAI, AutoVendorState(), "auto vendor state")
    {
    }
};

namespace AutoVendor
{
struct Needs
{
    bool sell = false;    // sellable items (vendor trash) in the bags
    bool buy = false;     // usable food and/or drink is running low
    bool repair = false;  // gear is damaged and the bot can pay for the repair

    bool Any() const { return sell || buy || repair; }
};

// Cheap, per-tick gate: feature enabled, bot able to act, open world, real player master on the same map,
// not staying/guarding.
bool CanRun(PlayerbotAI* botAI);

// True if the creature is an alive, friendly, visible NPC the bot could sell to / buy from / repair at.
bool IsVendorUsable(Player* bot, Creature* creature);

// What the bot would like to do at a vendor right now (walks the bags, so do not call it every tick).
Needs EvaluateNeeds(PlayerbotAI* botAI);

// Nearest usable vendor within AiPlayerbot.AutoVendor.Range of both the bot and its master that can serve at least
// one of the given needs (a vendor that serves more of them is preferred over a slightly nearer one).
Creature* FindVendor(PlayerbotAI* botAI, Needs const& needs);
}  // namespace AutoVendor

#endif
