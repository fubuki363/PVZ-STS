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

#include <SDL.h>

#include "SexyAppBase.h"
#include "graphics/GLInterface.h"
#include "graphics/GLImage.h"
#include "graphics/GLPlatform.h"
#include "widget/WidgetManager.h"

#ifndef SDL_HINT_APP_ID // SDL2 compatibility (already defined in SDL3.2+)
#define SDL_HINT_APP_ID "SDL_APP_ID"
#endif

using namespace Sexy;

void SexyAppBase::SetDisplayResolution(int theWidth, int theHeight)
{
	mDisplayWidth = theWidth;
	mDisplayHeight = theHeight;
	if (mWindow && mIsWindowed)
	{
		int aLogicalWidth, aLogicalHeight, aPixelWidth, aPixelHeight;
		SDL_GetWindowSize((SDL_Window*)mWindow, &aLogicalWidth, &aLogicalHeight);
		SDL_GL_GetDrawableSize((SDL_Window*)mWindow, &aPixelWidth, &aPixelHeight);
		float aScaleX = aPixelWidth / (float)std::max(1, aLogicalWidth);
		float aScaleY = aPixelHeight / (float)std::max(1, aLogicalHeight);
		int aWidth = theWidth > 0 ? theWidth : mWidth * IMG_DOWNSCALE;
		int aHeight = theHeight > 0 ? theHeight : mHeight * IMG_DOWNSCALE;
		SDL_Rect aAvailable;
		if (SDL_GetDisplayUsableBounds(SDL_GetWindowDisplayIndex((SDL_Window*)mWindow), &aAvailable) == 0)
		{
			// Keep large presets usable on a smaller monitor. Fullscreen always
			// uses the monitor's native physical resolution, including 4K.
			float aFit = std::min(1.0f, std::min(
				(aAvailable.w * aScaleX * 0.94f) / aWidth,
				(aAvailable.h * aScaleY * 0.90f) / aHeight));
			aWidth = std::max(1, (int)(aWidth * aFit));
			aHeight = std::max(1, (int)(aHeight * aFit));
		}
		SDL_SetWindowSize((SDL_Window*)mWindow,
			std::max(1, (int)(aWidth / aScaleX + 0.5f)),
			std::max(1, (int)(aHeight / aScaleY + 0.5f)));
		SDL_SetWindowPosition((SDL_Window*)mWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	if (mGLInterface)
	{
		mGLInterface->UpdateViewport();
		mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
		mWidgetManager->MarkAllDirty();
	}
}

void SexyAppBase::MakeWindow()
{
	if (mWindow)
	{
		SDL_SetWindowFullscreen((SDL_Window*)mWindow, (!mIsWindowed ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
		if (mIsWindowed)
			SetDisplayResolution(mDisplayWidth, mDisplayHeight);
	}
	else
	{
		// For Wayland's icon support on the game window
		SDL_SetHint(SDL_HINT_APP_ID, "io.github.wszqkzqk.pvz-portable");

#if (defined(__ANDROID__) && !defined(__TERMUX__)) || defined(__IPHONEOS__)
		SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

		// Request a physical-pixel drawable instead of OS bitmap magnification.
#ifdef _WIN32
		SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");
#endif
		SDL_Init(SDL_INIT_VIDEO);

		Uint32 winFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
			| (!mIsWindowed ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
		if (GetBoolean("SANDBOX_SELF_TEST", false)) winFlags |= SDL_WINDOW_HIDDEN;

		// Try OpenGL ES 2.0 first (Linux, most Windows drivers, ANGLE, etc.)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

		mWindow = (void*)SDL_CreateWindow(
			mTitle.c_str(),
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			mWidth * IMG_DOWNSCALE, mHeight * IMG_DOWNSCALE, winFlags);

		if (mWindow)
			mContext = (void*)SDL_GL_CreateContext((SDL_Window*)mWindow);

#if defined(__ANDROID__) || defined(__IPHONEOS__)
		// EGL/EAGL surface may be transiently unavailable on mobile
		for (int retry = 0; !mContext && mWindow && retry < 20; retry++)
		{
			SDL_Delay(100);
			SDL_PumpEvents();
			mContext = (void*)SDL_GL_CreateContext((SDL_Window*)mWindow);
		}
		if (!mContext)
		{
			if (mWindow) { SDL_DestroyWindow((SDL_Window*)mWindow); mWindow = nullptr; }
			fprintf(stderr, "Failed to create OpenGL ES context.\n");
			return;
		}
#else
		// Fallback: desktop GL 2.1 compatibility (macOS, old Windows drivers, etc.)
		if (!mContext)
		{
			if (mWindow) { SDL_DestroyWindow((SDL_Window*)mWindow); mWindow = nullptr; }

			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

			mWindow = (void*)SDL_CreateWindow(
				mTitle.c_str(),
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				mWidth * IMG_DOWNSCALE, mHeight * IMG_DOWNSCALE, winFlags);

			if (mWindow)
				mContext = (void*)SDL_GL_CreateContext((SDL_Window*)mWindow);

			if (!mContext)
			{
				if (mWindow) { SDL_DestroyWindow((SDL_Window*)mWindow); mWindow = nullptr; }
				fprintf(stderr, "Failed to create any OpenGL context. "
					"Please check your graphics drivers.\n");
				return;
			}

			gDesktopGLFallback = true;
		}
#endif

		SDL_GL_SetSwapInterval(1);
		if (mIsWindowed)
			SetDisplayResolution(mDisplayWidth, mDisplayHeight);
	}

	if (mGLInterface == nullptr)
	{
		mGLInterface = new GLInterface(this);
		if (!InitGLInterface())
		{
			delete mGLInterface;
			mGLInterface = nullptr;
			return;
		}
	}

	bool isActive = mActive;
	mActive = !!(SDL_GetWindowFlags((SDL_Window*)mWindow) & SDL_WINDOW_INPUT_FOCUS);

	mPhysMinimized = false;
	if (mMinimized)
	{
		if (mMuteOnLostFocus)
			Unmute(true);

		mMinimized = false;
		isActive = mActive; // set this here so we don't call RehupFocus again.
		RehupFocus();
	}
	
	if (isActive != mActive)
		RehupFocus();

	ReInitImages();

	mWidgetManager->mImage = mGLInterface->GetScreenImage();
	mWidgetManager->MarkAllDirty();

	mGLInterface->UpdateViewport();
	mWidgetManager->Resize(mScreenBounds, mGLInterface->mPresentationRect);
}
