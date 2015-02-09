/*
    This file is part of Nori, a simple educational ray tracer

    Copyright (c) 2015 by Wenzel Jakob

    Nori is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Nori is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <nori/common.h>
#include <chrono>

NORI_NAMESPACE_BEGIN

/**
 * \brief Simple timer with millisecond precision
 *
 * This class is convenient for collecting performance data
 */
class Timer {
public:
    /// Create a new timer and reset it
    Timer() { reset(); }

    /// Reset the timer to the current time
    void reset() { start = std::chrono::system_clock::now(); }

    /// Return the number of milliseconds elapsed since the timer was last reset
    double elapsed() const {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        return (double) duration.count();
    }

    /// Like \ref elapsed(), but return a human-readable string
    std::string elapsedString(bool precise = false) const {
        return timeString(elapsed(), precise);
    }

    /// Return the number of milliseconds elapsed since the timer was last reset and then reset it
    double lap() {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        start = now;
        return (double) duration.count();
    }

    /// Like \ref lap(), but return a human-readable string
    std::string lapString(bool precise = false) {
        return timeString(lap(), precise);
    }
private:
    std::chrono::system_clock::time_point start;
};

NORI_NAMESPACE_END
