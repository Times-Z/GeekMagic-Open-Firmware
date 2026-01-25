// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * GeekMagic Open Firmware
 * Copyright (C) 2026 Times-Z
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SRC_DISPLAY_GIF_H
#define SRC_DISPLAY_GIF_H

#include <Arduino.h>
#include <AnimatedGIF.h>
#include <LittleFS.h>
#include <array>

class Gif {
   public:
    Gif();
    ~Gif();

    auto begin() -> bool;
    auto playOne(const String& path) -> bool;
    auto update() -> void;
    auto playAllFromLittleFS() -> bool;
    auto stop() -> void;
    auto isPlaying() const -> bool;
    auto setLoopEnabled(bool enabled) -> void;

   private:
    AnimatedGIF* m_gif;
    volatile bool m_playRequested;
    volatile bool m_playing;
    volatile bool m_loopEnabled;
    volatile bool m_stopRequested;

    int m_delayMsFromGif;
    uint32_t m_targetMs;
    uint32_t m_lastFrameMs;
    uint32_t m_startMs;
    int m_frameCount;

    static constexpr size_t LINEBUF_MAX = 240;

    std::array<uint16_t, LINEBUF_MAX> m_lineBuf;
    bool m_inFrameWrite = false;

    int16_t m_offsetX = 0;
    int16_t m_offsetY = 0;
    bool m_centered = false;

    String m_currentPath;

    File m_file;
    bool m_fileInUse = false;

    bool m_havePrev = false;
    uint8_t m_prevDisposal = 0;
    bool m_prevHadTransparency = false;
    int16_t m_prevX = 0;
    int16_t m_prevY = 0;
    int16_t m_prevW = 0;
    int16_t m_prevH = 0;
    uint16_t m_prevBg = 0;

    uint8_t m_curDisposal = 0;
    bool m_curHadTransparency = false;
    int16_t m_curX = 0;
    int16_t m_curY = 0;
    int16_t m_curW = 0;
    int16_t m_curH = 0;
    uint16_t m_curBg = 0;

    static Gif* s_instance;

    static auto gifOpenFile(const char* fname, int32_t* pSize) -> void*;
    static auto gifCloseFile(void* pHandle) -> void;
    static auto gifReadFile(GIFFILE* pFile, uint8_t* pBuf, int32_t iLen) -> int32_t;
    static auto gifSeekFile(GIFFILE* pFile, int32_t iPosition) -> int32_t;
    static auto gifDraw(GIFDRAW* pDraw) -> void;
};

#endif  // SRC_DISPLAY_GIF_H
