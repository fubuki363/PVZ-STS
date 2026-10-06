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

#include "DebugScenarioScreen.h"
#include "SandboxShootingCheck.h"
#include "SandboxStokeCheck.h"
#include "SandboxGlassCheck.h"
#include "GameButton.h"
#include "../../LawnApp.h"
#include "../Board.h"
#include "../Plant.h"
#include "../Zombie.h"
#include "../Projectile.h"
#include "../Coin.h"
#include "../CursorObject.h"
#include "../System/SaveGame.h"
#include "../SeedPacket.h"
#include "../CutScene.h"
#include "../System/PlayerInfo.h"
#include "../System/Music.h"
#include "../../Resources.h"
#include "graphics/Graphics.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/GLPlatform.h"
#include "imagelib/ImageLib.h"
#include <fstream>
#include "widget/WidgetManager.h"
#include "../../Sexy.TodLib/TodStringFile.h"
using namespace Sexy;

void LawnApp::ShowDebugSetup()
{
    if(mDebugScreen)return;
    if(!mDebugConfig.mEnabled)
    {
        if(!mPlayerInfo)return;
        if(mBoard)mBoard->TryToSaveGame();
        mDebugOriginalPlayer=mPlayerInfo;
        mDebugPlayer=new PlayerInfo(*mPlayerInfo);
        mDebugPlayer->mLevel=6;mDebugPlayer->mFinishedAdventure=1;
        mDebugPlayer->mId=0x7ffffff0U;
        mPlayerInfo=mDebugPlayer;mDebugConfig.mEnabled=true;
        KillGameSelector();KillBoard();
    }
    if(mBoard)mBoard->mPaused=true;
    mDebugScreen=new DebugScenarioScreen(this);
    mDebugScreen->Resize(0,0,mWidth,mHeight);
    mWidgetManager->AddWidget(mDebugScreen);
    mWidgetManager->BringToFront(mDebugScreen);
    mWidgetManager->SetFocus(mDebugScreen);
}
void LawnApp::CloseDebugSetup()
{
    if(mDebugScreen)
    {
        mWidgetManager->RemoveWidget(mDebugScreen);
        SafeDeleteWidget(mDebugScreen);mDebugScreen=nullptr;
    }
    if(mBoard){mBoard->mPaused=false;mWidgetManager->SetFocus(mBoard);}
}
void LawnApp::EndDebugSandbox(bool theShowMenu)
{
    CloseDebugSetup();
    if(mDebugConfig.mEnabled)
    {
        KillBoard();mPlayerInfo=mDebugOriginalPlayer;
        mDebugOriginalPlayer=nullptr;delete mDebugPlayer;mDebugPlayer=nullptr;
        mDebugConfig.mEnabled=false;mDebugConfig.mOrbTool=0;
    }
    if(theShowMenu)ShowGameSelector();
}
void LawnApp::ApplyDebugCards()
{
    if(!mBoard)return;
    mDebugConfig.Normalize();
    mBoard->mSeedBank->mNumPackets=mDebugConfig.mSlots;
    mBoard->mSeedBank->UpdateWidth();
    for(int i=0;i<mDebugConfig.mSlots;i++)
    {
        SeedPacket& p=mBoard->mSeedBank->mSeedPackets[i];
        SeedType aSeed=(SeedType)mDebugConfig.mCards[i];
        p.SetPacketType(aSeed,aSeed==SeedType::SEED_IMITATER?(SeedType)mDebugConfig.mImitater:SeedType::SEED_NONE);
        p.mRefreshCounter=0;p.mRefreshing=false;p.Activate();
    }
    Plant* aPlant=nullptr;
    while(mBoard->IteratePlants(aPlant))
        if(Plant::IsNocturnal(aPlant->mSeedType))
            aPlant->SetSleeping(!mDebugConfig.mWakeMushrooms&&!mBoard->StageIsNight());
}
void LawnApp::StartDebugScenario()
{
    mDebugConfig.Normalize();
    if(mBoard && (int)mBoard->mBackground==mDebugConfig.mScene)
    {ApplyDebugCards();CloseDebugSetup();return;}
    CloseDebugSetup();mGameMode=GameMode::GAMEMODE_ADVENTURE;
    MakeNewBoard();mBoard->InitLevel();
    mBoardResult=BoardResult::BOARDRESULT_NONE;
    mBoard->mCutScene->PreloadResources();
    for(int i=0;i<(int)ZombieType::NUM_ZOMBIE_TYPES;i++)Zombie::PreloadZombieResources((ZombieType)i);
    mBoard->mSodPosition=0;mBoard->mTutorialState=TutorialState::TUTORIAL_OFF;
    mBoard->mSunMoney=9990;mBoard->mCurrentWave=0;
    mGameScene=GameScenes::SCENE_PLAYING;
    mBoard->StartLevel();ApplyDebugCards();
    mBoard->mTutorialState=TutorialState::TUTORIAL_OFF;
    mWidgetManager->SetFocus(mBoard);
}
void LawnApp::SpawnDebugZombies()
{
    if(!mBoard || (int)mBoard->mBackground!=mDebugConfig.mScene)return;
    mDebugConfig.Normalize();
    ZombieType type=(ZombieType)mDebugConfig.mZombie;
    int count=type==ZombieType::ZOMBIE_BOSS?1:mDebugConfig.mBatch;
    Zombie* z=nullptr;
    if(type==ZombieType::ZOMBIE_BOSS)
        while(mBoard->IterateZombies(z))if(!z->mDead&&z->mZombieType==type)return;
    int aNeeded=type==ZombieType::ZOMBIE_BOBSLED?4:1;
    for(int i=0;i<count && mBoard->mZombies.mSize+aNeeded<mBoard->mZombies.mMaxSize;i++)
    {
        int row=mDebugConfig.mRow>=0?mDebugConfig.mRow:i%mDebugConfig.Rows();
        Zombie* aZombie=mBoard->AddZombieInRow(type,row,0);
        if(aZombie && type!=ZombieType::ZOMBIE_BOSS && type!=ZombieType::ZOMBIE_BUNGEE)
            aZombie->mPosX+=i*18.0f;
    }
}
void LawnApp::ClearDebugField(bool thePlants)
{
    if(!mBoard)return;
    if(thePlants){Plant* p=nullptr;while(mBoard->IteratePlants(p))p->Die();}
    else {Zombie* z=nullptr;while(mBoard->IterateZombies(z))z->DieNoLoot();}
    Projectile* p=nullptr;while(mBoard->IterateProjectiles(p))p->Die();
    mBoard->ProcessDeleteQueue();
}

