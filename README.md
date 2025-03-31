# stablediffusiongui-win-ROCm6

- stablediffusiongui-windows AMD ROCm 6
ROCm is x2 faster than vulkan.

    [Binaries for Radeon](https://github.com/githubcto/sd.cpp.gui.wx-win-ROCm6/releases) RX 7900XTX , 7900XT, 7800XT, 7700XT, 7600, 6950XT, 6900XT, 6800XT and 6800.

    Other Radeon, use vulkan.

- INSTALL
1. You need to install [AMD HIP SDK](https://www.amd.com/en/developer/resources/rocm-hub/hip-sdk.html)
2. Download [binaries for Radeon](https://github.com/githubcto/sd.cpp.gui.wx-win-ROCm6/releases)
3. Extract zip.
4. Double click StableDiffusionGUI-x.x-win64.exe. This exe is installer for windows. I recommend NOT install CUDA Backends. stable-diffusion_cuda.dll is unnecessary, file size is very large.
5. Make .bat's desktop shortcut if needed.
   
    bat files are in installed folder which you specified when you install.


- START

  Double click stablediffusiongui.exe.
  
  If AMD HIP SDK is detected (HIP_PATH is detected in PATH), backend become ROCm HIP, stable-diffusion_hipblas.dll.
If not, backend is vulkan, stable-diffusion_vulkan.dll.
(HIP_PATH is configured by AMD HIP SDK installer automatically. So, you do not need configure HIP_PATH and HIP_PATHbin manually.)

    Depend on your PC, you may need to configure HIP_VISIBLE_DEVICES. Two sample .bat files are included. Try them!

These are commandline options. -avx, -avx2, -avx512, -hipblas, -vulkan, -cuda.
hipblas means ROCm. cuda means nvidia.
For example, if you want to run FORCE VULKAN, add -vulkan. Sample .bat file is included.

- UNINSTALL

    Use windows OS's uninstaller or use Uninstall.exe .

- KNOWN ISSUE

    SDXL speed is slow: Compared with Comfy, SDXL's speed is slow, takes x2 sec/it. though, FLUX speed is slightly faster then Comfy.This SDXL-SLOW issue is sd.cpp's.
radeon, nvidia, intel, and even cpu are affected.



## **sd.cpp.gui.wx - Stable Diffusion GUI**

A cross-platform GUI for [Stable Diffusion C++](https://github.com/leejet/stable-diffusion.cpp), built using [wxWidgets](https://www.wxwidgets.org/).

Licensed under the MIT License.

[![Release and Check build on multiple platforms](https://github.com/fszontagh/sd.cpp.gui.wx/actions/workflows/multiplatform.yml/badge.svg?event=push)](https://github.com/fszontagh/sd.cpp.gui.wx/actions/workflows/multiplatform.yml)

### **Features**

-   **Text-to-image (text2img)** generation
-   **Image-to-image (img2img)** generation
-   **Inpaint & Outpaint**, save / load masks, built-in inpaint / outpaint editor
-   **Tiling** for low memory
-   **Headless** computation with [SDGUI Server](https://github.com/fszontagh/sd.cpp.gui.wx/wiki/SD-GUI-Server) even in containerized mode
-   Model **quantization** in-fly or manually
-   Built-in **upscaling** capabilities
-   **ControlNet** integration
-   [**Model conversion** to GGUF format](https://github.com/fszontagh/sd.cpp.gui.wx/wiki/Convert-safetensors-model-into-gguf-(aka.-quantization))
-   Optional integration with [CivitAi Model Downloader](https://github.com/fszontagh/sd.cpp.gui.wx/wiki/CivitAi-Integration)
-   Hardware detection for optimal diffusion performance
-   Integrated **model management** tools
-   **VAE** support
-   **TAESD** support
-   Customizable **generation presets**
-   Customizable [**output file names**](https://github.com/fszontagh/sd.cpp.gui.wx/wiki/GUI-howtos#image-output-filename-format)
-   ComfyUI compatible [**Prompt styles**](https://github.com/fszontagh/sd.cpp.gui.wx/wiki/GUI-howtos#prompt-presets--styles)
-   **Generation queue** for batch processing
-   Real-time **progress monitoring** during generation
-   Save and load metadata directly from images
-   Multi-language GUI support

#### **Supported Backends**

-   CPU (AVX, AVX2, AVX512)
-   CUDA
-   VULKAN

----------

### **OS Compatibility**

Refer to the [GitHub Releases](https://github.com/fszontagh/sd.cpp.gui.wx/releases) page for available installers.  
For compiling on unsupported operating systems, consult the Wiki.


---



----------

_Use at your own risk!_  
These releases are preview versions and may contain bugs. If you encounter issues, please report them via the [GitHub Issues](https://github.com/fszontagh/sd.cpp.gui.wx/issues) page.
