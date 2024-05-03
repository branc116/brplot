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
    generateFont.addCSourceFile(.{ .file = b.path("./tools/font_export.c") });
    generateFont.addCSourceFile(.{ .file = b.path("./src/str.c") });
    generateFont.addIncludePath(b.path("."));
    generateFont.addIncludePath(b.path("./external/raylib-5.0/src"));
    generateFont.linkLibrary(native_rl);
    generateFont.linkLibC();
    var ret = b.addRunArtifact(generateFont);
    ret.addArg("./fonts/PlayfairDisplayRegular-ywLOY.ttf");
    ret.addArg("src/misc/default_font.h");
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
    ret.addArg("./src/misc/shaders.h");
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

pub fn build_imgui(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Step.Compile {
    const imgui = b.addStaticLibrary(.{
        .name = b.fmt("imgui_{s}_{s}", .{ @tagName(target.result.os.tag), @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });

    const imguiSources = .{
        "./external/imgui-docking/imgui.cpp",
        "./external/imgui-docking/imgui.cpp",
        "./external/imgui-docking/imgui_draw.cpp",
        "./external/imgui-docking/imgui_tables.cpp",
        "./external/imgui-docking/imgui_widgets.cpp",
        "./external/imgui-docking/backends/imgui_impl_glfw.cpp",
        "./external/imgui-docking/backends/imgui_impl_opengl3.cpp",
    };
    inline for (imguiSources) |source| {
        imgui.addCSourceFile(.{ .file = b.path(source) });
    }

    imgui.linkLibC();
    imgui.linkLibCpp();
    imgui.addIncludePath(b.path("./external/imgui-docking"));
    imgui.addIncludePath(b.path("./external/glfw/include"));
    return imgui;
}

const gui_kind_t = enum { imgui, raylib, headless };

pub fn build_brplot(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode, kind: gui_kind_t) !*std.Build.Step.Compile {
    is_release = optimize != std.builtin.OptimizeMode.Debug;

    const exe = b.addExecutable(.{
        .name = b.fmt("brplot_{s}_{s}_{s}", .{ @tagName(kind), @tagName(target.result.os.tag), @tagName(optimize) }),
        .target = target,
        .optimize = optimize,
    });
    const raylib = try build_raylib(b, target, optimize);
    const imgui = build_imgui(b, target, optimize);

    exe.addCSourceFiles(.{ .files = &.{
        "./src/data.c",
        "./src/filesystem.cpp",
        "./src/graph_utils.c",
        "./src/help.c",
        "./src/keybindings.c",
        "./src/main.c",
        "./src/memory.cpp",
        "./src/permastate.c",
        "./src/plot.c",
        "./src/plotter.c",
        "./src/q.c",
        "./src/read_input.c",
        "./src/resampling2.cpp",
        "./src/shaders.c",
        "./src/smol_mesh.c",
        "./src/str.c",
    } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(b.path("./src"));
    exe.addIncludePath(b.path("./external/raylib-5.0/src"));
    exe.addIncludePath(b.path("./external/Tracy"));
    exe.linkLibC();
    exe.defineCMacro("PLATFORM_DESKTOP", "1");

    if (is_release) {
        exe.defineCMacro("RELEASE", "1");
        exe.step.dependOn(&(try generate_shaders(b)).step);
    }

    switch (target.result.os.tag) {
        .windows => {
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/win/read_input.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/win/filesystem.cpp") });
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/platform.c") });
            if (false == is_release) {
                exe.addCSourceFile(.{ .file = b.path("./src/desktop/nob/refresh_shaders.c") });
            }
        },
        .freebsd, .linux, .macos => {
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/linux/read_input.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/linux/filesystem.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/desktop/platform.c") });
            if (false == is_release) {
                if (target.result.os.tag == .linux) {
                    exe.addCSourceFile(.{ .file = b.path("./src/desktop/linux/refresh_shaders.c") });
                } else {
                    exe.addCSourceFile(.{ .file = b.path("./src/desktop/nob/refresh_shaders.c") });
                }
            }
        },
        else => {
            unreachable();
        },
    }
    switch (kind) {
        .raylib => {
            exe.linkLibrary(raylib);
            exe.addCSourceFile(.{ .file = b.path("./src/raylib/gui.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/raylib/ui.c") });
        },
        .imgui => {
            exe.linkLibrary(raylib);
            exe.linkLibrary(imgui);
            exe.addCSourceFile(.{ .file = b.path("./src/imgui/gui.cpp") });
            exe.addCSourceFile(.{ .file = b.path("./src/imgui/imgui_extensions.cpp") });
            exe.addCSourceFile(.{ .file = b.path("./src/imgui/ui_settings.cpp") });
            exe.addCSourceFile(.{ .file = b.path("./src/imgui/ui_info.cpp") });
            exe.addCSourceFile(.{ .file = b.path("./src/imgui/file_saver.cpp") });
            exe.addIncludePath(b.path("./external/imgui-docking"));
            exe.addIncludePath(b.path("./src/imgui"));
        },
        .headless => {
            exe.defineCMacro("NUMBER_OF_STEPS", "100");
            exe.addCSourceFile(.{ .file = b.path("./src/headless/gui.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/headless/gui.c") });
            exe.addCSourceFile(.{ .file = b.path("./src/headless/raylib_headless.c") });
        },
    }
    exe.linkLibCpp();

    exe.step.dependOn(&(try generate_fonts(b)).step);
    return exe;
}

pub fn build_all(b: *std.Build, target: std.Build.ResolvedTarget) !void {
    inline for (std.meta.fields(std.builtin.OptimizeMode)) |optim| {
        inline for (std.meta.fields(gui_kind_t)) |kind| {
            const brplot = try build_brplot(b, target, @enumFromInt(optim.value), @enumFromInt(kind.value));
            b.installArtifact(brplot);
        }
    }
}

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) !void {
    // Standard target options allows the person running `zig build` to choose
    // what target to build for. Here we do not override the defaults, which
    // means any target is allowed, and the default is native. Other options
    // for restricting supported target set are available.
    const target = b.standardTargetOptions(.{});

    // Standard optimization options allow the person running `zig build` to select
    // between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
    // set a preferred release mode, allowing the user to decide how to optimize.
    const optimize = b.standardOptimizeOption(.{});

    const gui_kind = b.option(gui_kind_t, "Gui", "Gui Kind") orelse gui_kind_t.imgui;
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
