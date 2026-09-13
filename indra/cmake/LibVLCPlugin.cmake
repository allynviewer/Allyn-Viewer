# -*- cmake -*-
include(Linking)
include(Prebuilt)
include(Variables)

if (LIBVLCPLUGIN)
if (USESYSTEMLIBS)
else (USESYSTEMLIBS)
    use_prebuilt_binary(vlc-bin)
    set(VLC_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/vlc)
endif (USESYSTEMLIBS)

if (WINDOWS)
    set(VLC_PLUGIN_LIBRARIES
        libvlc.lib
        libvlccore.lib
    )
endif (WINDOWS)
endif (LIBVLCPLUGIN)
