/*
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This file is part of PvZ-Portable.
 *
 * PvZ-Portable is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PvZ-Portable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PvZ-Portable. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SANDBOXSHOOTINGCHECK_H__
#define __SANDBOXSHOOTINGCHECK_H__

#include "../../LawnApp.h"
#include "../Board.h"
#include "../Plant.h"
#include "../Projectile.h"
#include "../Zombie.h"
#include "../../Sexy.TodLib/EffectSystem.h"
#include <fstream>

// Called on an empty sandbox field once per background. Use the ordinary
// Plant::Update/Projectile::Update paths, without advancing waves or triggering
// unrelated plants, so a visually hidden first-frame ground impact is detected.
inline bool VerifyConstructShooting(LawnApp* app,int& caseCount)
{
    if(!app || !app->mBoard || !app->mDebugConfig.mEnabled)return false;
    Board* board=app->mBoard;
    DebugScenarioConfig savedConfig=app->mDebugConfig;
    uint32_t savedCounter=board->mMainCounter;
    GameScenes savedScene=app->mGameScene;
    app->mDebugConfig.mDefect=true;
    app->mDebugConfig.mFrozen=false;
    app->mDebugConfig.mOrbTool=0;
    app->mGameScene=GameScenes::SCENE_PLAYING;
    bool passed=true;
    std::ofstream report(Sexy::PathFromU8(Sexy::GetAppDataPath("shooting-verification.csv")),
        caseCount==0?std::ios::out:std::ios::app);
    if(caseCount==0)report<<"scene,mode,shots,first_update_survivors,damage,passed\n";

    for(int mode=0;mode<3;++mode)
    {
        // Column 6 lies on the flat part of both roofs. Row 0 is land on pool
        // boards; flowerpots exercise the separate roof and pot height offsets.
        Plant* pot=board->StageHasRoof()?
            board->AddPlant(6,0,SeedType::SEED_FLOWERPOT,SeedType::SEED_NONE):nullptr;
        Plant* pea=board->AddPlant(6,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
        Zombie* target=board->AddZombieInRow(ZombieType::ZOMBIE_NORMAL,0,0);
        bool casePassed=pea && target && (!board->StageHasRoof() || pot);
        int shots=0,firstUpdateSurvivors=0,damage=0;
        int expected=mode==0?1:mode==1?2:20;
        if(casePassed)
        {
            target->mPosX=pea->mX+130.0f;
            target->mX=static_cast<int>(target->mPosX);
            // Keep this unarmored target alive for all 20 independent impacts.
            target->mBodyHealth=target->mBodyMaxHealth=1000;
            int initialHealth=target->mBodyHealth;
            casePassed=pea->mDefectOrbs.PassiveShots()==1;
            if(mode>=1)pea->ChannelOrb(DefectOrbType::LIGHTNING);
            if(mode==2)pea->ChannelOrb(DefectOrbType::LIGHTNING);
            casePassed=casePassed && pea->mDefectOrbs.PassiveShots()==(mode==0?1:2) &&
                pea->mOrbBurstRemaining==(mode==2?20:0);
            // Modes 0/1 automatically find the target, start their shooting
            // animation, and fire on the normal countdown. Mode 2 suppresses
            // additional passive volleys to measure the evoke independently.
            pea->mLaunchCounter=mode==2?10000:1;
            for(int tick=0;tick<(mode==2?180:100);++tick)
            {
                ++board->mMainCounter;
                app->mEffectSystem->Update();
                pea->Update();
                Projectile* shot=nullptr;
                while(board->IterateProjectiles(shot))
                {
                    bool newborn=shot->mProjectileAge==0;
                    float initialX=shot->mPosX;
                    if(newborn)
                    {
                        ++shots;
                        casePassed=casePassed && shot->mProjectileType==ProjectileType::PROJECTILE_PEA &&
                            shot->mIsLightningPea && shot->GetProjectileDef().mDamage==20;
                    }
                    shot->Update();
                    if(newborn && !shot->mDead && shot->mPosX>initialX)++firstUpdateSurvivors;
                }
            }
            damage=initialHealth-target->mBodyHealth;
            casePassed=casePassed && shots==expected && firstUpdateSurvivors==expected &&
                damage==expected*20 && pea->mOrbBurstRemaining==0;
        }
        report<<static_cast<int>(board->mBackground)<<','<<mode<<','<<shots<<','
            <<firstUpdateSurvivors<<','<<damage<<','<<(casePassed?1:0)<<'\n';
        ++caseCount;
        passed=passed && casePassed;
        Projectile* shot=nullptr;
        while(board->IterateProjectiles(shot))shot->Die();
        if(target)target->DieNoLoot();
        if(pea)pea->Die();
        if(pot)pot->Die();
        board->ProcessDeleteQueue();
    }
    app->mDebugConfig=savedConfig;
    app->mGameScene=savedScene;
    board->mMainCounter=savedCounter;
    return passed;
}

#endif
