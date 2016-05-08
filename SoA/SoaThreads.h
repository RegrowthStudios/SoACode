//
// SoaThreads.h
// Seed of Andromeda
//
// Created by Benjamin Arnold on 7 May 2016
// Copyright 2014 Regrowth Studios
// All Rights Reserved
//
// Summary:
// Contains SoA thread specific information and utilities
//

#pragma once

#ifndef SoaThreads_h__
#define SoaThreads_h__

#include <Vorb/VorbAssert.hpp>
#include <thread>

extern std::thread::id THREAD_ID_GRAPHICS;

inline void SetThisThreadIsGraphics() { THREAD_ID_GRAPHICS = std::this_thread::get_id(); }
inline void AssertIsGraphics() { vorb_assert(std::this_thread::get_id() == THREAD_ID_GRAPHICS, "Not graphics thread!") }
inline void AssertIsNotGraphics() { vorb_assert(std::this_thread::get_id() != THREAD_ID_GRAPHICS, "Is graphics thread!") }

#endif // SoaThreads_h__
