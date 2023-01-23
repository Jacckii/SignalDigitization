@echo off
title Web assembly build script

echo Build script for porting C++ to WebAssembly

echo Checking for Ninja in PATH...
where ninja >nul
if %errorlevel% neq 0 (
  echo Error: Ninja is not in PATH. Please install Ninja and try again.
  goto :eof
)

echo Checking for Emscripten in PATH...
where emcc >nul
if %errorlevel% neq 0 (
  echo Error: Emscripten is not in PATH. Please install Emscripten and try again.
  goto :eof
)

echo Checking for web folder...
if not exist web mkdir web
cd web

echo Running emcmake cmake..
call emcmake cmake .. -G ninja

echo Running Ninja...
call ninja -j4
echo ninja finished

echo Build complete.

cd ..
