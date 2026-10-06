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

#ifndef __SANDBOXSTOKECHECK_H__
#define __SANDBOXSTOKECHECK_H__

#include "../../LawnApp.h"
#include "../Board.h"
#include "../Plant.h"
#include "../Projectile.h"
#include "../Zombie.h"
#include "../CursorObject.h"
#include "../SeedPacket.h"
#include "../GridItem.h"
#include "../System/SaveGame.h"
#include <fstream>
#include <string>

// Verify the real row-targeted mouse path and the planted-plant special path.
// This runs on the isolated sandbox profile and does not advance normal waves.
inline bool VerifyStokeRow(LawnApp* app,int& caseCount)
{
    if(!app || !app->mBoard || !app->mDebugConfig.mEnabled)return false;
    Board* board=app->mBoard;
    DebugScenarioConfig savedConfig=app->mDebugConfig;
    GameScenes savedScene=app->mGameScene;
    int savedSun=board->mSunMoney;
    uint32_t savedCounter=board->mMainCounter;
    int scene=static_cast<int>(board->mBackground);
    bool passed=true;
    std::ofstream report(Sexy::PathFromU8(Sexy::GetAppDataPath("stoke-verification.csv")),
        caseCount==0?std::ios::out:std::ios::app);
    if(caseCount==0)report<<"scene,case,changed,sun,passed\n";
    auto record=[&](const char* name,int changed,bool good)
    {
        report<<scene<<','<<name<<','<<changed<<','<<board->mSunMoney<<','<<(good?1:0)<<'\n';
        ++caseCount;passed=passed && good;
    };
    auto clear=[&]()
    {
        board->ClearCursor();
        Plant* plant=nullptr;while(board->IteratePlants(plant))plant->Die();
        Zombie* zombie=nullptr;while(board->IterateZombies(zombie))zombie->DieNoLoot();
        Projectile* shot=nullptr;while(board->IterateProjectiles(shot))shot->Die();
        board->ProcessDeleteQueue();
    };
    auto cards=[&](SeedType result)
    {
        app->mDebugConfig.mSlots=4;
        app->mDebugConfig.mCards[0]=static_cast<int>(result);
        app->mDebugConfig.mCards[1]=static_cast<int>(SeedType::SEED_LIGHTNING_ORB_CARD);
        app->mDebugConfig.mCards[2]=static_cast<int>(SeedType::SEED_LIGHTNING_ORB_CARD);
        app->mDebugConfig.mCards[3]=static_cast<int>(SeedType::SEED_JALAPENO);
        app->ApplyDebugCards();
        for(int slot=0;slot<4;++slot)board->mSeedBank->mSeedPackets[slot].mTimesUsed=0;
    };
    auto click=[&](int col,int row)
    {
        board->mCursorObject->mCursorType=CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
        board->mCursorObject->mType=SeedType::SEED_JALAPENO;
        board->mCursorObject->mImitaterType=SeedType::SEED_NONE;
        board->mCursorObject->mSeedBankIndex=3;
        board->MouseDownWithPlant(board->GridToPixelX(col,row)+35,board->GridToPixelY(col,row)+35,1);
    };
    auto count=[&](int row,SeedType type)
    {
        int result=0;Plant* plant=nullptr;
        while(board->IteratePlants(plant))if(plant->mRow==row && plant->mSeedType==type)++result;
        return result;
    };
    auto addSupport=[&](int col,int row)->Plant*
    {
        if(board->StageHasRoof())return board->AddPlant(col,row,SeedType::SEED_FLOWERPOT,SeedType::SEED_NONE);
        if(board->IsPoolSquare(col,row))return board->AddPlant(col,row,SeedType::SEED_LILYPAD,SeedType::SEED_NONE);
        return nullptr;
    };

    app->mGameScene=GameScenes::SCENE_PLAYING;
    app->mDebugConfig.mInfiniteSun=false;
    app->mDebugConfig.mNoCooldown=false;
    app->mDebugConfig.mFrozen=false;
    app->mDebugConfig.mOrbTool=0;
    app->mDebugConfig.mDefect=true;
    app->mDebugConfig.mFreePlacement=true;
    clear();cards(SeedType::SEED_WALLNUT);
    // The nightly board may start with graves. Remove these sandbox fixtures
    // before constructing a full row that would be legal under normal rules.
    GridItem* obstacle=nullptr;
    while(board->IterateGridItems(obstacle))
        if(obstacle->mGridItemType==GridItemType::GRIDITEM_GRAVESTONE ||
            obstacle->mGridItemType==GridItemType::GRIDITEM_CRATER)obstacle->GridItemDie();
    board->ProcessDeleteQueue();

    // A fully occupied row proves this is a row skill, not empty-cell planting.
    // On the two six-row pool boards, target the last row through the mouse path.
    int row=board->StageHas6Rows()?5:0;
    int otherRow=row==0?1:0;
    for(int col=0;col<MAX_GRID_SIZE_X;++col)
    {
        addSupport(col,row);
        board->AddPlant(col,row,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    }
    addSupport(1,otherRow);
    Plant* untouched=board->AddPlant(1,otherRow,SeedType::SEED_SUNFLOWER,SeedType::SEED_NONE);
    unsigned int untouchedId=board->mPlants.DataArrayGetID(untouched);
    int untouchedHealth=untouched->mPlantHealth;
    int untouchedOrb=untouched->mDefectOrbs.PlasmaCount();
    board->mSunMoney=250;
    click(4,row);
    SeedPacket& chili=board->mSeedBank->mSeedPackets[3];
    bool fullRow=count(row,SeedType::SEED_WALLNUT)==MAX_GRID_SIZE_X &&
        count(row,SeedType::SEED_PEASHOOTER)==0 && board->mSunMoney==125 &&
        chili.mRefreshing && chili.mRefreshTime==Plant::GetRefreshTime(SeedType::SEED_JALAPENO,SeedType::SEED_NONE) &&
        chili.mTimesUsed==1 && board->mCursorObject->mCursorType==CursorType::CURSOR_TYPE_NORMAL;
    Plant* stillThere=board->mPlants.DataArrayTryToGet(untouchedId);
    fullRow=fullRow && stillThere && !stillThere->mDead && stillThere->mSeedType==SeedType::SEED_SUNFLOWER &&
        stillThere->mPlantHealth==untouchedHealth && stillThere->mDefectOrbs.PlasmaCount()==untouchedOrb;
    record("paid_full_row_and_cooldown",count(row,SeedType::SEED_WALLNUT),fullRow);

    if(scene==0)
    {
        board->ProcessDeleteQueue();
        std::string path=Sexy::GetAppDataPath("stoke-roundtrip.dat");
        bool saved=LawnSaveGame(board,path);
        if(saved)
        {
            app->MakeNewBoard();board=app->mBoard;
            saved=board && board->LoadGame(path);
            app->mGameScene=GameScenes::SCENE_PLAYING;
        }
        bool roundtrip=saved && board && count(row,SeedType::SEED_WALLNUT)==MAX_GRID_SIZE_X &&
            count(otherRow,SeedType::SEED_SUNFLOWER)==1 && board->mSunMoney==125 &&
            board->mSeedBank->mSeedPackets[3].mPacketType==SeedType::SEED_JALAPENO &&
            board->mSeedBank->mSeedPackets[3].mRefreshing;
        record("transformed_save_roundtrip",board?count(row,SeedType::SEED_WALLNUT):0,roundtrip);
        if(!board){app->mDebugConfig=savedConfig;app->mGameScene=savedScene;return false;}
    }

    clear();cards(SeedType::SEED_WALLNUT);
    addSupport(1,0);
    Plant* poor=board->AddPlant(1,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    unsigned int poorId=board->mPlants.DataArrayGetID(poor);
    board->mSunMoney=124;click(1,0);
    poor=board->mPlants.DataArrayTryToGet(poorId);
    record("insufficient_sun",count(0,SeedType::SEED_WALLNUT),poor && !poor->mDead &&
        poor->mSeedType==SeedType::SEED_PEASHOOTER && board->mSunMoney==124 &&
        !board->mSeedBank->mSeedPackets[3].mRefreshing &&
        board->mSeedBank->mSeedPackets[3].mTimesUsed==0);

    // No actual plant card remains: both skill cards must be excluded, and an
    // unusable cast must neither spend sunlight nor start the card cooldown.
    clear();cards(SeedType::SEED_JALAPENO);
    addSupport(1,0);
    Plant* noChoice=board->AddPlant(1,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    unsigned int noChoiceId=board->mPlants.DataArrayGetID(noChoice);
    board->mSunMoney=250;click(1,0);
    noChoice=board->mPlants.DataArrayTryToGet(noChoiceId);
    record("skills_excluded_no_charge",count(0,SeedType::SEED_PEASHOOTER),noChoice && !noChoice->mDead &&
        noChoice->mDefectOrbs.PassiveShots()==1 && board->mSunMoney==250 &&
        !board->mSeedBank->mSeedPackets[3].mRefreshing &&
        board->mSeedBank->mSeedPackets[3].mTimesUsed==0);

    // A directly added pepper exercises DoSpecial, as old saves and other game
    // modes can instantiate the plant without entering MouseDownWithPlant.
    clear();cards(SeedType::SEED_WALLNUT);
    addSupport(1,0);addSupport(4,0);
    board->AddPlant(1,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    Plant* direct=board->AddPlant(4,0,SeedType::SEED_JALAPENO,SeedType::SEED_NONE);
    Zombie* target=board->AddZombieInRow(ZombieType::ZOMBIE_NORMAL,0,0);
    int initialHealth=target?target->mBodyHealth:0;
    board->mSunMoney=250;
    direct->DoSpecial();
    record("planted_special_no_zombie_burn",count(0,SeedType::SEED_WALLNUT),
        direct->mDead && count(0,SeedType::SEED_WALLNUT)>=1 &&
        target && !target->mDead && target->mBodyHealth==initialHealth && board->mSunMoney==250);

    // Terrain checks run without the sandbox placement override. Supporting
    // plants in an occupied cell must survive while the plant above transforms.
    if(board->StageHasRoof() || board->StageHasPool())
    {
        clear();cards(SeedType::SEED_WALLNUT);
        app->mDebugConfig.mFreePlacement=false;
        int supportRow=board->StageHasRoof()?0:2;
        Plant* support=addSupport(1,supportRow);
        unsigned int supportId=support?board->mPlants.DataArrayGetID(support):0;
        SeedType supportType=board->StageHasRoof()?SeedType::SEED_FLOWERPOT:SeedType::SEED_LILYPAD;
        board->AddPlant(1,supportRow,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
        board->mSunMoney=250;click(1,supportRow);
        support=board->mPlants.DataArrayTryToGet(supportId);
        record(board->StageHasRoof()?"roof_pot_retained":"pool_lily_retained",count(supportRow,SeedType::SEED_WALLNUT),
            support && !support->mDead && support->mSeedType==supportType &&
            count(supportRow,SeedType::SEED_WALLNUT)==1 && board->mSunMoney==125);
    }

    // Purple-card transforms may bypass their base-plant requirement, but
    // they must not bypass an occupied normal layer while transforming a pot,
    // lily or pumpkin. A single cell with all applicable layers exposes that.
    clear();cards(SeedType::SEED_GATLINGPEA);
    app->mDebugConfig.mFreePlacement=false;
    int layeredRow=board->StageHasPool()?2:0;
    Plant* base=addSupport(1,layeredRow);
    unsigned int baseId=base?board->mPlants.DataArrayGetID(base):0;
    board->AddPlant(1,layeredRow,SeedType::SEED_REPEATER,SeedType::SEED_NONE);
    Plant* pumpkin=board->AddPlant(1,layeredRow,SeedType::SEED_PUMPKINSHELL,SeedType::SEED_NONE);
    unsigned int pumpkinId=board->mPlants.DataArrayGetID(pumpkin);
    board->mSunMoney=250;click(1,layeredRow);
    pumpkin=board->mPlants.DataArrayTryToGet(pumpkinId);
    base=baseId?board->mPlants.DataArrayTryToGet(baseId):nullptr;
    record("purple_upgrade_retains_other_layers",count(layeredRow,SeedType::SEED_GATLINGPEA),
        count(layeredRow,SeedType::SEED_GATLINGPEA)==1 &&
        count(layeredRow,SeedType::SEED_REPEATER)==0 && pumpkin && !pumpkin->mDead &&
        (!baseId || (base && !base->mDead)) && board->mSunMoney==125);

    // A legal plant card can still be unusable for the clicked terrain.
    clear();cards(SeedType::SEED_TANGLEKELP);
    app->mDebugConfig.mFreePlacement=false;
    addSupport(1,0);
    Plant* land=board->AddPlant(1,0,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
    unsigned int landId=board->mPlants.DataArrayGetID(land);
    board->mSunMoney=250;click(1,0);
    land=board->mPlants.DataArrayTryToGet(landId);
    record("no_legal_terrain_candidate",count(0,SeedType::SEED_TANGLEKELP),land && !land->mDead &&
        land->mSeedType==SeedType::SEED_PEASHOOTER && board->mSunMoney==250 &&
        !board->mSeedBank->mSeedPackets[3].mRefreshing);

    clear();
    app->mDebugConfig=savedConfig;
    app->mGameScene=savedScene;
    board->mSunMoney=savedSun;
    board->mMainCounter=savedCounter;
    app->ApplyDebugCards();
    return passed;
}

#endif
