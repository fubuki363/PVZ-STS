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

#ifndef __DEFECTORBS_H__
#define __DEFECTORBS_H__
#include <cstdint>
inline bool IsDefectPeashooterStage(bool theAdventure,int theLevel,bool thePeashooter)
{return theAdventure&&theLevel==1&&thePeashooter;}
enum class DefectOrbType : int32_t { NONE=0,LIGHTNING=1,PLASMA=2,GLASS=3 };
class DefectOrbSlots
{
public:
    static constexpr int32_t CAPACITY=2;
    static constexpr int32_t EVOKE_PROJECTILES=20;
    int32_t mSlots[CAPACITY]{0,0};
    bool mInitialized=false;
    void Initialize(DefectOrbType theType=DefectOrbType::LIGHTNING)
    {mSlots[0]=(int32_t)theType;mSlots[1]=0;mInitialized=true;}
    DefectOrbType Get(int i) const
    {return i>=0&&i<CAPACITY?(DefectOrbType)mSlots[i]:DefectOrbType::NONE;}
    int Count(DefectOrbType t) const
    {return (mSlots[0]==(int)t)+(mSlots[1]==(int)t);}
    int PassiveShots() const{return mInitialized?Count(DefectOrbType::LIGHTNING)+Count(DefectOrbType::GLASS):0;}
    DefectOrbType PassiveType(int index) const
    {
        for(int slot=0;slot<CAPACITY;++slot)
            if(Get(slot)==DefectOrbType::LIGHTNING || Get(slot)==DefectOrbType::GLASS)
                if(index--==0)return Get(slot);
        return DefectOrbType::NONE;
    }
    int PlasmaCount() const{return mInitialized?Count(DefectOrbType::PLASMA):0;}
    void Restore(int32_t count)
    {mSlots[0]=count>0?1:0;mSlots[1]=count>1?1:0;mInitialized=true;}
    void RestoreSlots(int32_t a,int32_t b)
    {mSlots[0]=a>=0&&a<=3?a:0;mSlots[1]=b>=0&&b<=3?b:0;if(!mSlots[0]){mSlots[0]=mSlots[1];mSlots[1]=0;}mInitialized=true;}
    DefectOrbType Channel(DefectOrbType t)
    {
        mInitialized=true;
        if(mSlots[0]==0){mSlots[0]=(int)t;return DefectOrbType::NONE;}
        if(mSlots[1]==0){mSlots[1]=(int)t;return DefectOrbType::NONE;}
        DefectOrbType evoked=(DefectOrbType)mSlots[0];mSlots[0]=mSlots[1];mSlots[1]=(int)t;return evoked;
    }
    DefectOrbType Evoke()
    {DefectOrbType t=(DefectOrbType)mSlots[0];mSlots[0]=mSlots[1];mSlots[1]=0;return t;}
    bool ChannelLightning(){return Channel(DefectOrbType::LIGHTNING)!=DefectOrbType::NONE;}
    bool EvokeLightning(){return Evoke()!=DefectOrbType::NONE;}
};
#endif
