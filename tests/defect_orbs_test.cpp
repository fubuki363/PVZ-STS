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

#include "Lawn/DefectOrbs.h"
#include <iostream>
#include <type_traits>
int main()
{
    static_assert(std::is_trivially_copyable_v<DefectOrbSlots>);
    DefectOrbSlots orbs;
    if(orbs.PassiveShots()!=0) return 1;
    orbs.Initialize();
    if(DefectOrbSlots::CAPACITY!=2 || orbs.PassiveShots()!=1) return 2;
    // Repeated passive fire must leave the initial orb in its slot.
    for(int i=0;i<1000;++i) if(orbs.PassiveShots()!=1) return 3;
    if(orbs.ChannelLightning() || orbs.PassiveShots()!=2) return 4;
    if(!orbs.ChannelLightning() || orbs.PassiveShots()!=2) return 5;
    if(!orbs.EvokeLightning() || orbs.PassiveShots()!=1) return 6;
    if(!orbs.EvokeLightning() || orbs.PassiveShots()!=0 || orbs.EvokeLightning()) return 7;
    orbs.Restore(0);
    if(!orbs.mInitialized || orbs.PassiveShots()!=0) return 8;
    orbs.Restore(2);
    if(orbs.PassiveShots()!=2) return 9;
    orbs.Restore(999);
    if(orbs.PassiveShots()!=2) return 10;
    orbs.Restore(-1);
    if(orbs.PassiveShots()!=0) return 11;
    if(!IsDefectPeashooterStage(true,1,true)) return 12;
    if(IsDefectPeashooterStage(true,2,true) || IsDefectPeashooterStage(false,1,true) || IsDefectPeashooterStage(true,1,false)) return 13;
    orbs.Initialize(DefectOrbType::PLASMA);
    if(orbs.PlasmaCount()!=1 || orbs.PassiveShots()!=0) return 14;
    if(orbs.Channel(DefectOrbType::LIGHTNING)!=DefectOrbType::NONE) return 15;
    if(orbs.Channel(DefectOrbType::PLASMA)!=DefectOrbType::PLASMA || orbs.Get(0)!=DefectOrbType::LIGHTNING || orbs.Get(1)!=DefectOrbType::PLASMA) return 16;
    if(orbs.Evoke()!=DefectOrbType::LIGHTNING || orbs.Get(0)!=DefectOrbType::PLASMA || orbs.Get(1)!=DefectOrbType::NONE) return 17;
    orbs.RestoreSlots(0,2);
    if(orbs.Get(0)!=DefectOrbType::PLASMA || orbs.Get(1)!=DefectOrbType::NONE) return 18;
    orbs.RestoreSlots(99,-1);
    if(orbs.PlasmaCount()!=0 || orbs.PassiveShots()!=0 || DefectOrbSlots::EVOKE_PROJECTILES!=20) return 19;
    orbs.Initialize(DefectOrbType::GLASS);
    if(orbs.PassiveShots()!=1 || orbs.PassiveType(0)!=DefectOrbType::GLASS) return 20;
    if(orbs.Channel(DefectOrbType::LIGHTNING)!=DefectOrbType::NONE || orbs.PassiveShots()!=2 ||
        orbs.PassiveType(1)!=DefectOrbType::LIGHTNING) return 21;
    if(orbs.Channel(DefectOrbType::PLASMA)!=DefectOrbType::GLASS || orbs.PassiveShots()!=1 ||
        orbs.PlasmaCount()!=1 || orbs.PassiveType(0)!=DefectOrbType::LIGHTNING) return 22;
    orbs.RestoreSlots(3,1);
    if(orbs.Get(0)!=DefectOrbType::GLASS || orbs.PassiveShots()!=2 || orbs.PassiveType(2)!=DefectOrbType::NONE) return 23;
    std::cout << "Defect orbs: initial Lightning/Plasma, two slots, FIFO overflow, typed restore and 20-shot evoke passed.\n";
}
