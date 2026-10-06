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

#ifndef __SANDBOXGLASSCHECK_H__
#define __SANDBOXGLASSCHECK_H__

#include "../../LawnApp.h"
#include "../Board.h"
#include "../Plant.h"
#include "../Projectile.h"
#include "../Zombie.h"
#include "../CursorObject.h"
#include "../SeedPacket.h"
#include "../System/SaveGame.h"
#include "../../Sexy.TodLib/EffectSystem.h"
#include <array>
#include <fstream>
#include <string>
#include <vector>

// Called on an empty sandbox field for every background. Native automatic fire,
// projectile movement/collision and save serialization exercise the same paths
// as normal play; stationary high-health targets make each impact countable.
inline bool VerifyGlassOrbs(LawnApp* app,int& caseCount)
{
    if(!app || !app->mBoard || !app->mDebugConfig.mEnabled)return false;
    Board* board=app->mBoard;
    DebugScenarioConfig savedConfig=app->mDebugConfig;
    GameScenes savedScene=app->mGameScene;
    uint32_t savedCounter=board->mMainCounter;
    int savedSun=board->mSunMoney;
    int scene=static_cast<int>(board->mBackground);
    bool passed=true;
    std::ofstream report(Sexy::PathFromU8(Sexy::GetAppDataPath("glass-verification.csv")),
        caseCount==0?std::ios::out:std::ios::app);
    if(caseCount==0)report<<"scene,case,shots,damage_first,damage_second,damage_third,damage_fourth,passed\n";
    auto record=[&](const char* name,int shots,const std::array<Zombie*,4>& targets,bool good)
    {
        report<<scene<<','<<name<<','<<shots;
        for(Zombie* target:targets)report<<','<<(target?1000-target->mBodyHealth:0);
        report<<','<<(good?1:0)<<'\n';
        report.flush();++caseCount;passed=passed && good;
    };
    auto clear=[&]()
    {
        board->ClearCursor();
        Plant* plant=nullptr;while(board->IteratePlants(plant))plant->Die();
        Zombie* zombie=nullptr;while(board->IterateZombies(zombie))zombie->DieNoLoot();
        Projectile* shot=nullptr;while(board->IterateProjectiles(shot))shot->Die();
        board->ProcessDeleteQueue();
    };
    auto addSupport=[&](int col)
    {
        if(board->StageHasRoof())board->AddPlant(col,0,SeedType::SEED_FLOWERPOT,SeedType::SEED_NONE);
    };
    auto addPea=[&]()->Plant*
    {
        // Columns 5 and 6 are on the flat part of both roofs; row 0 is land.
        addSupport(5);
        return board->AddPlant(5,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    };
    auto addTargets=[&](Plant* pea,ZombieType firstType=ZombieType::ZOMBIE_NORMAL)
    {
        std::array<Zombie*,4> targets{};
        if(!pea)return targets;
        for(int i=0;i<4;++i)
        {
            targets[i]=board->AddZombieInRow(i==0?firstType:ZombieType::ZOMBIE_NORMAL,0,0);
            if(targets[i])
            {
                targets[i]->mPosX=pea->mX+90.0f+i*55.0f;
                targets[i]->mX=static_cast<int>(targets[i]->mPosX);
                targets[i]->mBodyHealth=targets[i]->mBodyMaxHealth=1000;
            }
        }
        return targets;
    };
    auto allTargets=[&](const std::array<Zombie*,4>& targets)
    {for(Zombie* target:targets)if(!target)return false;return true;};
    auto damageIs=[&](const std::array<Zombie*,4>& targets,int first,int second,int third,int fourth)
    {
        const int expected[]={first,second,third,fourth};
        for(int i=0;i<4;++i)if(!targets[i] || 1000-targets[i]->mBodyHealth!=expected[i])return false;
        return true;
    };
    auto updateShots=[&](int& shots,std::vector<int>& types,bool& good)
    {
        Projectile* shot=nullptr;
        while(board->IterateProjectiles(shot))
        {
            bool newborn=shot->mProjectileAge==0;
            float originalX=shot->mPosX;
            if(newborn)
            {
                ++shots;
                int type=shot->mIsGlassPea?3:shot->mIsLightningPea?1:0;
                types.push_back(type);
                good=good && shot->mProjectileType==ProjectileType::PROJECTILE_PEA &&
                    (type==1 || type==3) && !(shot->mIsGlassPea && shot->mIsLightningPea) &&
                    shot->GetProjectileDef().mDamage==(type==3?10:20);
            }
            shot->Update();
            if(newborn)good=good && !shot->mDead && shot->mPosX>originalX;
        }
    };
    auto playTicks=[&](Plant* pea,int ticks,int& shots,std::vector<int>& types,bool& good)
    {
        for(int tick=0;tick<ticks;++tick)
        {
            ++board->mMainCounter;app->mEffectSystem->Update();
            if(pea)pea->Update();
            updateShots(shots,types,good);
        }
    };

    app->mGameScene=GameScenes::SCENE_PLAYING;
    app->mDebugConfig.mDefect=true;
    app->mDebugConfig.mFrozen=false;
    app->mDebugConfig.mOrbTool=0;
    app->mDebugConfig.mInfiniteSun=false;
    app->mDebugConfig.mNoCooldown=false;
    app->mDebugConfig.mFreePlacement=true;
    app->mDebugConfig.mSlots=5;
    app->mDebugConfig.mCards[4]=static_cast<int>(SeedType::SEED_GLASS_ORB_CARD);
    app->ApplyDebugCards();

    // The fifth packet must charge the same 25 sunlight as Lightning, channel
    // into the existing host, and begin the ordinary two-second cooldown.
    clear();
    Plant* pea=addPea();
    board->mSunMoney=100;
    SeedPacket& card=board->mSeedBank->mSeedPackets[4];
    card.mTimesUsed=0;
    auto clickGlass=[&]()
    {
        board->mCursorObject->mCursorType=CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
        board->mCursorObject->mType=SeedType::SEED_GLASS_ORB_CARD;
        board->mCursorObject->mImitaterType=SeedType::SEED_NONE;
        board->mCursorObject->mSeedBankIndex=4;
        board->MouseDownWithPlant(pea->mX+35,pea->mY+35,1);
    };
    bool good=pea && card.mPacketType==SeedType::SEED_GLASS_ORB_CARD &&
        board->GetCurrentPlantCost(SeedType::SEED_GLASS_ORB_CARD,SeedType::SEED_NONE)==25;
    if(pea)clickGlass();
    good=good && board->mSunMoney==75 && pea->mDefectOrbs.Get(0)==DefectOrbType::LIGHTNING &&
        pea->mDefectOrbs.Get(1)==DefectOrbType::GLASS && pea->mOrbBurstRemaining==0 &&
        card.mRefreshing && card.mRefreshTime==200 && card.mTimesUsed==1 &&
        board->mCursorObject->mCursorType==CursorType::CURSOR_TYPE_NORMAL;
    record("fifth_card_paid_channel",0,{},good);

    clear();pea=addPea();app->ApplyDebugCards();
    board->mSunMoney=24;
    board->mSeedBank->mSeedPackets[4].mTimesUsed=0;
    board->mSeedBank->mSeedPackets[4].mRefreshing=false;
    if(pea)clickGlass();
    record("insufficient_sun",0,{},pea && board->mSunMoney==24 &&
        pea->mDefectOrbs.Get(0)==DefectOrbType::LIGHTNING && pea->mDefectOrbs.Get(1)==DefectOrbType::NONE &&
        board->mSeedBank->mSeedPackets[4].mTimesUsed==0 && !board->mSeedBank->mSeedPackets[4].mRefreshing);

    for(int mode=0;mode<3;++mode)
    {
        clear();pea=addPea();
        auto targets=addTargets(pea);
        good=pea && allTargets(targets);
        int shots=0;std::vector<int> types;
        if(good)
        {
            pea->mDefectOrbs.RestoreSlots(mode==1?1:3,mode==1?3:0);
            if(mode==2)pea->EvokeOrb();
            pea->mLaunchCounter=mode==2?10000:1;
            playTicks(pea,mode==2?220:100,shots,types,good);
            int expected=mode==2?20:mode==1?2:1;
            good=good && shots==expected && pea->mOrbBurstRemaining==0 &&
                damageIs(targets,mode==2?200:mode==1?30:10,mode==2?200:10,mode==2?200:10,0);
            if(mode==1)
            {
                // Reused DataArray slots are iterated in storage order, which
                // need not match the creation order of simultaneous shots.
                good=good && std::count(types.begin(),types.end(),1)==1 &&
                    std::count(types.begin(),types.end(),3)==1;
            }
            else for(int type:types)good=good && type==3;
        }
        record(mode==0?"glass_automatic_three_targets":mode==1?"mixed_passive_flags_and_damage":"glass_evoke_twenty_three_targets",shots,targets,good);
    }

    // Leave a projectile inside a single enemy's rectangle after the first
    // hit; both repeated impact calls and collision scans must ignore it.
    clear();pea=addPea();auto targets=addTargets(pea);
    for(int i=1;i<4;++i){if(targets[i])targets[i]->DieNoLoot();targets[i]=nullptr;}
    good=pea && targets[0];int shots=0;std::vector<int> types;
    Projectile* repeated=nullptr;
    if(good)
    {
        pea->mDefectOrbs.RestoreSlots(3,0);pea->Fire(targets[0],0,PlantWeapon::WEAPON_PRIMARY);
        board->IterateProjectiles(repeated);
        for(int tick=0;tick<100 && repeated && repeated->mGlassHitCount==0;++tick)repeated->Update();
        good=repeated && !repeated->mDead && repeated->mGlassHitCount==1 && targets[0]->mBodyHealth==990;
        if(repeated)for(int tick=0;tick<20;++tick)
        {repeated->DoImpact(targets[0]);repeated->CheckForCollision();}
        good=good && !repeated->mDead && repeated->mGlassHitCount==1 && targets[0]->mBodyHealth==990;
    }
    record("overlap_same_enemy_once",1,targets,good);

    clear();pea=addPea();targets=addTargets(pea);addSupport(6);
    Plant* torch=board->AddPlant(6,0,SeedType::SEED_TORCHWOOD,SeedType::SEED_NONE);
    good=pea && torch && allTargets(targets);shots=0;types.clear();
    if(good)
    {
        pea->mDefectOrbs.RestoreSlots(3,0);pea->mLaunchCounter=10000;
        pea->Fire(targets[0],0,PlantWeapon::WEAPON_PRIMARY);
        Projectile* throughTorch=nullptr;board->IterateProjectiles(throughTorch);
        // Explicit conversion also covers direct special/game-mode callers.
        if(throughTorch)throughTorch->ConvertToFireball(6);
        playTicks(nullptr,100,shots,types,good);
        good=good && shots==1 && throughTorch && throughTorch->mProjectileType==ProjectileType::PROJECTILE_PEA &&
            throughTorch->mIsGlassPea && throughTorch->GetProjectileDef().mDamage==10 &&
            damageIs(targets,10,10,10,0);
    }
    record("torch_preserves_glass",shots,targets,good);

    for(int shield=0;shield<2;++shield)
    {
        clear();pea=addPea();targets=addTargets(pea,shield?ZombieType::ZOMBIE_DOOR:ZombieType::ZOMBIE_TRAFFIC_CONE);
        good=pea && allTargets(targets);shots=0;types.clear();
        int armorBefore=good?(shield?targets[0]->mShieldHealth:targets[0]->mHelmHealth):0;
        if(good)
        {
            pea->mDefectOrbs.RestoreSlots(3,0);pea->mLaunchCounter=1;
            playTicks(pea,100,shots,types,good);
            int armorAfter=shield?targets[0]->mShieldHealth:targets[0]->mHelmHealth;
            good=good && shots==1 && armorBefore-armorAfter==10 && damageIs(targets,0,10,10,0);
        }
        record(shield?"shield_damage_no_bypass":"helmet_absorbs_glass",shots,targets,good);
    }

    if(scene==0)
    {
        clear();pea=addPea();targets=addTargets(pea);
        good=pea && allTargets(targets);shots=0;types.clear();
        Projectile* partial=nullptr;
        std::array<ZombieID,4> targetIds{};unsigned int shotId=0;
        if(good)
        {
            for(int i=0;i<4;++i)targetIds[i]=board->ZombieGetID(targets[i]);
            pea->mDefectOrbs.RestoreSlots(3,0);pea->Fire(targets[0],0,PlantWeapon::WEAPON_PRIMARY);
            board->IterateProjectiles(partial);
            if(partial)shotId=board->mProjectiles.DataArrayGetID(partial);
            for(int tick=0;tick<100 && partial && partial->mGlassHitCount<2;++tick)partial->Update();
            good=partial && !partial->mDead && partial->mGlassHitCount==2 && damageIs(targets,10,10,0,0);
            std::string path=Sexy::GetAppDataPath("glass-hit-roundtrip.dat");
            bool saved=good && LawnSaveGame(board,path);
            if(saved)
            {
                app->MakeNewBoard();board=app->mBoard;
                saved=board && board->LoadGame(path);app->mGameScene=GameScenes::SCENE_PLAYING;
            }
            good=good && saved;
            if(saved)
            {
                partial=board->mProjectiles.DataArrayTryToGet(shotId);
                for(int i=0;i<4;++i)targets[i]=board->ZombieTryToGet(targetIds[i]);
                good=good && partial && partial->mIsGlassPea && !partial->mIsLightningPea &&
                    partial->mGlassHitCount==2 && partial->mGlassHitZombieIDs[0]==targetIds[0] &&
                    partial->mGlassHitZombieIDs[1]==targetIds[1];
                if(partial)for(int tick=0;tick<100 && !partial->mDead;++tick)partial->Update();
                good=good && partial && partial->mDead && partial->mGlassHitCount==3 && damageIs(targets,10,10,10,0);
            }
            else targets={};
        }
        record("two_hits_save_then_third_dies",1,targets,good);
        if(!board){app->mDebugConfig=savedConfig;app->mGameScene=savedScene;return false;}

        // Evoke Lightning then Glass immediately. Saving after seven shots must
        // retain the current batch's thirteen shots before the twenty Glass.
        clear();pea=addPea();good=pea;shots=0;types.clear();
        unsigned int plantId=pea?board->mPlants.DataArrayGetID(pea):0;
        if(pea)
        {
            pea->mDefectOrbs.RestoreSlots(1,3);pea->EvokeOrb();pea->EvokeOrb();
            pea->mLaunchCounter=10000;
            for(int tick=0;tick<100 && shots<7;++tick)
            {
                ++board->mMainCounter;pea->UpdateOrbBurst();updateShots(shots,types,good);
            }
            good=good && shots==7 && pea->mOrbBurstRemaining==33 && pea->mOrbBurstBatchRemaining==13 &&
                pea->mOrbBurstType==1 && pea->mOrbBurstQueueCount==1 && pea->mOrbBurstQueue[0]==3;
            int timer=pea->mOrbBurstTimer;
            std::string path=Sexy::GetAppDataPath("glass-burst-roundtrip.dat");
            bool saved=good && LawnSaveGame(board,path);
            if(saved)
            {
                app->MakeNewBoard();board=app->mBoard;
                saved=board && board->LoadGame(path);app->mGameScene=GameScenes::SCENE_PLAYING;
            }
            good=good && saved;
            if(saved)
            {
                pea=board->mPlants.DataArrayTryToGet(plantId);
                good=good && pea && pea->mOrbBurstRemaining==33 && pea->mOrbBurstBatchRemaining==13 &&
                    pea->mOrbBurstType==1 && pea->mOrbBurstQueueCount==1 && pea->mOrbBurstQueue[0]==3 &&
                    pea->mOrbBurstTimer==timer;
                if(pea)for(int tick=0;tick<300;++tick)
                {
                    ++board->mMainCounter;pea->UpdateOrbBurst();updateShots(shots,types,good);
                }
                good=good && pea && shots==40 && pea->mOrbBurstRemaining==0 && pea->mOrbBurstQueueCount==0;
                for(int i=0;i<static_cast<int>(types.size());++i)good=good && types[i]==(i<20?1:3);
            }
        }
        record("mixed_fifo_save_after_seven",shots,{},good);
        if(!board){app->mDebugConfig=savedConfig;app->mGameScene=savedScene;return false;}
    }

    clear();
    app->mDebugConfig=savedConfig;
    app->mGameScene=savedScene;
    board->mMainCounter=savedCounter;
    board->mSunMoney=savedSun;
    app->ApplyDebugCards();
    return passed;
}

#endif
