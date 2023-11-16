NAME = TICRAFT
DESCRIPTION = "Perspective Correct 3D Minecraft"
COMPRESSED = YES
COMPRESSED_MODE = zx7
ARCHIVED = YES
BSSHEAP_LOW = D031F6
BSSHEAP_HIGH = D13FD8
CFLAGS = -O3 -ffast-math -fapprox-func
CXXFLAGS = -O3 -ffast-math -fapprox-func
ICON = logo.png
PREFER_OS_LIBC = YES
include $(shell cedev-config --makefile)