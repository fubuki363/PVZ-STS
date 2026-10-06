/*
 * Portions of this file are based on the PopCap Games Framework
 * Copyright (C) 2005-2009 PopCap Games, Inc.
 *
 * Copyright (C) 2026 Zhou Qiankang <wszqkzqk@qq.com>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later AND LicenseRef-PopCap
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

#include "graphics/DisplayGeometry.h"
#include <cstdlib>
#include <iostream>
#include <utility>
using Sexy::GetDisplayViewport;
int main()
{
    for (auto size : {std::pair{1920,1080}, std::pair{2560,1440}, std::pair{3840,2160}, std::pair{7680,4320}})
    {
        auto p = GetDisplayViewport(size.first,size.second);
        if (p.mWidth * 3 != p.mHeight * 4 || p.mX * 2 + p.mWidth != size.first || p.mY != 0) return 1;
    }
    auto p = GetDisplayViewport(3840,2160);
    if (p.mX != 480 || p.mWidth != 2880 || p.mHeight != 2160) return 2;
    // Logical mouse coordinates must map identically at 100/150/200/300% DPI.
    for (int scale : {1,2,3})
    {
        auto logical = GetDisplayViewport(3840/scale,2160/scale);
        for (int gameX : {0,100,400,700,800})
        {
            int mouseX = logical.mX + gameX * logical.mWidth / 800;
            int mapped = (mouseX - logical.mX) * 800 / logical.mWidth;
            if (std::abs(mapped-gameX)>1) return 3;
        }
        int gameCenterY = (1080/scale-logical.mY)*600/logical.mHeight;
        if (gameCenterY != 300) return 4;
    }
    auto dpi150 = GetDisplayViewport(2560,1440);
    if (dpi150.mX != 320 || dpi150.mWidth != 1920) return 5;
    for(int w : {0,1,640,800,1200,1920,2560,3840,7680})
        for(int h : {0,1,480,600,720,1080,1440,2160,4320})
        {
            auto v=GetDisplayViewport(w,h);
            if(v.mWidth<1 || v.mHeight<1 || v.mX<0 || v.mY<0) return 6;
        }
    std::cout << "Display geometry: 4K/8K, DPI mapping, letterboxing and minimized windows passed.\n";
}
