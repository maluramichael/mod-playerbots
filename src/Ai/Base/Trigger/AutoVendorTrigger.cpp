/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AutoVendorTrigger.h"
#include "AutoVendorValues.h"
#include "Playerbots.h"

namespace
{
// IsActive runs every AI tick; the need/vendor scan walks the bags and the surrounding grid cells, so it is throttled.
constexpr time_t SCAN_INTERVAL_SECONDS = 3;
}  // namespace

bool AutoVendorNeededTrigger::IsActive()
{
    AutoVendorState& state = AI_VALUE_REF(AutoVendorState, "auto vendor state");

    if (!AutoVendor::CanRun(botAI))
    {
        // Combat, map change, stay, ...: drop a trip that was in progress.
        state.active = false;
        return false;
    }

    // Trip in progress: keep the action alive on every tick (no throttling), otherwise "follow" would drag the bot
    // back towards the master between two scans. The action ends the trip itself (arrival, timeout, no progress).
    if (state.active)
        return true;

    time_t const now = time(nullptr);
    if (now < state.nextScan || now < state.cooldownUntil)
        return false;

    state.nextScan = now + SCAN_INTERVAL_SECONDS;

    AutoVendor::Needs const needs = AutoVendor::EvaluateNeeds(botAI);
    return needs.Any() && AutoVendor::FindVendor(botAI, needs) != nullptr;
}
