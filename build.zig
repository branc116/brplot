const std = @import("std");
const os = std.os;

var is_release = true;

pub fn generate_fonts(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    const native_rl = try build_raylib(b, native_target, full);
    var generateFont = b.addExecutable(.{
        .name = "generate_shaders",
        .target = native_target,
        .optimize = full,
    });
    generateFont.addCSourceFile(.{ .file = b.path("./tools/font_bake.c") });
    generateFont.linkLibrary(native_rl);
    generateFont.linkLibC();
    var ret = b.addRunArtifact(generateFont);
    ret.addArg("./content/PlayfairDisplayRegular-ywLOY.ttf");
    ret.addArg(".generated/default_font.h");
    return ret;
}

pub fn generate_shaders(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    const native_rl = try build_raylib(b, native_target, full);
    var shaderGen = b.addExecutable(.{
        .name = "generate_shaders",
        .target = native_target,
        .optimize = full,
    });
    shaderGen.addCSourceFile(.{ .file = b.path("./tools/shaders_bake.c") });
    shaderGen.addCSourceFile(.{ .file = b.path("./src/str.c") });
    shaderGen.addIncludePath(b.path("./external/raylib-5.0/src"));
    shaderGen.addIncludePath(b.path("./src"));
    shaderGen.addIncludePath(b.path("."));
    shaderGen.linkLibC();
    shaderGen.linkLibrary(native_rl);
    var ret = b.addRunArtifact(shaderGen);
    ret.addArg("WINDOWS");
    ret.addArg(".generated/shaders.h");
    return ret;
}

pub fn build_raylib(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) !*std.Build.Step.Compile {
    const raylib = b.addStaticLibrary(.{
        .name = b.fmt("raylib_{s}_{s}", .{ @tagName(target.result.os.tag), @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });

    const raylibSources = .{
        "./external/raylib-5.0/src/rmodels.c",
        "./external/raylib-5.0/src/rshapes.c",
        "./external/raylib-5.0/src/rtext.c",
        "./external/raylib-5.0/src/rtextures.c",
        "./external/raylib-5.0/src/utils.c",
        "./external/raylib-5.0/src/rcore.c",
    };
    inline for (raylibSources) |source| {
        raylib.addCSourceFile(.{ .file = b.path(source) });
    }

    switch (target.result.os.tag) {
        .windows => {
            raylib.addCSourceFile(.{ .file = b.path("./external/raylib-5.0/src/rglfw.c") });
            raylib.linkSystemLibrary("winmm");
            raylib.linkSystemLibrary("gdi32");
            raylib.linkSystemLibrary("opengl32");
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
        },
        .linux, .freebsd => {
            raylib.linkSystemLibrary("glfw");
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
        },
        .macos => {
            var raylib_flags_arr: std.ArrayListUnmanaged([]const u8) = .{};
            raylib_flags_arr.clearRetainingCapacity();
            try raylib_flags_arr.append(b.allocator, "-ObjC");
            raylib.addCSourceFile(.{
                .file = b.path("./external/raylib-5.0/src/rglfw.c"),
                .flags = raylib_flags_arr.items,
            });
            raylib_flags_arr.deinit(b.allocator);
            raylib.linkFramework("Foundation");
            raylib.linkFramework("CoreServices");
            raylib.linkFramework("CoreGraphics");
            raylib.linkFramework("AppKit");
            raylib.linkFramework("IOKit");
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
        },
        else => {
            unreachable();
        },
    }
    raylib.linkLibC();
    raylib.addIncludePath(b.path("."));
    raylib.addIncludePath(b.path("./external/glfw/include"));
    return raylib;
}

pub fn build_brplot(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode, is_headless: bool) !*std.Build.Step.Compile {
    is_release = optimize != std.builtin.OptimizeMode.Debug;

    const exe = b.addExecutable(.{
        .name = b.fmt("brplot_{}_{s}_{s}", .{ is_headless, @tagName(target.result.os.tag), @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });

    exe.addCSourceFiles(.{ .files = &.{
        "./src/data.c",
        "./src/gl.c",
        "./src/icons.c",
        "./src/platform.c",
        "./src/data_generator.c",
        "./src/filesystem.c",
        "./src/graph_utils.c",
        "./src/gui.c",
        "./src/help.c",
        "./src/keybindings.c",
        "./src/main.c",
        "./src/memory.c",
        "./src/permastate.c",
        "./src/plot.c",
        "./src/plotter.c",
        "./src/q.c",
        "./src/read_input.c",
        "./src/resampling2.c",
        "./src/shaders.c",
        "./src/smol_mesh.c",
        "./src/str.c",
        "./src/text_renderer.c",
    } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(b.path("./src"));
    exe.addIncludePath(b.path("./external/Tracy"));
    exe.linkLibC();
    exe.defineCMacro("PLATFORM_DESKTOP", "1");

    if (is_release) {
        exe.defineCMacro("RELEASE", "1");
        exe.step.dependOn(&(try generate_shaders(b)).step);
    }

    switch (is_headless) {
        false => {
            exe.defineCMacro("RAYLIB", null);
        },
        true => {
            exe.defineCMacro("NUMBER_OF_STEPS", "100");
            exe.defineCMacro("HEADLESS", null);
        },
    }

    exe.step.dependOn(&(try generate_fonts(b)).step);
    return exe;
}

pub fn build_all(b: *std.Build, target: std.Build.ResolvedTarget) !void {
    inline for (std.meta.fields(std.builtin.OptimizeMode)) |optim| {
        {
            const brplot = try build_brplot(b, target, @enumFromInt(optim.value), true);
            b.installArtifact(brplot);
        }
        {
            const brplot = try build_brplot(b, target, @enumFromInt(optim.value), false);
            b.installArtifact(brplot);
        }
    }
}

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});

    const optimize = b.standardOptimizeOption(.{});

    const gui_kind = b.option(bool, "Headless", "Build headless") orelse false;
    const all = b.option(bool, "All", "Build all targets") orelse false;
    if (all) {
        try build_all(b, target);
    } else {
        const brplot = try build_brplot(b, target, optimize, gui_kind);
        b.installArtifact(brplot);
    }
    //        const run_cmd = b.addRunArtifact(brplot);
    //        run_cmd.step.dependOn(b.getInstallStep());
    //
    //        const run_step = b.step("run", "Run the app");
    //        run_step.dependOn(&run_cmd.step);
}
