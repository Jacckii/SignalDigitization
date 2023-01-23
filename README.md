# Signal digitalization thesis
This is part of my theis on demonstration of analog signal digitalization.   
Supports Web Assembly & Windows builds.  
C++ 20 standards

![preview](preview.jpg)

Windows version dependencies:
 - DirectX 10

Web assembly version dependencies:
 - browser with WASM support
 - browser with WebGL 2 support
 - browser with javascript support 

Libraries used:
 - Dear ImGui - https://github.com/ocornut/imgui
 - ImPlot - https://github.com/epezent/implot
 - ImGuiFileDialog - https://github.com/aiekick/ImGuiFileDialog
 - MagicEnum - https://github.com/Neargye/magic_enum
 - exprtk - https://github.com/ArashPartow/exprtk
 - nlohmann::json - https://github.com/nlohmann/json
 - Emscripten - https://github.com/emscripten-core/emscripten

ImGui and ImPlot libraries were slightly modified for this application.  
Please be aware that if you include any other version of these libraries, this app won't work the same!

Features:
 - Add multiple sources of "analog" signal
 - Input mathematic expression as input source
 - Render plots with real time generated data
 - Show sampling step of digitalization
 - Show quantization step of digitalization
 - Show digital data in main plot
 - Show digital data as string
 - export digital data into .csv file
 - Change value format for digital data output
 - Realistic noise generator
 - Extra gaussian noise for sampled data
 - Sync sampled timing
 - Show qunatization levels
 - Show quantization limits
 - Quantization bit prcession
 - GUI & plot customizations

## How to build:

### Windows .exe:
This version will output .exe application for x64 platforms
 - Build working under platform toolkit: v142
 - SDK: Windows SDK 10.0.22000.0

You will need Windows DirectX 10 SDK

```
git clone https://github.com/Jacckii/SignalDigitalization
```
open signal_digitalization.sln in Visual studio 2019 and newver  
Build GUI library first!  
Then build App application.  
Output should be in `\signal_digitalization\x64\Release\App.exe`


### Web assembly:
This will compile an Web assembly that can be run in browser platform independent, but these install steps are made for windows!
It should be possible to compile it on other platforms as well, for that see documentation of Emenscripten and Ninja

Install Emenscripten and follow install instruction from DOCs: https://emscripten.org/docs/getting_started/downloads.html  
Make sure Emenscripten is in your `%PATH%`  

Install Ninja https://ninja-build.org/  
you can download pre-build Ninja https://github.com/ninja-build/ninja/releases/  
then put your Ninja.exe into `C:\Ninja\` or somewhere else, and add that path to `%PATH%`  

now you can run `build.bat` if you're running Windows.   
Or you can follow these steps to do it manually:

```
mkdir web
cd web
emcmake cmake ..
ninja -j4
```
This will output 2 files 
`index.js` and `index.wasm`  

now all you need is to coppy those 2 files and `index.html` that can be found in root directory of this repo on some web server with apache or nginx.

