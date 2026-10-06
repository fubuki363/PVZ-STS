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

#ifndef __DEBUGSCENARIOSCREEN_H__
#define __DEBUGSCENARIOSCREEN_H__
#include "widget/Widget.h"
#include "widget/ButtonListener.h"
#include <vector>
class LawnApp;
class LawnStoneButton;
class DebugScenarioScreen : public Sexy::Widget,public Sexy::ButtonListener
{
public:
    LawnApp* mApp;
    int mTab=0,mPage=0,mSelectedSlot=0;
    std::vector<LawnStoneButton*> mButtons;
    DebugScenarioScreen(LawnApp* theApp);
    ~DebugScenarioScreen();
    void AddedToManager(Sexy::WidgetManager*) override;
    void RemovedFromManager(Sexy::WidgetManager*) override;
    void Draw(Sexy::Graphics*) override;
    void MouseDown(int,int,int) override;
    void KeyDown(Sexy::KeyCode) override;
    void ButtonDepress(int) override;
    void ButtonPress(int) override {}
    void ButtonDownTick(int) override {}
    void ButtonMouseEnter(int) override {}
    void ButtonMouseLeave(int) override {}
    void ButtonMouseMove(int,int,int) override {}
    void Refresh();
    void AddButton(int,const char*,int,int,int,int);
};
#endif
