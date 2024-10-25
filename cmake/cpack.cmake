# CPack configuration

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "StableDiffusionGUI")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Ferenc Szonta'gh <szf@fsociety.hu>")
set(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/graphics/icons/256/stablediffusiongui.png)
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_PACKAGE_VENDOR "Ferenc Szontágh")

set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN TRUE)
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED FALSE)


set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "${CMAKE_PROJECT_NAME}")

set(bin_INSTALL_PATH_NSIS "bin")
set(lib_INSTALL_PATH_NSIS "lib")

if (WIN32)
	set(bin_INSTALL_PATH_NSIS ".")
	set(lib_INSTALL_PATH_NSIS ".")
endif()


if (UNIX AND NOT APPLE)
    execute_process(
        COMMAND lsb_release -si
        OUTPUT_VARIABLE DISTRO_NAME
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND lsb_release -sr
        OUTPUT_VARIABLE DISTRO_VER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(DISTRO_VERSION "${DISTRO_NAME}${DISTRO_VER}")
endif()

# CPack settings
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")


# Ensure the version numbers
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_EXECUTABLES "sd_ui;Stable Diffusion GUI")
set(CPACK_STRIP_FILES TRUE)

#set_property(TARGET sd_ui PROPERTY VERSION "${PROJECT_VERSION}")
set_property(TARGET sd_ui PROPERTY SOVERSION "${GIT_HASH}")
set_property(TARGET sd_ui PROPERTY DESCRIPTION "${CMAKE_PROJECT_DESCRIPTION}")
set_property(TARGET sd_ui PROPERTY URL "${CMAKE_PROJECT_HOMEPAGE_URL}")
set_property(TARGET sd_ui PROPERTY APPIMAGE_DESKTOP_FILE ${CMAKE_BINARY_DIR}/stablediffusiongui.desktop)
set_property(TARGET sd_ui PROPERTY APPIMAGE_ICON_FILE ${CMAKE_SOURCE_DIR}/graphics/icons/256/stablediffusiongui.png)



install(TARGETS sd_ui DESTINATION ${bin_INSTALL_PATH_NSIS} COMPONENT "${CMAKE_PROJECT_NAME}")
list(APPEND CPACK_COMPONENTS_ALL "${CMAKE_PROJECT_NAME}")

if (SDGUI_AVX)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_avx${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS} 
	COMPONENT libsdcpp_avx)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_avx")
    set(CPACK_DEBIAN_LIBSDCPP_AVX_PACKAGE_NAME "libstablediffusion-avx-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_AVX_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()

if (SDGUI_AVX2)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_avx2${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS}  
	COMPONENT libsdcpp_avx2)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_avx2")
    set(CPACK_DEBIAN_LIBSDCPP_AVX2_PACKAGE_NAME "libstablediffusion-avx2-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_AVX2_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()

if (SDGUI_AVX512)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_avx512${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS}  
	COMPONENT libsdcpp_avx512)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_avx512")
    set(CPACK_DEBIAN_LIBSDCPP_AVX512_PACKAGE_NAME "libstablediffusion-avx512-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_AVX512_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()

if (SDGUI_CUBLAS)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_cuda${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS}  
	COMPONENT libsdcpp_cuda)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_cuda")
    set(CPACK_DEBIAN_LIBSDCPP_CUDA_PACKAGE_NAME "libstablediffusion-cuda-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_CUDA_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()

if (SDGUI_ROCM)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_rocm${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS}  
	COMPONENT libsdcpp_rocm)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_rocm")
    set(CPACK_DEBIAN_LIBSDCPP_ROCM_PACKAGE_NAME "libstablediffusion-rocm-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_ROCM_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()

if (SDGUI_VULKAN)
	install(FILES ${CMAKE_BINARY_DIR}/${EPREFIX}${CMAKE_SHARED_LIBRARY_PREFIX}stable-diffusion_vulkan${CMAKE_SHARED_LIBRARY_SUFFIX}
	DESTINATION ${lib_INSTALL_PATH_NSIS}  
	COMPONENT libsdcpp_vulkan)
	list(APPEND CPACK_COMPONENTS_ALL "libsdcpp_vulkan")
    set(CPACK_DEBIAN_LIBSDCPP_VULKAN_PACKAGE_NAME "libstablediffusion-vulkan-${SDCPP_VERSION}")
    set(CPACK_DEBIAN_LIBSDCPP_VULKAN_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
endif()


if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME ${CMAKE_PROJECT_NAME})
    set(CPACK_NSIS_PACKAGE_NAME "${CMAKE_PROJECT_NAME} ${PROJECT_VERSION}")
    set(CPACK_NSIS_URL_INFO_ABOUT ${CMAKE_PROJECT_HOMEPAGE_URL})
    set(CPACK_NSIS_HELP_LINK "https://github.com/fszontagh/sd.cpp.gui.wx")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/graphics/icons/256/cube.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/graphics/icons/256/cube.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "StableDiffusionGUI.exe")
    
