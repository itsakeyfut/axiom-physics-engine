@echo off
REM Configure Axiom with Ninja generator for clang-tidy support
REM Requires Visual Studio 2022 and Ninja

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cmake --preset windows-debug-ninja
