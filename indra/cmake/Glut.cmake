# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (WINDOWS)
    use_prebuilt_binary(freeglut)
    set(GLUT_LIBRARY
        debug freeglut_static.lib
        optimized freeglut_static.lib)
endif (WINDOWS)