DebugScenarioScreen::DebugScenarioScreen(LawnApp* theApp):mApp(theApp)
{
    const char* tabs[]={"场景","植物 / 卡槽","僵尸","测试规则"};
    for(int i=0;i<4;i++)AddButton(10+i,tabs[i],30+185*i,62,175,36);
    AddButton(20,"上一页",45,510,130,30);AddButton(21,"下一页",195,510,130,30);
    AddButton(30,"退出调试",35,552,150,34);AddButton(31,"开始 / 应用",605,552,160,34);
    AddButton(40,"槽位 -",520,510,105,30);AddButton(41,"槽位 +",645,510,105,30);
    AddButton(50,"无限阳光",70,155,295,42);AddButton(51,"无冷却",425,155,295,42);
    AddButton(52,"忽略地形",70,215,295,42);AddButton(53,"冻结世界",425,215,295,42);
    AddButton(54,"故障豌豆",70,275,295,42);AddButton(55,"不会失败",425,275,295,42);
    AddButton(56,"蘑菇保持清醒",70,335,295,42);AddButton(57,"模仿者目标",425,335,295,42);
    AddButton(60,"生成行",50,395,170,38);AddButton(62,"生成数量",240,395,170,38);
    AddButton(63,"生成僵尸",590,395,160,38);AddButton(64,"清空僵尸",50,452,150,36);
    AddButton(65,"清空植物",215,452,150,36);AddButton(66,"充能球 +1",405,452,150,36);
    AddButton(67,"释放充能球",575,452,150,36);Refresh();
}
DebugScenarioScreen::~DebugScenarioScreen(){for(auto* b:mButtons)delete b;}
void DebugScenarioScreen::AddButton(int id,const char* text,int x,int y,int w,int h)
{auto* b=MakeButton(id,this,text);b->Resize(x,y,w,h);mButtons.push_back(b);}
void DebugScenarioScreen::AddedToManager(WidgetManager* m)
{Widget::AddedToManager(m);for(auto* b:mButtons)AddWidget(b);}
void DebugScenarioScreen::RemovedFromManager(WidgetManager* m)
{for(auto* b:mButtons)RemoveWidget(b);Widget::RemovedFromManager(m);}
void DebugScenarioScreen::Refresh()
{
    auto& c=mApp->mDebugConfig;c.Normalize();mSelectedSlot=std::min(mSelectedSlot,c.mSlots-1);
    for(auto* b:mButtons)
    {
        int id=b->mId;
        bool visible=id<20 || id==30 || id==31 || ((id==20||id==21)&&(mTab==1||mTab==2)) ||
            ((id==40||id==41)&&mTab==1) || (id>=50&&id<=57&&mTab==3) || (id>=60&&mTab==2);
        b->SetVisible(visible);
        if(id>=63&&id<=67)b->SetDisabled(!mApp->mBoard || (int)mApp->mBoard->mBackground!=c.mScene);
        if(id>=50&&id<=56)
        {
            const char* names[]={"无限阳光","无冷却","忽略地形限制","冻结世界","故障豌豆射手","不会失败","蘑菇保持清醒"};
            bool values[]={c.mInfiniteSun,c.mNoCooldown,c.mFreePlacement,c.mFrozen,c.mDefect,c.mNoLoss,c.mWakeMushrooms};
            b->SetLabel(std::string(names[id-50])+(values[id-50]?"：开":"：关"));
        }
        if(id==57)b->SetLabel("模仿者："+Plant::GetNameString((SeedType)c.mImitater));
        if(id==60)b->SetLabel(c.mRow<0?"生成行：轮流各行":StrFormat("生成行：%d",c.mRow+1));
        if(id==62)b->SetLabel(StrFormat("数量：%d",c.mBatch));
    }
    MarkDirty();
}
void DebugScenarioScreen::Draw(Graphics* g)
{
    g->SetColor(Color(20,29,39));g->FillRect(0,0,800,600);
    g->SetFont(FONT_HOUSEOFTERROR28);g->SetColor(Color(241,227,161));g->DrawString("调试关卡",30,40);
    g->SetFont(FONT_DWARVENTODCRAFT18);g->SetColor(Color(177,204,218));
    g->DrawString("独立测试 / F8 打开面板",420,38);
    auto& c=mApp->mDebugConfig;
    if(mTab==0)
    {
        g->DrawString("选择场景；进入关卡后改场景会重建场地",45,135);
        for(int i=0;i<10;i++)
        {
            int x=45+(i%5)*145,y=165+(i/5)*125;
            g->SetColor(i==c.mScene?Color(69,121,92):Color(47,60,75));g->FillRect(x,y,135,100);
            g->SetColor(Color(230,236,240));g->DrawString(DebugSceneName(i),x+15,y+42);
            g->DrawString((i==2||i==3||i==8)?"6 行场地":"5 行场地",x+15,y+75);
        }
    }
    if(mTab==1)
    {
        g->DrawString(StrFormat("卡槽：%d / 10；先选卡槽，再点植物卡",c.mSlots),45,118);
        for(int i=0;i<c.mSlots;i++)
        {
            int x=40+i*72;
            g->SetColor(i==mSelectedSlot?Color(174,134,43):Color(44,58,71));g->FillRect(x,128,65,42);
            Graphics cg(*g);cg.SetScale(0.46f,0.46f,(float)x+19,133.0f);
            DrawSeedPacket(&cg,(float)x+19,133.0f,(SeedType)c.mCards[i],(SeedType)c.mImitater,0,255,false,false);
            g->SetColor(Color::White);g->DrawString(StrFormat("%d",i+1),x+3,148);
        }
        for(int n=0;n<20;n++)
        {
            int index=mPage*20+n;if(index>=DebugSelectablePlantCount)break;
            int type=DebugSelectablePlantAt(index);
            int x=45+(n%5)*145,y=182+(n/5)*79;
            g->SetColor(c.mCards[mSelectedSlot]==type?Color(56,111,82):Color(45,57,70));g->FillRect(x,y,135,72);
            Graphics cg(*g);cg.SetScale(0.72f,0.72f,(float)x+5,(float)y+9);
            DrawSeedPacket(&cg,(float)x+5,(float)y+9,(SeedType)type,(SeedType)c.mImitater,0,255,false,false);
            std::string name=Plant::GetNameString((SeedType)type);
            if(type>=49&&type<=52){const char* hidden[]={"爆炸坚果","巨型坚果","芽","左向双发"};name=hidden[type-49];}
            g->SetFont(FONT_DWARVENTODCRAFT12);g->SetColor(Color::White);g->DrawString(name,x+48,y+37);
            g->SetFont(FONT_DWARVENTODCRAFT18);
        }
        g->SetColor(Color(195,209,216));g->DrawString(StrFormat("第 %d / 3 页",mPage+1),365,532);
    }
    if(mTab==2)
    {
        for(int n=0;n<12;n++)
        {
            int type=mPage*12+n;if(type>=(int)ZombieType::NUM_ZOMBIE_TYPES)break;
            int x=45+(n%4)*180,y=126+(n/4)*82;
            g->SetColor(type==c.mZombie?Color(130,80,69):Color(50,60,74));g->FillRect(x,y,168,68);
            g->SetColor(Color::White);g->DrawString(DebugZombieName(type),x+12,y+38);
        }
        g->SetColor(Color(197,212,220));g->DrawString("生成后点开始 / 应用继续；充能球按钮再点击射手或柱形构筑体",45,380);
    }
    if(mTab==3)
    {
        g->DrawString("选卡与生成不受冒险解锁限制；金币、进度和成就使用临时玩家",45,125);
        g->DrawString("充能球测试：在僵尸页选择充能或释放，再点击植物",70,410);
        g->DrawString("满槽充能释放最早的球；每次释放连续发射 20 颗豌豆",70,435);
        g->DrawString("全屏自动匹配显示器分辨率，支持 4K",70,455);
    }
}
void DebugScenarioScreen::MouseDown(int x,int y,int count)
{
    Widget::MouseDown(x,y,count);if(count<0)return;
    auto& c=mApp->mDebugConfig;
    if(mTab==0)for(int i=0;i<10;i++)if(Rect(45+(i%5)*145,165+(i/5)*125,135,100).Contains(x,y))c.mScene=i;
    if(mTab==1)
    {
        for(int i=0;i<c.mSlots;i++)if(Rect(40+i*72,128,65,42).Contains(x,y))mSelectedSlot=i;
        for(int n=0;n<20;n++)if(Rect(45+(n%5)*145,182+(n/5)*79,135,72).Contains(x,y))c.SetCard(mSelectedSlot,DebugSelectablePlantAt(mPage*20+n));
    }
    if(mTab==2)for(int n=0;n<12;n++)if(Rect(45+(n%4)*180,126+(n/4)*82,168,68).Contains(x,y))
    {int z=mPage*12+n;if(z<(int)ZombieType::NUM_ZOMBIE_TYPES)c.mZombie=z;}
    Refresh();
}
void DebugScenarioScreen::KeyDown(KeyCode k)
{
    if((int)k>='1'&&(int)k<='4'){mTab=(int)k-'1';mPage=0;Refresh();}
    if(k==KEYCODE_LEFT){mPage=(mPage+2)%3;Refresh();}
    if(k==KEYCODE_RIGHT){mPage=(mPage+1)%3;Refresh();}
    if(k==KEYCODE_RETURN){mApp->StartDebugScenario();return;}
    if(k==KEYCODE_ESCAPE||k==KEYCODE_F8){if(mApp->mBoard)mApp->CloseDebugSetup();else mApp->EndDebugSandbox(true);}
}
void DebugScenarioScreen::ButtonDepress(int id)
{
    auto& c=mApp->mDebugConfig;
    if(id>=10&&id<14){mTab=id-10;mPage=0;}
    if(id==20)mPage=(mPage+2)%3;
    if(id==21)mPage=(mPage+1)%3;
    if(id==30){mApp->EndDebugSandbox(true);return;}
    if(id==31){mApp->StartDebugScenario();return;}
    if(id==40)c.mSlots=std::max(1,c.mSlots-1);
    if(id==41)c.mSlots=std::min(10,c.mSlots+1);
    if(id==50)c.mInfiniteSun=!c.mInfiniteSun;
    if(id==51)c.mNoCooldown=!c.mNoCooldown;
    if(id==52)c.mFreePlacement=!c.mFreePlacement;
    if(id==53)c.mFrozen=!c.mFrozen;
    if(id==54)c.mDefect=!c.mDefect;
    if(id==55)c.mNoLoss=!c.mNoLoss;
    if(id==56)c.mWakeMushrooms=!c.mWakeMushrooms;
    if(id==57)c.mImitater=(c.mImitater+1)%48;
    if(id==60){++c.mRow;if(c.mRow>=c.Rows())c.mRow=-1;}
    if(id==62){const int counts[]={1,5,10,20,50};int next=0;for(int i=0;i<5;i++)if(c.mBatch==counts[i])next=(i+1)%5;c.mBatch=counts[next];}
    if(id==63)mApp->SpawnDebugZombies();
    if(id==64)mApp->ClearDebugField(false);
    if(id==65)mApp->ClearDebugField(true);
    if(id==66||id==67){c.mOrbTool=id==66?1:2;mApp->CloseDebugSetup();return;}
    Refresh();
}

