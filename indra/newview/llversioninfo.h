/**
 * @file llversioninfo.h
 * @brief Routines to access the viewer version and build information
 * @author Martin Reddy
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#ifndef LL_LLVERSIONINFO_H
#define LL_LLVERSIONINFO_H

#include "stdtypes.h"
#include "llsingleton.h"
#include <string>
#include <memory>

class LLEventMailDrop;
template <typename T>
class LLStoreListener;

///
/// This API provides version information for the viewer.  This
/// includes access to the major, minor, patch, and build integer
/// values, as well as human-readable string representations. All
/// viewer code that wants to query the current version should
/// use this API.
///
class LLVersionInfo: public LLSingleton<LLVersionInfo>
{
    LLSINGLETON(LLVersionInfo);
    void initSingleton() override;
public:
    ~LLVersionInfo();

    /// return the major version number as an integer
    S32 getMajor() const;

    /// return the minor version number as an integer
    S32 getMinor() const;

    /// return the patch version number as an integer
    S32 getPatch() const;

    /// return the build number as an integer
    U64 getBuild() const;

    /// return the full viewer version as a string like "2.0.0.200030"
    std::string getVersion() const;

//<FS:CZ>
    /// return the full viewer version as a string like "200030"
    std::string getBuildVersion() const;
//</FS:CZ>

    /// return the viewer version as a string like "2.0.0"
    std::string getShortVersion() const;

    /// return the viewer version and channel as a string
    /// like "Second Life Release 2.0.0.200030"
    std::string getChannelAndVersion();

    //<FS:TS> Needed for fsdata version checking
    /// return the viewer version and hardcoded channel as a string
    /// like "Firestorm-Release 2.0.0 (200030)"
    std::string getChannelAndVersionFS() const;

    /// return the channel name, e.g. "Second Life"
    std::string getChannel() const;

    /// return the CMake build type
    std::string getBuildConfig() const;

    /// reset the channel name used by the viewer.
    void resetChannel(const std::string& channel);

// [SL:KB] - Patch: Viewer-CrashReporting | Checked: 2011-05-08 (Catznip-2.6.0a) | Added: Catznip-2.6.0a
    /// Return the platform the viewer was built for
    std::string getBuildPlatform() const;
// [/SL:KB]

    /// return the bit width of an address
    S32 getAddressSize() const { return ADDRESS_SIZE; }

    typedef enum
    {
        TEST_VIEWER,
        PROJECT_VIEWER,
        BETA_VIEWER,
        RELEASE_VIEWER
    } ViewerMaturity;
    ViewerMaturity getViewerMaturity() const;

// <FS:Beq> Add an FS specific viewer maturity enum
    using FSViewerMaturity = 
    enum class FSViewerMaturityEnum
    {
        UNOFFICIAL_VIEWER=0,
        ALPHA_VIEWER,
        MANUAL_VIEWER,
        BETA_VIEWER,
        NIGHTLY_VIEWER,
        RELEASE_VIEWER,
        STREAMING_VIEWER,
    };
    FSViewerMaturity getFSViewerMaturity() const;
// </FS:Beq>

    /// get the release-notes URL, once it becomes available -- until then,
    /// return empty string
    std::string getReleaseNotes() const;

    static std::string getGitHash();
private:
    std::string version;
    std::string short_version;
    /// Storage of the channel name the viewer is using.
    //  The channel name is set by hardcoded constant,
    //  or by calling resetChannel()
    std::string mWorkingChannelName;
    // Storage for the "version and channel" string.
    // This will get reset too.
    mutable std::string mVersionChannel;
    std::string build_configuration;
    std::string mReleaseNotes;
    // Store unique_ptrs to the next couple things so we don't have to explain
    // to every consumer of this header file all the details of each.
    // mPump is the LLEventMailDrop on which we listen for SLVersionChecker to
    // post the release-notes URL from the Viewer Version Manager.
    std::unique_ptr<LLEventMailDrop> mPump;
    // mStore is an adapter that stores the release-notes URL in mReleaseNotes.
    std::unique_ptr<LLStoreListener<std::string>> mStore;
};

#endif
