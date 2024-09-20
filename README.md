![Firestorm Viewer Logo](doc/firestorm_256.png)

**[Firestorm](https://www.firestormviewer.org/) is a free client for 3D virtual worlds such as Second Life and various OpenSim worlds where users can create, connect and chat with others from around the world.** This repository contains the official source code for the Firestorm viewer.

## Open Source

Firestorm is a third party viewer derived from the official [Second Life](https://github.com/secondlife/viewer) client. The client codebase has been open source since 2007 and is available under the LGPL license.

## Download

Pre-built versions of the viewer releases for Windows, Mac and Linux can be downloaded from the [official website](https://www.firestormviewer.org/choose-your-platform/).

## Build Instructions

Build instructions for each operating system can be found using the links below and in the official [wiki](https://wiki.firestormviewer.org/).

- [Windows](doc/building_windows.md)
- [Mac](doc/building_macos.md)
- [Linux](doc/building_linux.md)

> [!NOTE]
> We do not provide support for compiling the viewer or issues resulting from using a self-compiled viewer. However, there is a self-compilers group within Second Life that can be joined to ask questions related to compiling the viewer: [Firestorm Self Compilers](https://tinyurl.com/firestorm-self-compilers)

## Contribute

Help make Firestorm better! You can get involved with improvements by filing bugs and suggesting enhancements via [JIRA](https://jira.firestormviewer.org) or [creating pull requests](doc/FS_PR_GUIDELINES.md).



## ShareStorm Features

Successfully building for Windows from the currently latest FireStorm 7.1.9, added a number of the enhancements, working on more.  Verified on OS so far.

Some OS regions (eg: Arcadia Store on OSG) somehow detect that this isn't the official build, regardless of any spoofing, even when it was built from only pure FS source.  The only difference I knew of is the patch number such as 74804 or 74745 in "Phoenix-FirestormOS-Release64-7-1-9-74804_Setup.exe," vs 9209 from their last actual release (or maybe since we can't NSI-sign the binary?).  Seems lame that they ban nightly builders and future releases.  For now I've left out my config/app/pkg/file renaming in hopes of coming back to solve this.  Perhaps when FS puts out their next official release I'll rebase to that, and if it helps then I'll stop pulling more often.  But then I bet Aardvark has to update her region script with every release as well.  Thumbs down.  Hey I'm not there to steal - just trying to browse with more utilities to inspect & learn to improve my builds.

Implemented and confirmed working:
- Inspect Textures! / floater
- Teleport to Safety Ctrl+PgUp
- Teleport to Ground Ctrl+PgDn
- Export DAE
- Import DAE
- Export OXP
- Import OXP
- Inject Particles
- Avatar Textures


Testing and working on:
- Import XML
- test in SL?
- Export object as OBJ
- lltexturectrl

Assessing:
- XML Export
- Login Spoofing
- Import OBJ
- bulk import varieties
- AddParticles
- Measure
- (Rip Selected Objects Particles Alt+Shift+P)
- Particle Explorer
- Send to &gt;
	- OffWorld
	- Inventory
	- Owner
	- Take Copy
	- Trash
	- Explode
- more

! I hate pie menus, so I work first on the regular rt-click menu and then maybe copy stuff over if I don't forget.\ \
! Like most every other attempt at this, I've only done the default skin.  Could be persuaded to fix more skins or take your pull request, but someone else must test. \ \
! Like most every other attempt at this, I've only done EN (or US-EN depending on the build).  I could be asked nicely to apply to your language too if it helps you and if you test for me.  :) \ \
