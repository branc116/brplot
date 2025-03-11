const std = @import("std");
const os = std.os;

var is_release = true;

pub fn generate_gl(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    var generateFont = b.addExecutable(.{
        .name = "gen_gl",
        .target = native_target,
        .optimize = full,
    });
    generateFont.addCSourceFile(.{ .file = b.path("./tools/gl_gen.c") });
    generateFont.addCSourceFile(.{ .file = b.path("./src/str.c") });
    generateFont.addIncludePath(b.path("."));
    generateFont.linkLibC();
    const ret = b.addRunArtifact(generateFont);
    return ret;
}

pub fn generate_fonts(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    var generateFont = b.addExecutable(.{
        .name = "generate_shaders",
        .target = native_target,
        .optimize = full,
    });
    generateFont.addCSourceFile(.{ .file = b.path("./tools/font_bake.c") });
    generateFont.linkLibC();
    var ret = b.addRunArtifact(generateFont);
    ret.addArg("./content/font.ttf");
    ret.addArg(".generated/default_font.h");
    return ret;
}

pub fn generate_shaders(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    var shaderGen = b.addExecutable(.{
        .name = "generate_shaders",
        .target = native_target,
        .optimize = full,
    });
    shaderGen.addCSourceFile(.{ .file = b.path("./tools/shaders_bake.c") });
    shaderGen.addCSourceFile(.{ .file = b.path("./src/str.c") });
    shaderGen.addIncludePath(b.path("."));
    shaderGen.linkLibC();
    var ret = b.addRunArtifact(shaderGen);
    ret.addArg("WINDOWS");
    ret.addArg(".generated/shaders.h");
    return ret;
}

pub fn pack_icons(b: *std.Build) !*std.Build.Step.Run {
    const full = std.builtin.OptimizeMode.ReleaseSafe;
    const native_target = b.resolveTargetQuery(.{});
    var shaderGen = b.addExecutable(.{
        .name = "pack_icons",
        .target = native_target,
        .optimize = full,
    });
    shaderGen.addCSourceFile(.{ .file = b.path("./tools/pack_icons.c") });
    shaderGen.addIncludePath(b.path("."));
    shaderGen.linkLibC();
    return b.addRunArtifact(shaderGen);
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
        "./src/data_generator.c",
        "./src/filesystem.c",
        "./src/gl.c",
        "./src/graph_utils.c",
        "./src/gui.c",
        "./src/icons.c",
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
        "./src/theme.c",
        "./src/ui.c",
    } });
    exe.addIncludePath(b.path("."));
    exe.addIncludePath(b.path("./src"));
    exe.linkLibC();

    if (target.result.os.tag == .windows) {
        exe.linkSystemLibrary("gdi32");
        exe.linkSystemLibrary("opengl32");
        exe.linkSystemLibrary("winmm");
    }

    if (target.result.os.tag == .macos) {
        exe.root_module.addCSourceFile(.{ .file = b.path("src/platform.c"), .flags = &.{"-ObjC"} });
        exe.linkFramework("Foundation");
        exe.linkFramework("CoreServices");
        exe.linkFramework("CoreGraphics");
        exe.linkFramework("AppKit");
        exe.linkFramework("IOKit");
    } else {
        exe.root_module.addCSourceFile(.{ .file = b.path("src/platform.c") });
    }

    exe.defineCMacro("_GNU_SOURCE", null);
    if (false == is_release) {
        exe.defineCMacro("BR_DEBUG", "1");
    } else {
        exe.step.dependOn(&(try generate_shaders(b)).step);
    }

    switch (is_headless) {
        false => {},
        true => {
            exe.defineCMacro("NUMBER_OF_STEPS", "100");
            exe.defineCMacro("HEADLESS", null);
        },
    }

    exe.step.dependOn(&(try pack_icons(b)).step);
    exe.step.dependOn(&(try generate_fonts(b)).step);
    exe.step.dependOn(&(try generate_gl(b)).step);
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
    std.fs.cwd().makeDir(".generated") catch {};
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
