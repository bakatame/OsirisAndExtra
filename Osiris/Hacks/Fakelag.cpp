#include "EnginePrediction.h"
#include "Fakelag.h"
#include "Tickbase.h"

#include "../SDK/Entity.h"
#include "../SDK/UserCmd.h"
#include "../SDK/NetworkChannel.h"
#include "../SDK/Localplayer.h"
#include "../SDK/Vector.h"
#include "AntiAim.h"

void Fakelag::run(const UserCmd* cmd, bool& sendPacket) noexcept
{
    if (!localPlayer || !localPlayer->isAlive())
        return;
    const auto cur_moving_flag{ AntiAim::get_moving_flag(cmd) };
    const auto netChannel = interfaces->engine->getNetworkChannel();
    if (!netChannel)
        return;
    srand(static_cast<unsigned int>(time(nullptr)));
    auto chokedPackets = config->legitAntiAim.enabled || config->fakeAngle[static_cast<int>(cur_moving_flag)].enabled ? (rand() % 2 + 2) : 0;
    if (config->tickbase.DisabledTickbase && config->tickbase.onshotFl && config->tickbase.readyFire) {
        chokedPackets = -1;
        sendPacket = true;
        config->tickbase.readyFire = false;
        return;
    }
    if (config->tickbase.DisabledTickbase && config->tickbase.onshotFl && config->tickbase.lastFireShiftTick > memory->globalVars->tickCount + chokedPackets) {
        chokedPackets = 0;
        config->tickbase.readyFire = false;
        return;
    }


    if (config->fakelag[static_cast<int>(cur_moving_flag)].enabled)
    {


        const float speed = EnginePrediction::getVelocity().length2D() >= 15.0f ? EnginePrediction::getVelocity().length2D() : 0.0f;
        switch (config->fakelag[static_cast<int>(cur_moving_flag)].mode) {
        case 0: //Static
            chokedPackets = config->fakelag[static_cast<int>(cur_moving_flag)].limit;
            break;
        case 1: //Adaptive
            chokedPackets = std::clamp(static_cast<int>(std::ceilf(64 / (speed * memory->globalVars->intervalPerTick))), 1, config->fakelag[static_cast<int>(cur_moving_flag)].limit);
            break;
        case 2: // Random
            srand(static_cast<unsigned int>(time(nullptr)));
            chokedPackets = rand() % config->fakelag[static_cast<int>(cur_moving_flag)].limit + 1;
            break;
        case 3:
            int i;
            static auto frameRate = 1.0f;
            frameRate = 0.9f * frameRate + 0.1f * memory->globalVars->absoluteFrameTime;
            srand(static_cast<unsigned int>(frameRate != 0.0f ? static_cast<int>(1 / frameRate) : 0));
            for (i = 0; i <= 30; ++i)
            {
                if (i == 29 && (memory->globalVars->tickCount % 2 == 0))
                    chokedPackets = maxUserCmdProcessTicks;
                else
                {
                    if ((rand() % (360)) - 180 < 160)
                        chokedPackets = 2;
                    else {
                        chokedPackets = 1;
                        //sendPacket = true;
                    }
                }
            }
            i = 0;
            break;
        }
    }

    chokedPackets = std::clamp(chokedPackets, 0, maxUserCmdProcessTicks - Tickbase::getTargetTickShift());
    sendPacket = netChannel->chokedPackets >= chokedPackets;
}