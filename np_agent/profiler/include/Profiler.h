/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#pragma once

#include <chrono>
#include <cstdio>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

#include "Trace.h"

namespace profiler {

class EventProfiler;

class NpProfiler {
public:
    static NpProfiler& GetInstance() {
        static NpProfiler instance;
        return instance;
    }
    void Reg(const EventProfiler* prof) { mProfilers.push_back(prof); }
    int Enter() { return mIndent++; }
    void Exit() { mIndent--; }
    std::ostream& Print(std::ostream& o, int fnwidth) const;
    std::ostream& Log(std::ostream& o, const char* format, ...) const;
    NpProfiler(const NpProfiler&) = delete;
    NpProfiler(NpProfiler&&) = delete;
    NpProfiler& operator=(const NpProfiler&) = delete;

private:
    std::vector<const EventProfiler*> mProfilers;
    int mIndent;
    NpProfiler() {}
    const std::string FormatTime(double v) const {
        char buf[1024];
        if (v < 1e-6) {
            snprintf(buf, sizeof(buf), "%8.2lfns", 1e9 * v);
        } else if (v < 1e-3) {
            snprintf(buf, sizeof(buf), "%8.2lfus", 1e6 * v);
        } else if (v < 1) {
            snprintf(buf, sizeof(buf), "%8.2lfms", 1e3 * v);
        } else {
            snprintf(buf, sizeof(buf), "%8.2lfs ", v);
        }
        return std::string(buf);
    }
};

inline const NpProfiler& GetInstance() { return NpProfiler::GetInstance(); }

class EventProfiler {
public:
    EventProfiler(const char* fn, const int line, const char* func)
        : mCalls(0),
          mTotalTime(0),
          mDepth(0),
          mFileName(fn),
          mPrettyFunction(func),
          mLineNumber(line) {
        NpProfiler::GetInstance().Reg(this);
    }
    void Enter() {
        mCalls++;
        if (mDepth > 0) {
            double elapsed = Stop();
            mTotalTime += elapsed;  // suspend
            mMaxTime = elapsed > mMaxTime ? elapsed : mMaxTime;
            mMinTime = elapsed < mMaxTime ? elapsed : mMinTime;
        } else {
            mIndent = NpProfiler::GetInstance().Enter();
        }
        mDepth++;
        Start();
    }
    void Exit() {
        double elapsed = Stop();
        mTotalTime += elapsed;
        mMaxTime = elapsed > mMaxTime ? elapsed : mMaxTime;
        mMinTime = mMinTime == 0 ? elapsed : elapsed < mMinTime ? elapsed : mMinTime;
        mDepth--;
        if (mDepth > 0) {
            Start();  // resume
        } else {
            NpProfiler::GetInstance().Exit();
        }
    }
    const std::string& Filename() const { return mFileName; }
    int LineNumber() const { return mLineNumber; }
    const std::string Function() const { return std::string(mIndent, ' ') + mPrettyFunction; }
    int Calls() const { return mCalls; }
    double TotalTime() const { return mTotalTime; }
    double MinTime() const { return mMinTime; }
    double MaxTime() const { return mMaxTime; }

private:
    std::chrono::high_resolution_clock::time_point mStartPoint;
    int mCalls = 0;
    double mTotalTime = 0;
    double mMinTime = 0;
    double mMaxTime = 0;
    int mDepth = 0;
    int mIndent = 0;
    void Start() { mStartPoint = std::chrono::high_resolution_clock::now(); }
    double Stop() {
        std::chrono::duration<double> elapsed =
            (std::chrono::high_resolution_clock::now() - mStartPoint);
        return (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() +
                1e-9 * std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
    }
    const std::string mFileName;
    const std::string mPrettyFunction;
    const int mLineNumber;
};

template <typename T>
void profile_gate(const char* fn, const int line, const char* func, bool enter, T*) {
    static EventProfiler prof(fn, line, func);
    if (enter)
        prof.Enter();
    else
        prof.Exit();
}

inline std::ostream& NpProfiler::Log(std::ostream& o, const char* format, ...) const {
    va_list args;
    va_start(args, format);
    static char buffer[1024];
    vsprintf(buffer, format, args);
    o << buffer;
    va_end(args);
    return o;
}

inline std::ostream& NpProfiler::Print(std::ostream& o, int fnwidth = 60) const {
    o << std::endl
      << "Profiling Summary:" << std::endl
      << "-----------------------------------------------------------------------"
         "------------------"
         "--------"
      << std::endl;
    o << std::internal << std::setw(fnwidth) << "Function" << std::setw(10) << "Calls"
      << std::setw(15) << "Total time" << std::setw(10) << "Avg time" << std::setw(10) << "Min time"
      << std::setw(10) << "Max time" << std::endl
      << "-----------------------------------------------------------------------"
         "------------------"
         "--------"
      << std::endl;
    for (const EventProfiler* p : mProfilers) {
        o << std::internal << std::setw(fnwidth) << p->Function() << std::setw(10) << p->Calls()
          << std::setw(15) << FormatTime(p->TotalTime()) << std::setw(10)
          << FormatTime(p->TotalTime() / p->Calls()) << std::setw(10) << FormatTime(p->MinTime())
          << std::setw(10) << FormatTime(p->MaxTime()) << std::endl;
    }
    return o;
}

inline std::ostream& operator<<(std::ostream& o, const NpProfiler& prof) {
    return prof.Print(o, 120);
}

}  // namespace profiler

#if !defined(NDEBUG)
#define PROFILE_ME_AS(name)                                             \
    NPAGENT_ATRACE_NAME(name);                                          \
    struct __prof_data {                                                \
        bool early_exit;                                                \
        __prof_data(const char* fn, const int line, const char* func) { \
            early_exit = false;                                         \
            profiler::profile_gate(fn, line, func, true, this);         \
        }                                                               \
        void Stop() {                                                   \
            early_exit = true;                                          \
            profiler::profile_gate(nullptr, -1, nullptr, false, this);  \
        }                                                               \
        ~__prof_data() {                                                \
            if (!early_exit) Stop();                                    \
        }                                                               \
    } __helper_var(__FILE__, __LINE__, name)
#define PROFILE_ME  PROFILE_ME_AS(__FUNCTION__)
#define PROFILE_END __helper_var.Stop()
#else
#define PROFILE_ME_AS(name) NPAGENT_ATRACE_NAME(name)
#define PROFILE_ME
#define PROFILE_END
#endif
