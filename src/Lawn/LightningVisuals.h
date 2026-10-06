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

#ifndef __LIGHTNINGVISUALS_H__
#define __LIGHTNINGVISUALS_H__
#include "graphics/Graphics.h"
#include <cmath>

inline void DrawLightningDisc(Sexy::Graphics* g, int theX, int theY, float theRadius, const Sexy::Color& theColor)
{
    Sexy::Point aPoints[20];
    for (int i = 0; i < 20; ++i)
    {
        float aAngle = i * 6.28318530718f / 20.0f;
        aPoints[i] = Sexy::Point(theX + (int)std::round(std::cos(aAngle) * theRadius),
                                theY + (int)std::round(std::sin(aAngle) * theRadius));
    }
    g->SetColor(theColor);
    g->PolyFill(aPoints, 20, true);
}
inline void DrawLightningBolt(Sexy::Graphics* g, int theX, int theY, int theSize)
{
    g->SetColor(Sexy::Color(255,255,220));
    g->DrawLineAA(theX + theSize/3, theY - theSize, theX - theSize/3, theY);
    g->DrawLineAA(theX - theSize/3, theY, theX + theSize/3, theY);
    g->DrawLineAA(theX + theSize/3, theY, theX - theSize/3, theY + theSize);
}
#endif
