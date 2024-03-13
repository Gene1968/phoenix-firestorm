/** 
 * @file llluamanager.h
 * @brief classes and functions for interfacing with LUA. 
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#ifndef LL_LLLUAMANAGER_H
#define LL_LLLUAMANAGER_H

#include "llcoros.h"
#include "llsd.h"
#include <functional>
#include <string>
#include <utility>                  // std::pair

#include "luau/lua.h"
#include "luau/lualib.h"

class LuaState;

class LLLUAmanager
{
public:
    // Pass a callback with this signature to obtain the error message, if
    // any, from running a script or source string. Empty msg means success.
    typedef std::function<void(std::string msg)> script_finished_fn;
    // Pass a callback with this signature to obtain the result, if any, of
    // running a script or source string.
    // count <  0 means error, and result.asString() is the error message.
    // count == 0 with result.isUndefined() means the script returned no results.
    // count == 1 means the script returned one result.
    // count >  1 with result.isArray() means the script returned multiple
    //            results, represented as the entries of the result array.
    typedef std::function<void(int count, const LLSD& result)> script_result_fn;

    static void runScriptFile(const std::string &filename, script_finished_fn cb = {});
    static void runScriptFile(const std::string &filename, script_result_fn cb);
    static void runScriptFile(LuaState& L, const std::string &filename, script_result_fn cb = {});
    // Start running a Lua script file, returning an LLCoros::Future whose
    // get() method will pause the calling coroutine until it can deliver the
    // (count, result) pair described above. Between startScriptFile() and
    // Future::get(), the caller and the Lua script coroutine will run
    // concurrently.
    static LLCoros::Future<std::pair<int, LLSD>>
        startScriptFile(LuaState& L, const std::string& filename);
    // Run a Lua script file, and pause the calling coroutine until it completes.
    // The return value is the (count, result) pair described above.
    static std::pair<int, LLSD> waitScriptFile(LuaState& L, const std::string& filename);

    static void runScriptLine(const std::string &chunk, script_finished_fn cb = {});
    static void runScriptLine(const std::string &chunk, script_result_fn cb);
    static void runScriptLine(LuaState& L, const std::string &chunk, script_result_fn cb = {});
    // Start running a Lua chunk, returning an LLCoros::Future whose
    // get() method will pause the calling coroutine until it can deliver the
    // (count, result) pair described above. Between startScriptLine() and
    // Future::get(), the caller and the Lua script coroutine will run
    // concurrently.
    static LLCoros::Future<std::pair<int, LLSD>>
        startScriptLine(LuaState& L, const std::string& chunk);
    // Run a Lua chunk, and pause the calling coroutine until it completes.
    // The return value is the (count, result) pair described above.
    static std::pair<int, LLSD> waitScriptLine(LuaState& L, const std::string& chunk);

    static void runScriptOnLogin();
};

class LLRequireResolver
{
 public:
    static void resolveRequire(lua_State *L, std::string path);

 private:
    std::string mPathToResolve;
    std::string mSourceChunkname;

    LLRequireResolver(lua_State *L, const std::string& path);

    void findModule();
    lua_State *L;

    bool findModuleImpl(const std::string& absolutePath);
    void runModule(const std::string& desc, const std::string& code);
};
#endif