<img align="left" width="100" height="100" src="doc/firestorm_256.png" alt="Logo of Firestorm viewer"/>

**[Firestorm](https://www.firestormviewer.org) is a free client for 3D virtual worlds such as Second Life and various OpenSim worlds where users can create, connect and chat with others from around the world.**

This repository contains the official source code for the Firestorm viewer.

## Open Source

Firestorm is a third party viewer derived from the official [Second Life](https://github.com/secondlife/viewer) client. The client codebase has been open source since 2007 and is available under the LGPL license.

## Download

Pre-built versions of the viewer releases for Windows, Mac and Linux can be downloaded from the [official website](https://www.firestormviewer.org/choose-your-platform/).

## Build Instructions

Build instructions for each operating system can be found using the links below and in the official [wiki](https://wiki.firestormviewer.org).

- [Windows](doc/building_windows.md)
- [Mac](doc/building_macos.md)
- [Linux](doc/building_linux.md)

> [!NOTE]
> We do not provide support for compiling the viewer or issues resulting from using a self-compiled viewer. However, there is a self-compilers group within Second Life that can be joined to ask questions related to compiling the viewer: [Firestorm Self Compilers](https://tinyurl.com/firestorm-self-compilers)

## Contribute

Help make Firestorm better! You can get involved with improvements by filing bugs and suggesting enhancements via [JIRA](https://jira.firestormviewer.org) or [creating pull requests](CONTRIBUTING.md).

## Community respect

This section is guided by the [TPV Policy](https://secondlife.com/corporate/third-party-viewers) and the [Second Life Code of Conduct](https://github.com/secondlife/viewer?tab=coc-ov-file).

Firestorm code is made available during ongoing development, with the **master** branch representing the current nightly build. Developers and self-compilers are encouraged to work on their own forks and contribute back via pull requests, as detailed in the [contributing guide](CONTRIBUTING.md).

If you intend to use our code for your own viewer beyond personal use, please only use code from official release branches (for example, `Firestorm_7.1.13`), rather than from pre-release/preview or nightly builds.



## ShareStorm Features

Successfully building for Windows from the currently latest FireStorm 7.1.9, added a number of the enhancements, working on more.  Verified on OS so far.

Some OS regions (eg: Arcadia Store on OSG) somehow detect that this isn't the official build, regardless of any spoofing, even when it was built from only pure FS source.  The only difference I knew of is the patch number such as 74804 or 74745 in "Phoenix-FirestormOS-Release64-7-1-9-74804_Setup.exe," vs 9209 from their last actual release (or maybe since we can't NSI-sign the binary?).  Seems lame that they ban nightly builders and future releases.  For now I've left out my config/app/pkg/file renaming in hopes of coming back to solve this.  Perhaps when FS puts out their next official release I'll rebase to that, and if it helps then I'll stop pulling more often.  But then I bet Aardvark has to update her region script with every release as well.  Thumbs down.  Hey I'm not there to steal - just trying to browse with more utilities to inspect & learn to improve my builds.

Implemented and confirmed working:
- Toggle Hacked Godmode
- Inspect Textures! / floater / Copy to Inv
- Export DAE with all textures! (combine with Inspect Textures -> "Copy All to Inventory." putting [temp.]textures in your inventory. Then when you click back on the object, those will show in the export floater as co-exportable!  (sometimes minus a single one))
- Import DAE with textures
- Export OXP for non-mesh
- Import OXP
- Teleport to Safety Ctrl+PgUp
- Teleport to Ground Ctrl+PgDn
- Inject Particles
- Avatar Textures
- Both OS & SL
- Login Spoofing - thanks to LOstorm!
- all other LOstorm 16 features

Testing and working on:
- Export XML Export
- Import XML
- Export object as OBJ
- lltexturectrl

Assessing missing code:
- Import OBJ
- bulk import varieties
- AddParticles
- Measure
- (Rip Selected Objects Particles Alt+Shift+P)
- Particle Explorer
- Send to >
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
