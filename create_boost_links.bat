@echo off
REM Create hard links for boost libraries without 'lib' prefix
REM This allows CMake to find them when searching for boost_context-mt-x64, etc.
REM Hard links don't require admin privileges like symbolic links do
REM Run this from the project root directory

pushd build-vc170-64\packages\lib\release

echo Creating hard links for boost libraries...

mklink /H boost_atomic-mt-x64.lib libboost_atomic-mt-x64.lib
mklink /H boost_chrono-mt-x64.lib libboost_chrono-mt-x64.lib
mklink /H boost_container-mt-x64.lib libboost_container-mt-x64.lib
mklink /H boost_context-mt-x64.lib libboost_context-mt-x64.lib
mklink /H boost_date_time-mt-x64.lib libboost_date_time-mt-x64.lib
mklink /H boost_fiber-mt-x64.lib libboost_fiber-mt-x64.lib
mklink /H boost_filesystem-mt-x64.lib libboost_filesystem-mt-x64.lib
mklink /H boost_iostreams-mt-x64.lib libboost_iostreams-mt-x64.lib
mklink /H boost_json-mt-x64.lib libboost_json-mt-x64.lib
mklink /H boost_program_options-mt-x64.lib libboost_program_options-mt-x64.lib
mklink /H boost_regex-mt-x64.lib libboost_regex-mt-x64.lib
mklink /H boost_stacktrace_from_exception-mt-x64.lib libboost_stacktrace_from_exception-mt-x64.lib
mklink /H boost_stacktrace_noop-mt-x64.lib libboost_stacktrace_noop-mt-x64.lib
mklink /H boost_stacktrace_windbg_cached-mt-x64.lib libboost_stacktrace_windbg_cached-mt-x64.lib
mklink /H boost_stacktrace_windbg-mt-x64.lib libboost_stacktrace_windbg-mt-x64.lib
mklink /H boost_system-mt-x64.lib libboost_system-mt-x64.lib
mklink /H boost_thread-mt-x64.lib libboost_thread-mt-x64.lib
mklink /H boost_url-mt-x64.lib libboost_url-mt-x64.lib
mklink /H boost_wave-mt-x64.lib libboost_wave-mt-x64.lib

popd
echo Done! Hard links created.
