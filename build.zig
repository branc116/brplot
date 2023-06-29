const std = @import("std");
const glfw = @import("mach-glfw/build.zig");

const rlplot_flags = &[_][]const u8{
    "-std=gnu99",
    "-D_GNU_SOURCE",
    "-DGL_SILENCE_DEPRECATION=199309L",
    "-fno-sanitize=undefined", // https://github.com/raysan5/raylib/issues/1891
};

fn rlf(comptime name: []const u8) []const u8 {
    return "raylib/src/" ++ name;
}

pub fn addRayLib(b: *std.build.Builder, target: std.zig.CrossTarget, opt: std.builtin.Mode) *std.Build.CompileStep {
    const raylib = b.addStaticLibrary(.{ .name = "raylib", .optimize = opt, .target = target });
    const sources = .{ rlf("rcore.c"), rlf("rtext.c"), rlf("rmodels.c"), rlf("rtextures.c"), rlf("utils.c"), rlf("rshapes.c") };
    raylib.addCSourceFiles(&sources, rlplot_flags);
    //    if (b.is_release) {
    //        raylib.defineCMacro("RELEASE", null);
    //        raylib.want_lto = true;
    //    }

    switch (target.getOsTag()) {
        .linux => {
            raylib.addCSourceFiles(&.{"raylib/src/rglfw.c"}, rlplot_flags);
            //raylib.addIncludePath("/usr/include");
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
        },
        else => {
            @panic("Unsupported OS");
        },
    }
    return raylib;
}

// This has been tested to work with zig master branch as of commit 87de821 or May 14 2023
pub fn addRlPlot(b: *std.build.Builder, opt: std.builtin.OptimizeMode, target: std.zig.CrossTarget) error{ OutOfMemory, FailedToLinkGPU, CannotEnsureDependency }!*std.Build.CompileStep {
    const rlplot = b.addExecutable(.{ .name = "rlplot", .optimize = opt, .target = target });
    const raylib = addRayLib(b, target, opt);

    rlplot.addCSourceFiles(&.{ "graph.c", "main.c", "points_group.c", "read_input.c", "refresh_shaders.c", "smol_mesh.c" }, rlplot_flags);
    try glfw.link(b, raylib, .{ .shared = false, .x11 = true, .wayland = true, .opengl = true, .gles = true });
    rlplot.linkLibrary(raylib);
    rlplot.linkLibC();
    rlplot.addIncludePath("raylib/src");
    //rlplot.addIncludePath("/usr/include");

    switch (target.getOsTag()) {
        .linux => {
            rlplot.defineCMacro("PLATFORM_DESKTOP", null);
        },
        .freebsd, .openbsd, .netbsd, .dragonfly => {
            rlplot.defineCMacro("PLATFORM_DESKTOP", null);
        },
        else => {
            @panic("Unsupported OS");
        },
    }

    return rlplot;
}

pub fn build(b: *std.Build) error{ OutOfMemory, FailedToLinkGPU, CannotEnsureDependency }!void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    const lib = try addRlPlot(b, optimize, target);

    b.installArtifact(lib);
}

const srcdir = struct {
    fn getSrcDir() []const u8 {
        return std.fs.path.dirname(@src().file).?;
    }
}.getSrcDir();
