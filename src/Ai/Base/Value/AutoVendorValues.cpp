/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "AutoVendorValues.h"
#include "BudgetValues.h"
#include "CellImpl.h"
#include "Creature.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Map.h"
#include "Playerbots.h"
#include <limits>
#include <list>
#include <vector>

namespace
{
// Restock food/drink when fewer than this many usable pieces are left.
constexpr uint32 LOW_SUPPLY_COUNT = 10;

// Go and repair when the equipped gear is below this durability (percent).
constexpr uint8 REPAIR_BELOW_PERCENT = 90;

uint32 CountSupply(std::vector<Item*> const& items, uint32 spellCategory)
{
    uint32 count = 0;
    for (Item* item : items)
    {
        // "drink" falls back to food when the bot has no drink, so filter on the real spell category.
        ItemTemplate const* proto = item->GetTemplate();
        if (proto && proto->Spells[0].SpellCategory == spellCategory)
            count += item->GetCount();
    }

    return count;
}
}  // namespace

bool AutoVendor::CanRun(PlayerbotAI* botAI)
{
    if (!sPlayerbotAIConfig.autoVendorEnabled)
        return false;

    Player* master = botAI->GetMaster();
    if (!IsRealPlayer(master) || !master->IsInWorld())
        return false;

    Player* bot = botAI->GetBot();
    if (!bot->IsInWorld() || !bot->IsAlive() || bot->IsInCombat() || bot->IsInFlight() || bot->IsFlying() ||
        bot->IsBeingTeleported() || bot->GetTradeData())
        return false;

    if (master->GetMapId() != bot->GetMapId())
        return false;

    // Open world only: dungeons, raids, battlegrounds and arenas are left alone (mod-dungeon-clear in particular).
    Map* map = bot->GetMap();
    if (!map || map->Instanceable())
        return false;

    if (botAI->HasStrategy("stay", BOT_STATE_NON_COMBAT) || botAI->HasStrategy("guard", BOT_STATE_NON_COMBAT))
        return false;

    return true;
}

bool AutoVendor::IsVendorUsable(Player* bot, Creature* creature)
{
    if (!creature->IsInWorld() || creature->IsDuringRemoveFromWorld() || !creature->IsAlive())
        return false;

    if (!creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR) && !creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR))
        return false;

    if (creature->GetCharmerGUID() || creature->IsInCombat())
        return false;

    // Same reputation gate as Player::GetNPCIfCanInteractWith.
    if (creature->GetReactionTo(bot) <= REP_UNFRIENDLY)
        return false;

    return bot->CanSeeOrDetect(creature);
}

AutoVendor::Needs AutoVendor::EvaluateNeeds(PlayerbotAI* botAI)
{
    AiObjectContext* context = botAI->GetAiObjectContext();
    Player* bot = botAI->GetBot();
    Needs needs;

    // "durability" reads 0 for a bot without any durable item equipped, so also require a real repair bill.
    if (AI_VALUE(uint8, "durability") < REPAIR_BELOW_PERCENT && AI_VALUE(uint32, "repair cost") > 0 &&
        AI_VALUE(bool, "can repair"))
        needs.repair = true;

    needs.sell = AI_VALUE(bool, "can sell");

    if (!botAI->HasCheat(BotCheatMask::food) &&
        AI_VALUE2(uint32, "free money for", uint32(NeedMoneyFor::consumables)) > 0)
    {
        uint32 const food = CountSupply(AI_VALUE2(std::vector<Item*>, "inventory items", "food"), SPELL_CATEGORY_FOOD);

        // Mana-less classes (warrior, rogue, death knight) do not drink.
        uint32 drink = LOW_SUPPLY_COUNT;
        if (bot->GetMaxPower(POWER_MANA) > 0)
            drink = CountSupply(AI_VALUE2(std::vector<Item*>, "inventory items", "drink"), SPELL_CATEGORY_DRINK);

        needs.buy = food < LOW_SUPPLY_COUNT || drink < LOW_SUPPLY_COUNT;
    }

    return needs;
}

Creature* AutoVendor::FindVendor(PlayerbotAI* botAI, Needs const& needs)
{
    Player* bot = botAI->GetBot();
    Player* master = botAI->GetMaster();
    float const range = sPlayerbotAIConfig.autoVendorRange;
    if (!master || range <= 0.0f)
        return nullptr;

    std::list<Unit*> units;
    Acore::AnyUnitInObjectRangeCheck check(bot, range);
    Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(bot, units, check);
    Cell::VisitObjects(bot, searcher, range);

    Creature* best = nullptr;
    float bestScore = std::numeric_limits<float>::max();
    for (Unit* unit : units)
    {
        Creature* creature = unit->ToCreature();
        if (!creature || !IsVendorUsable(bot, creature))
            continue;

        if (!master->IsWithinDistInMap(creature, range) || !bot->IsWithinLOSInMap(creature))
            continue;

        bool const canTrade = creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR);
        bool const canRepair = creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR);
        bool const wantsTrade = needs.sell || needs.buy;

        // Skip NPCs that cannot serve any need; penalise the ones that only serve part of them.
        if (!(wantsTrade && canTrade) && !(needs.repair && canRepair))
            continue;

        float score = bot->GetDistance(creature);
        if (wantsTrade && !canTrade)
            score += 1000.0f;

        if (needs.repair && !canRepair)
            score += 1000.0f;

        if (score < bestScore)
        {
            bestScore = score;
            best = creature;
        }
    }

    return best;
}
