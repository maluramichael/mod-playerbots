/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_DADTELEMETRYBRIDGE_H
#define PLAYERBOTS_DADTELEMETRYBRIDGE_H

#include "Config.h"
#include <string>
#include <utility>
#include <vector>

// Weakly-coupled bridge to the optional "DadTelemetry" sink provided by another
// module (mod-bot-economy). We NEVER #include that module: we only forward-declare
// its two symbols and rely on the compile define DAD_TELEMETRY_AVAILABLE, which that
// module's build injects. When the define is absent this whole bridge compiles to
// no-ops, so mod-playerbots keeps building and running with telemetry simply off.
#if defined(DAD_TELEMETRY_AVAILABLE)
namespace DadTelemetry
{
    void Emit(std::string const&, std::vector<std::pair<std::string, std::string>> const&);
    bool Enabled();
}
#endif

namespace DadTelemetryBridge
{
    // DadMode master switch (declared in mod-bot-economy's conf; read here read-only).
    inline bool DadModeEnabled() { return sConfigMgr->GetOption<bool>("DadMode.Enabled", false); }

    inline bool TelemetryEnabled()
    {
#if defined(DAD_TELEMETRY_AVAILABLE)
        return DadTelemetry::Enabled();
#else
        return false;
#endif
    }

    // Emits only when the telemetry sink is present AND DadMode is enabled.
    inline void Emit(std::string const& event, std::vector<std::pair<std::string, std::string>> const& fields)
    {
#if defined(DAD_TELEMETRY_AVAILABLE)
        if (!DadModeEnabled() || !DadTelemetry::Enabled())
            return;
        DadTelemetry::Emit(event, fields);
#else
        (void)event;
        (void)fields;
#endif
    }
}

#endif  // PLAYERBOTS_DADTELEMETRYBRIDGE_H