elseif(UNIX AND NOT APPLE)

    set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}-${DISTRO_VERSION}")

    string(TIMESTAMP TODAY "%Y%m%d")
    
    set(CPACK_GENERATOR "DEB;TGZ")
    #set(CPACK_GENERATOR "External")
    
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    
    # DEB package settings
    set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ferenc Szontágh <szf@fsociety.hu>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.29), libstdc++6 (>= 9), stablediffusiongui (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION})")
    set(CPACK_DEBIAN_STABLEDIFFUSIONGUI_PACKAGE_DEPENDS "libc6 (>= 2.29), libstdc++6 (>= 9), libgtk-3-0 (>= 3.9.10) | libgtk-4-1, libudev1 (>= 183), libvulkan1, libx11-6, libstablediffusion-avx-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION}) | libstablediffusion-avx2-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION}) | libstablediffusion-avx512-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION}) | libstablediffusion-cuda-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION}) | libstablediffusion-rocm-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION}) | libstablediffusion-vulkan-${SDCPP_VERSION} (=${CPACK_PACKAGE_VERSION}-${TODAY}~${DISTRO_VERSION})")
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION ${CMAKE_PROJECT_DESCRIPTION})
    set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_DEBIAN_PACKAGE_RELEASE "${TODAY}~${DISTRO_VERSION}")
    set(CPACK_DEBIAN_STABLEDIFFUSIONGUI_PACKAGE_NAME "stablediffusiongui")


    set(SD_UI_EXEC "sd_ui")
    include(GNUInstallDirs)
    set(APPIMAGE_SOURCE_DIR "${CMAKE_BINARY_DIR}/AppImageSource")

    
    # Generate the .desktop file by replacing placeholders
    configure_file(${CMAKE_SOURCE_DIR}/platform/linux/app.desktop ${CMAKE_BINARY_DIR}/stablediffusiongui.desktop @ONLY)
    
    install(FILES ${CMAKE_BINARY_DIR}/stablediffusiongui.desktop DESTINATION share/applications COMPONENT "${CMAKE_PROJECT_NAME}")
    install(FILES ${CMAKE_SOURCE_DIR}/graphics/icons/256/stablediffusiongui.png DESTINATION share/icons/hicolor/256x256/apps COMPONENT "${CMAKE_PROJECT_NAME}")
    include(${CMAKE_SOURCE_DIR}/cmake/AppImage.cmake)
elseif(APPLE)
    #set(CPACK_GENERATOR "DragNDrop")
    #set(CPACK_DMG_VOLUME_NAME "StableDiffusionGUI ${PROJECT_VERSION}")
    #set(CPACK_DMG_DS_STORE_FILE ${CMAKE_SOURCE_DIR}/platform/mac/ds_store)
endif()


include(CPack)


cpack_add_component_group(backends
	DISPLAY_NAME "SD.cpp backends"
	DESCRIPTION "The Stable Diffusion C++ backends"
)

cpack_add_component("${CMAKE_PROJECT_NAME}"
	DISPLAY_NAME "SD C++ GUI"
	DESCRIPTION "Stable Diffusion CPP Desktop Graphical User Interface"
)


set(EPREFIX "")

if (WIN32)
	set(EPREFIX "${CMAKE_BUILD_TYPE}/")
endif()




if (SDGUI_AVX)
    cpack_add_component(libsdcpp_avx
        DISPLAY_NAME "SD C++ -AVX"
        DESCRIPTION "SD CPU backend with AVX CPU feature"
    )
endif()

if (SDGUI_AVX2)
    cpack_add_component(libsdcpp_avx2
        DISPLAY_NAME "SD C++ -AVX2"
        DESCRIPTION "SD CPU backend with AVX2 CPU feature"
    )
endif()

if (SDGUI_AVX512)
    cpack_add_component(libsdcpp_avx512
        DISPLAY_NAME "SD C++ -AVX512"
        DESCRIPTION "SD CPU backend with AVX512 CPU feature"
    )
endif()

if (SDGUI_CUBLAS)
    cpack_add_component(libsdcpp_cuda
        DISPLAY_NAME "SD C++ -CUDA"
        DESCRIPTION "SD CPU backend with CUDA feature"
    )
endif()

if (SDGUI_ROCM)
    cpack_add_component(libsdcpp_rocm
        DISPLAY_NAME "SD C++ -ROCM"
        DESCRIPTION "SD CPU backend with ROCM feature"
    )
endif()

if (SDGUI_VULCAN)
    cpack_add_component(libsdcpp_vulcan
        DISPLAY_NAME "SD C++ -VULCAN"
        DESCRIPTION "SD CPU backend with VULCAN feature"
    )
endif()


message("CPACK_COMPONENTS_ALL: ${CPACK_COMPONENTS_ALL}")