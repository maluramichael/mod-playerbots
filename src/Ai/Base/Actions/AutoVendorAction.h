/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_AUTOVENDORACTION_H
#define PLAYERBOTS_AUTOVENDORACTION_H

#include "AutoVendorValues.h"
#include "MovementActions.h"

class Creature;
class PlayerbotAI;

// Multi-tick vendor run: walk to a nearby vendor, sell junk, buy supplies, repair, then hand back to "follow".
// The progress is kept in the "auto vendor state" value; the "auto vendor needed" trigger keeps the action
// scheduled every tick while a trip is active.
class AutoVendorAction : public MovementAction
{
public:
    AutoVendorAction(PlayerbotAI* botAI) : MovementAction(botAI, "auto vendor") {}

    bool Execute(Event event) override;

private:
    bool StartTrip(AutoVendorState& state, time_t now);
    bool GiveUp(AutoVendorState& state, time_t now);
    void EndTrip(AutoVendorState& state, time_t now, uint32 cooldownFactor);
    bool Shop(Creature* vendor);
};

#endif
