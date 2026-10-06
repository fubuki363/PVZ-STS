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

#ifndef __DEBUGSCENARIO_H__
#define __DEBUGSCENARIO_H__
#include "../ConstEnums.h"
#include <array>
#include <algorithm>
constexpr int DebugSelectablePlantCount = static_cast<int>(SeedType::NUM_SEED_TYPES) + 1;
constexpr int DebugSelectablePlantAt(int theIndex)
{
    return theIndex < 0 || theIndex >= DebugSelectablePlantCount ? static_cast<int>(SeedType::SEED_NONE) :
        theIndex == static_cast<int>(SeedType::NUM_SEED_TYPES) ? static_cast<int>(SeedType::SEED_GLASS_ORB_CARD) : theIndex;
}
constexpr bool DebugIsSelectablePlant(int theSeed)
{
    return (theSeed >= 0 && theSeed < static_cast<int>(SeedType::NUM_SEED_TYPES)) ||
        theSeed == static_cast<int>(SeedType::SEED_GLASS_ORB_CARD);
}
struct DebugScenarioConfig
{
    bool mEnabled = false;
    int mScene = 0;
    int mSlots = 10;
    std::array<int,10> mCards{0,1,(int)SeedType::SEED_LIGHTNING_ORB_CARD,(int)SeedType::SEED_JALAPENO,(int)SeedType::SEED_GLASS_ORB_CARD,5,6,7,8,9};
    int mZombie = 0;
    int mRow = -1;
    int mBatch = 1;
    int mImitater = 0;
    bool mInfiniteSun = true;
    bool mNoCooldown = true;
    bool mFreePlacement = true;
    bool mFrozen = false;
    bool mDefect = true;
    bool mNoLoss = true;
    bool mWakeMushrooms = true;
    int mOrbTool = 0;
    int Rows() const { return mScene == 2 || mScene == 3 || mScene == 8 ? 6 : 5; }
    void Normalize()
    {
        mScene=std::clamp(mScene,0,9);mSlots=std::clamp(mSlots,1,10);
        mZombie=std::clamp(mZombie,0,(int)ZombieType::NUM_ZOMBIE_TYPES-1);
        mRow=std::clamp(mRow,-1,Rows()-1);mBatch=std::clamp(mBatch,1,50);
        mImitater=std::clamp(mImitater,0,(int)SeedType::SEED_IMITATER-1);
        for(int& card:mCards)if(!DebugIsSelectablePlant(card))card=std::clamp(card,0,(int)SeedType::NUM_SEED_TYPES-1);
    }
    bool SetCard(int theSlot,int theSeed)
    {
        if(theSlot<0||theSlot>=mSlots||!DebugIsSelectablePlant(theSeed))return false;
        mCards[theSlot]=theSeed;return true;
    }
};
inline const char* DebugSceneName(int i)
{
    static const char* names[]={"白天","夜晚","泳池","浓雾","屋顶","僵王屋顶","蘑菇园","温室","水族馆","智慧树"};
    return names[std::clamp(i,0,9)];
}
inline const char* DebugZombieName(int i)
{
    static const char* names[]={"普通僵尸","旗帜僵尸","路障僵尸","撑杆僵尸","铁桶僵尸","读报僵尸","铁门僵尸","橄榄球僵尸","舞王僵尸","伴舞僵尸","泳圈僵尸","潜水僵尸","冰车僵尸","雪橇僵尸","海豚骑士","小丑僵尸","气球僵尸","矿工僵尸","跳跳僵尸","雪人僵尸","蹦极僵尸","扶梯僵尸","投石车僵尸","巨人僵尸","小鬼僵尸","僵王博士","豌豆僵尸","坚果僵尸","辣椒僵尸","机枪僵尸","窝瓜僵尸","高坚果僵尸","红眼巨人"};
    return names[std::clamp(i,0,(int)ZombieType::NUM_ZOMBIE_TYPES-1)];
}
#endif
