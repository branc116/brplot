#!/usr/bin/env sh
set -ex

wayland-scanner -s client-header ../glfw-3.4/deps/wayland/fractional-scale-v1.xml external/wayland/fractional-scale-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/idle-inhibit-unstable-v1.xml external/wayland/idle-inhibit-unstable-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/pointer-constraints-unstable-v1.xml external/wayland/pointer-constraints-unstable-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/relative-pointer-unstable-v1.xml external/wayland/relative-pointer-unstable-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/viewporter.xml external/wayland/viewporter.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/wayland.xml external/wayland/wayland.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/xdg-activation-v1.xml external/wayland/xdg-activation-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/xdg-decoration-unstable-v1.xml external/wayland/xdg-decoration-unstable-v1.h
wayland-scanner -s client-header ../glfw-3.4/deps/wayland/xdg-shell.xml external/wayland/xdg-shell.h

wayland-scanner -s private-code ../glfw-3.4/deps/wayland/fractional-scale-v1.xml external/wayland/fractional-scale-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/idle-inhibit-unstable-v1.xml external/wayland/idle-inhibit-unstable-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/pointer-constraints-unstable-v1.xml external/wayland/pointer-constraints-unstable-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/relative-pointer-unstable-v1.xml external/wayland/relative-pointer-unstable-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/viewporter.xml external/wayland/viewporter-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/wayland.xml external/wayland/wayland-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/xdg-activation-v1.xml external/wayland/xdg-activation-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/xdg-decoration-unstable-v1.xml external/wayland/xdg-decoration-unstable-v1-private.c
wayland-scanner -s private-code ../glfw-3.4/deps/wayland/xdg-shell.xml external/wayland/xdg-shell-private.c

