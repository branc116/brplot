const std = @import("std");

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
    raylib.linkLibC();
    const sources = .{ rlf("rcore.c"), rlf("rtext.c"), rlf("rmodels.c"), rlf("rtextures.c"), rlf("utils.c"), rlf("rshapes.c") };
    raylib.addCSourceFiles(&sources, rlplot_flags);

    switch (target.getOsTag()) {
        .linux => {
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
            raylib.linkSystemLibrary("glfw");
        },
        else => {
            @panic("Unsupported OS");
        },
    }
    return raylib;
}

const print = std.debug.print;

fn one_shader(f: std.fs.File, comptime name: []const u8, comptime macroName: []const u8) !void {
    const file = @embedFile("shaders/" ++ name);
    _ = try f.write("\n#define " ++ macroName ++ "\\\n  \"");
    for (file) |v| {
        if (v == '\n') {
            _ = try f.write("\\n\" \\\n  \""); // `\n"<newline>  "`
        } else {
            _ = try f.write(&[1]u8{v});
        }
    }
    _ = try f.write("\"\n");
}

fn genShaderhFile() !void {
    std.fs.cwd().deleteFile("shaders.h") catch |e| print("{}", .{e});
    const file = try std.fs.cwd().createFile("shaders.h", .{ .read = true });
    _ = try one_shader(file, "grid.fs", "SHADER_GRID_FS");
    _ = try one_shader(file, "line.fs", "SHADER_LINE_FS");
    _ = try one_shader(file, "line.vs", "SHADER_LINE_VS");
}

// This has been tested to work with zig master branch as of commit 87de821 or May 14 2023
pub fn addRlPlot(b: *std.build.Builder, opt: std.builtin.OptimizeMode, target: std.zig.CrossTarget) !*std.Build.CompileStep {
    const rlplot = b.addExecutable(.{ .name = "rlplot", .optimize = opt, .target = target });
    const raylib = addRayLib(b, target, opt);

    rlplot.linkLibrary(raylib);
    rlplot.linkLibC();
    rlplot.addIncludePath("raylib/src");
    rlplot.addCSourceFiles(&.{ "graph.c", "main.c", "points_group.c", "read_input.c", "smol_mesh.c", "q.c" }, rlplot_flags);
    if (opt != .Debug) {
        try genShaderhFile();
        rlplot.defineCMacro("RELEASE", null);
        rlplot.want_lto = true;
        raylib.want_lto = true;
    } else {
        rlplot.addCSourceFiles(&.{"refresh_shaders.c"}, rlplot_flags);
    }

    switch (target.getOsTag()) {
        .linux => {
            rlplot.defineCMacro("PLATFORM_DESKTOP", null);
        },
        else => {
            @panic("Unsupported OS");
        },
    }

    return rlplot;
}

pub fn build(b: *std.Build) !void {
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