void LawnApp::RunSandboxSelfTest()
{
    int aLevel=mPlayerInfo->mLevel,aCoins=mPlayerInfo->mCoins;
    uint32_t aFinished=mPlayerInfo->mFinishedAdventure;
    int aCards=0,aZombieCases=0,aScenes=0;
    bool aPassed=true,aOrbRules=false,aSaveRoundtrip=false;
    bool aConstructAssets=mStsPillar && mStsShooter && mStsGlassOrb;
    for(auto& variant:mStsConstructLayers)for(Image* layer:variant)aConstructAssets=aConstructAssets && layer;
    int aAnimationFrames=0;
    bool aShooting=true,aStoke=true;
    int aShootingCases=0,aStokeCases=0;
    bool aGlass=true;int aGlassCases=0;
    ShowDebugSetup();
    for(int scene=0;scene<10;scene++)
    {
        mDebugConfig.mScene=scene;mDebugConfig.mSlots=10;mDebugConfig.mBatch=1;
        StartDebugScenario();ProcessSafeDeleteList();
        if(!mBoard||(int)mBoard->mBackground!=scene){aPassed=false;break;}
        ++aScenes;
        for(int slotCount=1;slotCount<=10;slotCount++)
        {
            mDebugConfig.mSlots=slotCount;ApplyDebugCards();
            if(mBoard->mSeedBank->mNumPackets!=slotCount)aPassed=false;
        }
        mDebugConfig.mSlots=10;
        for(int index=0;index<DebugSelectablePlantCount;index++)
        {
            int card=DebugSelectablePlantAt(index);
            mDebugConfig.mCards[0]=card;ApplyDebugCards();
            if((int)mBoard->mSeedBank->mSeedPackets[0].mPacketType!=card)aPassed=false;
            ++aCards;
        }
        for(int type=0;type<(int)ZombieType::NUM_ZOMBIE_TYPES;type++)
        {
            std::ofstream(PathFromU8(GetAppDataPath("sandbox-progress.txt")))<<"scene="<<scene<<",zombie="<<type;
            mDebugConfig.mZombie=type;
            mDebugConfig.mRow=(mBoard->StageHas6Rows() && (type==10||type==11||type==14))?2:0;
            SpawnDebugZombies();
            if(mBoard->mZombies.mSize==0)aPassed=false;
            for(int tick=0;tick<3;tick++)mBoard->Update();
            Graphics aG(mGLInterface->GetScreenImage());mBoard->Draw(&aG);mGLInterface->Flush();
            ClearDebugField(false);++aZombieCases;
        }
        aShooting=VerifyConstructShooting(this,aShootingCases) && aShooting;
        aStoke=VerifyStokeRow(this,aStokeCases) && aStoke;
        aGlass=VerifyGlassOrbs(this,aGlassCases) && aGlass;
        if(scene==0)
        {
            Plant* pea=mBoard->AddPlant(2,2,SeedType::SEED_PEASHOOTER,SeedType::SEED_NONE);
            Plant* pillar=mBoard->AddPlant(1,2,SeedType::SEED_SUNFLOWER,SeedType::SEED_NONE);
            aOrbRules=pea&&pillar&&pea->mDefectOrbs.PassiveShots()==1&&pillar->mDefectOrbs.PlasmaCount()==1;
            pea->ChannelOrb(DefectOrbType::LIGHTNING);
            aOrbRules=aOrbRules&&pea->mOrbBurstRemaining==0&&pea->mDefectOrbs.PassiveShots()==2;
            pea->ChannelOrb(DefectOrbType::LIGHTNING);
            aOrbRules=aOrbRules&&pea->mOrbBurstRemaining==20;
            int before=mBoard->mProjectiles.mSize;
            for(int tick=0;tick<120;tick++)pea->UpdateOrbBurst();
            aOrbRules=aOrbRules&&pea->mOrbBurstRemaining==0&&mBoard->mProjectiles.mSize-before==20;
            Projectile* projectile=nullptr;while(mBoard->IterateProjectiles(projectile))
                aOrbRules=aOrbRules&&projectile->mProjectileType==ProjectileType::PROJECTILE_PEA&&projectile->mIsLightningPea&&projectile->GetProjectileDef().mDamage==20;
            int coins=mBoard->mCoins.mSize;pillar->mLaunchCounter=1;pillar->UpdateProductionPlant();
            aOrbRules=aOrbRules&&mBoard->mCoins.mSize-coins==1&&pillar->mLaunchCounter>=2350&&pillar->mLaunchCounter<=2500;
            Coin* sun=nullptr;while(mBoard->IterateCoins(sun))if(sun->IsSun())aOrbRules=aOrbRules&&sun->GetSunValue()==25;
            pillar->ChannelOrb(DefectOrbType::PLASMA);coins=mBoard->mCoins.mSize;pillar->mLaunchCounter=1;pillar->UpdateProductionPlant();
            aOrbRules=aOrbRules&&mBoard->mCoins.mSize-coins==2;
            pillar->EvokeOrb();before=mBoard->mProjectiles.mSize;
            for(int tick=0;tick<120;tick++)pillar->UpdateOrbBurst();
            aOrbRules=aOrbRules&&mBoard->mProjectiles.mSize-before==20&&pillar->mDefectOrbs.PlasmaCount()==1;
            mDebugConfig.mInfiniteSun=false;mDebugConfig.mSlots=3;mDebugConfig.mCards={0,1,(int)SeedType::SEED_LIGHTNING_ORB_CARD,3,4,5,6,7,8,9};ApplyDebugCards();
            mBoard->mSunMoney=100;mBoard->mCursorObject->mCursorType=CursorType::CURSOR_TYPE_PLANT_FROM_BANK;
            mBoard->mCursorObject->mType=SeedType::SEED_LIGHTNING_ORB_CARD;mBoard->mCursorObject->mSeedBankIndex=2;
            mBoard->MouseDownWithPlant(pea->mX+35,pea->mY+35,1);
            aOrbRules=aOrbRules&&mBoard->mSunMoney==75&&pea->mOrbBurstRemaining==20&&mBoard->mPlants.mSize==2;
            pea->mDefectOrbs.RestoreSlots(1,2);pea->mOrbBurstRemaining=13;pea->mOrbBurstTimer=3;
            std::string save=GetAppDataPath("orb-roundtrip.dat");
            aSaveRoundtrip=LawnSaveGame(mBoard,save);MakeNewBoard();aSaveRoundtrip=mBoard->LoadGame(save)&&aSaveRoundtrip;
            pea=nullptr;Plant* loaded=nullptr;while(mBoard->IteratePlants(loaded))if(loaded->mSeedType==SeedType::SEED_PEASHOOTER)pea=loaded;
            aSaveRoundtrip=aSaveRoundtrip&&pea&&pea->mDefectOrbs.Get(0)==DefectOrbType::LIGHTNING&&pea->mDefectOrbs.Get(1)==DefectOrbType::PLASMA&&pea->mOrbBurstRemaining==13&&pea->mOrbBurstTimer==3;
            mDebugConfig.mInfiniteSun=true;mDebugConfig.mSlots=5;mDebugConfig.mCards[3]=(int)SeedType::SEED_JALAPENO;
            mDebugConfig.mCards[4]=(int)SeedType::SEED_GLASS_ORB_CARD;ApplyDebugCards();
            projectile=nullptr;while(mBoard->IterateProjectiles(projectile))projectile->Die();mBoard->ProcessDeleteQueue();
            sun=nullptr;while(mBoard->IterateCoins(sun))sun->Die();mBoard->ProcessDeleteQueue();
            // Show actual flying peas and all four starter cards in the native preview.
            mBoard->mSunMoney=375;pea->mOrbBurstRemaining=0;pea->mOrbBurstQueueCount=0;
            pea->mOrbBurstBatchRemaining=0;pea->mDefectOrbs.RestoreSlots(1,3);
            Zombie* previewZombie=mBoard->AddZombieInRow(ZombieType::ZOMBIE_NORMAL,2,0);
            previewZombie->mPosX=570.0f;previewZombie->mX=570;previewZombie->mBodyHealth=1000;
            for(int tick=0;tick<55;++tick)
            {
                if(tick%12==0)pea->Fire(previewZombie,2,PlantWeapon::WEAPON_PRIMARY);
                projectile=nullptr;while(mBoard->IterateProjectiles(projectile))projectile->Update();
            }
            mBoard->mSeedBank->mCutSceneDarken=255;
            auto capture=[&](const std::string& name)
            {
                Graphics preview(mGLInterface->GetScreenImage());mBoard->Draw(&preview);
                ImageLib::Image frame;frame.mWidth=mGLInterface->mDisplayWidth;frame.mHeight=mGLInterface->mDisplayHeight;
                frame.mBits=new uint32_t[frame.mWidth*frame.mHeight];std::vector<unsigned char> pixels(frame.mWidth*frame.mHeight*4);
                glReadPixels(0,0,frame.mWidth,frame.mHeight,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
                for(int y=0;y<frame.mHeight;y++)for(int x=0;x<frame.mWidth;x++)
                {int i=((frame.mHeight-1-y)*frame.mWidth+x)*4;frame.mBits[y*frame.mWidth+x]=0xff000000U|(pixels[i]<<16)|(pixels[i+1]<<8)|pixels[i+2];}
                bool saved=ImageLib::WritePNGImage(GetAppDataPath(name),&frame);mGLInterface->Flush();return saved;
            };
            aPassed=capture("orb-preview.png") && aPassed;
            auto oldCounter=mBoard->mMainCounter;
            // Sample one full two-second idle cycle from the native renderer.
            for(int frame=0;frame<24;++frame)
            {
                mBoard->mMainCounter=(int)(frame*(2.0*PI/0.032)/24.0);
                if(capture(StrFormat("construct-frame-%02d.png",frame)))++aAnimationFrames;
            }
            mBoard->mMainCounter=oldCounter;
            ClearDebugField(true);ClearDebugField(false);aPassed=aPassed&&aOrbRules&&aSaveRoundtrip;
        }
    }
    EndDebugSandbox(false);ProcessSafeDeleteList();
    bool aProfileUnchanged=mPlayerInfo->mLevel==aLevel&&mPlayerInfo->mCoins==aCoins&&mPlayerInfo->mFinishedAdventure==aFinished;
    aPassed=aPassed&&aProfileUnchanged&&aConstructAssets&&aAnimationFrames==24&&aShooting&&aStoke&&aGlass;
    std::ofstream aReport(PathFromU8(GetAppDataPath("sandbox-verification.json")));
    aReport<<"{\"passed\":"<<(aPassed?"true":"false")<<",\"scenes\":"<<aScenes
        <<",\"card_bindings\":"<<aCards<<",\"zombie_cases\":"<<aZombieCases
        <<",\"orb_rules\":"<<(aOrbRules?"true":"false")<<",\"orb_save_roundtrip\":"<<(aSaveRoundtrip?"true":"false")
        <<",\"profile_unchanged\":"<<(aProfileUnchanged?"true":"false")
        <<",\"construct_assets\":"<<(aConstructAssets?"true":"false")<<",\"animation_frames\":"<<aAnimationFrames
        <<",\"shooting_hits\":"<<(aShooting?"true":"false")<<",\"shooting_cases\":"<<aShootingCases
        <<",\"stoke_rules\":"<<(aStoke?"true":"false")<<",\"stoke_cases\":"<<aStokeCases
        <<",\"glass_rules\":"<<(aGlass?"true":"false")<<",\"glass_cases\":"<<aGlassCases<<"}";
    aReport.close();Shutdown();
}
