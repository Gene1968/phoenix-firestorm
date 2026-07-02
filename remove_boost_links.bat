@echo off
REM Remove hard links for boost libraries (without 'lib' prefix)
REM This tests if CMake can find the lib-prefixed versions after VS update
REM Run this from the project root directory

echo Removing hard links for boost libraries...

pushd build-vc170-64\packages\lib\release

del boost_atomic-mt-x64.lib 2>nul
del boost_chrono-mt-x64.lib 2>nul
del boost_container-mt-x64.lib 2>nul
del boost_context-mt-x64.lib 2>nul
del boost_date_time-mt-x64.lib 2>nul
del boost_fiber-mt-x64.lib 2>nul
del boost_filesystem-mt-x64.lib 2>nul
del boost_iostreams-mt-x64.lib 2>nul
del boost_json-mt-x64.lib 2>nul
del boost_program_options-mt-x64.lib 2>nul
del boost_regex-mt-x64.lib 2>nul
del boost_stacktrace_from_exception-mt-x64.lib 2>nul
del boost_stacktrace_noop-mt-x64.lib 2>nul
del boost_stacktrace_windbg_cached-mt-x64.lib 2>nul
del boost_stacktrace_windbg-mt-x64.lib 2>nul
del boost_system-mt-x64.lib 2>nul
del boost_thread-mt-x64.lib 2>nul
del boost_url-mt-x64.lib 2>nul
del boost_wave-mt-x64.lib 2>nul

popd

echo Hard links removed.
echo.
echo NOTE: You'll need to reconfigure CMake for it to find the lib-prefixed versions:
echo   autobuild configure -A 64 -c ReleaseFS_open


