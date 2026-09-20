/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AutoVendorAction.h"
#include "Creature.h"
#include "Event.h"
#include "Playerbots.h"

namespace
{
// Give up on a trip that takes longer than this (unreachable vendor, bot dragged into something else, ...).
constexpr time_t MAX_TRIP_SECONDS = 45;

// Give up when the bot has not got at least one yard closer to the vendor for this long.
constexpr time_t NO_PROGRESS_SECONDS = 12;

// Stop this far (plus the vendor's combat reach) from the vendor: inside INTERACTION_DISTANCE, without hugging it.
constexpr float VENDOR_STOP_DISTANCE = 2.5f;

// A visit that changed nothing (vendor does not sell what is missing, nothing was worth selling, ...) waits this many
// cooldowns before the next trip, so a bot cannot walk to the same useless vendor over and over.
constexpr uint32 IDLE_VISIT_COOLDOWN_FACTOR = 3;
}  // namespace

bool AutoVendorAction::Execute(Event /*event*/)
{
    AutoVendorState& state = AI_VALUE_REF(AutoVendorState, "auto vendor state");
    time_t const now = time(nullptr);

    if (!state.active && !StartTrip(state, now))
        return false;

    Player* master = botAI->GetMaster();
    Creature* vendor = botAI->GetCreature(state.vendor);
    if (!vendor || !master || !AutoVendor::IsVendorUsable(bot, vendor) || master->GetMapId() != bot->GetMapId() ||
        now - state.tripStart > MAX_TRIP_SECONDS ||
        master->GetDistance(vendor) > sPlayerbotAIConfig.autoVendorRange * 2.0f)
        return GiveUp(state, now);

    // Passing no npc flag mask skips the flag test but keeps the alive / reputation / interaction distance checks.
    if (bot->GetNPCIfCanInteractWith(state.vendor, 0))
    {
        EndTrip(state, now, Shop(vendor) ? 1 : IDLE_VISIT_COOLDOWN_FACTOR);
        return true;
    }

    float const distance = bot->GetDistance(vendor);
    if (distance + 1.0f < state.bestDistance)
    {
        state.bestDistance = distance;
        state.lastProgress = now;
    }
    else if (now - state.lastProgress > NO_PROGRESS_SECONDS)
        return GiveUp(state, now);

    // Always report success while walking: returning false would let "follow" run in the same tick. A move that was
    // refused (still waiting for the previous one, momentarily unable to move) is simply retried on a later tick;
    // the progress check above ends a trip that never gets anywhere.
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL))
        return true;

    if (!MoveNear(vendor, VENDOR_STOP_DISTANCE))
        MoveTo(vendor, VENDOR_STOP_DISTANCE);

    return true;
}

bool AutoVendorAction::StartTrip(AutoVendorState& state, time_t now)
{
    AutoVendor::Needs const needs = AutoVendor::EvaluateNeeds(botAI);
    if (!needs.Any())
        return false;

    Creature* vendor = AutoVendor::FindVendor(botAI, needs);
    if (!vendor)
        return false;

    state.active = true;
    state.vendor = vendor->GetGUID();
    state.tripStart = now;
    state.lastProgress = now;
    state.bestDistance = bot->GetDistance(vendor);
    return true;
}

bool AutoVendorAction::GiveUp(AutoVendorState& state, time_t now)
{
    EndTrip(state, now, 1);

    // Returning false lets the lower priority actions (follow) act in this very tick.
    return false;
}

void AutoVendorAction::EndTrip(AutoVendorState& state, time_t now, uint32 cooldownFactor)
{
    state.active = false;
    state.vendor.Clear();
    state.cooldownUntil = now + static_cast<time_t>(sPlayerbotAIConfig.autoVendorCooldownSec) * cooldownFactor;
}

bool AutoVendorAction::Shop(Creature* vendor)
{
    AutoVendor::Needs const needs = AutoVendor::EvaluateNeeds(botAI);
    ObjectGuid const guid = vendor->GetGUID();
    uint32 const moneyBefore = bot->GetMoney();

    // Every step may fail (no money, nothing worth selling, ...) without stopping the next one. The actions already
    // tell the master about each item they sell or buy, so there is no extra summary. They are run silently so a
    // failing step does not spam error messages. Order: sell (gain gold), repair, then buy, so the purchases (which
    // may include gear upgrades) cannot eat the gold the repair bill needs.
    bool const canTrade = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_VENDOR) != nullptr;
    if (canTrade && needs.sell)
        botAI->DoSpecificAction("sell", Event("auto vendor", "vendor"), true);

    if (needs.repair && bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_REPAIR))
        botAI->DoSpecificAction("repair", Event("auto vendor"), true);

    if (canTrade)
        botAI->DoSpecificAction("buy", Event("auto vendor", "vendor"), true);

    return bot->GetMoney() != moneyBefore;
}
