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

#include "Lawn/DebugScenario.h"
#include <iostream>
int main()
{
    static_assert((int)SeedType::SEED_LIGHTNING_ORB_CARD==53);
    static_assert((int)SeedType::SEED_BEGHOULED_BUTTON_SHUFFLE==54);
    static_assert((int)SeedType::SEED_ZOMBIE_IMP==74);
    static_assert((int)SeedType::SEED_GLASS_ORB_CARD==75);
    static_assert((int)SeedType::NUM_SEED_TYPES==54);
    static_assert(DebugSelectablePlantCount==55);
    static_assert(DebugSelectablePlantAt(53)==53);
    static_assert(DebugSelectablePlantAt(54)==75);
    static_assert(DebugSelectablePlantAt(55)==-1);
    static_assert(DebugSelectablePlantAt(-1)==-1);
    DebugScenarioConfig c;
    if(c.mEnabled || c.mSlots!=10 || c.mCards[4]!=75) return 1;
    for(int scene=0;scene<10;scene++)
    {
        c.mScene=scene;c.Normalize();
        if(c.mScene!=scene||c.Rows()<5||c.Rows()>6) return 2;
    }
    for(int slot=1;slot<=10;slot++)
    {
        c.mSlots=slot;
        for(int index=0;index<DebugSelectablePlantCount;index++)
        {
            int card=DebugSelectablePlantAt(index);
            if(!c.SetCard(slot-1,card)||c.mCards[slot-1]!=card) return 3;
        }
        if(c.SetCard(slot,0)||c.SetCard(-1,0)) return 4;
        if(c.SetCard(slot-1,54)||c.SetCard(slot-1,74)||c.SetCard(slot-1,76))return 8;
        c.Normalize();if(c.mCards[slot-1]!=75)return 9;
    }
    for(int zombie=0;zombie<(int)ZombieType::NUM_ZOMBIE_TYPES;zombie++)
    {c.mZombie=zombie;c.Normalize();if(c.mZombie!=zombie)return 5;}
    c.mSlots=999;c.mScene=999;c.mZombie=999;c.mRow=99;c.mBatch=999;c.Normalize();
    if(c.mSlots!=10||c.mScene!=9||c.mZombie!=(int)ZombieType::NUM_ZOMBIE_TYPES-1||c.mRow!=4||c.mBatch!=50)return 6;
    c.mSlots=-5;c.mScene=-2;c.mZombie=-2;c.mRow=-3;c.mBatch=0;c.Normalize();
    if(c.mSlots!=1||c.mScene!=0||c.mZombie!=0||c.mRow!=-1||c.mBatch!=1)return 7;
    std::cout << "Sandbox: all scenes, all plant cards, all zombie types, slot and spawn bounds passed.\n";
}
