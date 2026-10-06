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

#ifndef __DISPLAYGEOMETRY_H__
#define __DISPLAYGEOMETRY_H__

namespace Sexy
{
struct DisplayViewport
{
    int mX, mY, mWidth, mHeight;
};

inline DisplayViewport GetDisplayViewport(int theWidth, int theHeight)
{
    int aWidth = theWidth > 0 ? theWidth : 1;
    int aHeight = theHeight > 0 ? theHeight : 1;
    DisplayViewport aViewport{0, 0, aWidth, aHeight};
    if (aWidth * 3 > aHeight * 4)
    {
        aViewport.mWidth = aHeight * 4 / 3;
        if (aViewport.mWidth < 1) aViewport.mWidth = 1;
        aViewport.mX = (aWidth - aViewport.mWidth) / 2;
    }
    else if (aWidth * 3 < aHeight * 4)
    {
        aViewport.mHeight = aWidth * 3 / 4;
        if (aViewport.mHeight < 1) aViewport.mHeight = 1;
        aViewport.mY = (aHeight - aViewport.mHeight) / 2;
    }
    return aViewport;
}
}
#endif
