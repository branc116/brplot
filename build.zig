const std = @import("std");

const rlplot_flags = &[_][]const u8{
    "-DZIG",
    "-std=gnu99",
    "-D_GNU_SOURCE",
    "-DGL_SILENCE_DEPRECATION=199309L",
    "-fno-sanitize=undefined", // https://github.com/raysan5/raylib/issues/189
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
        .windows => {
            raylib.defineCMacro("PLATFORM_DESKTOP", null);
            raylib.defineCMacro("_WIN32", "1");
            raylib.addIncludePath(.{ .path = "./raylib/src/external/glfw/include" });
            raylib.addCSourceFile(.{ .file = .{ .path = rlf("/rglfw.c") }, .flags = rlplot_flags });
            raylib.linkSystemLibrary("winmm");
            raylib.linkSystemLibrary("gdi32");
            raylib.linkSystemLibrary("opengl32");
            raylib.addIncludePath(.{ .path = "./raylib/src/external/glfw/deps/mingw" });
        },
        .wasi => {
            //-DGRAPHICS_API_OPENGL_ES2 -DPLATFORM_WEB --memory-init-file 1 --closure 1 -s WASM_BIGINT -s ENVIRONMENT=web -sALLOW_MEMORY_GROWTH -s USE_GLFW=3 -s ASYNCIFY $(CCFLAGS)
            raylib.defineCMacro("PLATFORM_WEB", null);
            raylib.defineCMacro("GRAPHICS_API_OPENGL_ES2", null);
        },
        else => {
            @panic("Unsupported OS");
        },
    }
    return raylib;
}

const print = std.debug.print;

fn one_shader(f: std.fs.File, comptime name: []const u8, comptime macroName: []const u8) !void {
    const file = @embedFile(name);
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
    const shader_file_name = "src/shaders.h";
    std.fs.cwd().deleteFile(shader_file_name) catch |e| print("{}", .{e});
    const file = try std.fs.cwd().createFile(shader_file_name, .{ .read = true });
    _ = try one_shader(file, "src/desktop/shaders/grid.fs", "SHADER_GRID_FS");
    _ = try one_shader(file, "src/desktop/shaders/line.fs", "SHADER_LINE_FS");
    _ = try one_shader(file, "src/desktop/shaders/line.vs", "SHADER_LINE_VS");
    _ = try one_shader(file, "src/desktop/shaders/quad.fs", "SHADER_QUAD_FS");
    _ = try one_shader(file, "src/desktop/shaders/quad.vs", "SHADER_QUAD_VS");
}

// This has been tested to work with zig master branch as of commit 87de821 or May 14 2023
pub fn addRlPlot(b: *std.build.Builder, opt: std.builtin.OptimizeMode, target: std.zig.CrossTarget) !*std.Build.CompileStep {
    const rlplot = b.addExecutable(.{ .name = "rlplot", .optimize = opt, .target = target });
    const raylib = addRayLib(b, target, opt);

    rlplot.linkLibrary(raylib);
    rlplot.linkLibC();
    rlplot.addIncludePath(.{ .path = "raylib/src" });
    rlplot.addCSourceFiles(&.{ "src/graph.c", "src/main.c", "src/points_group.c", "src/read_input.c", "src/smol_mesh.c", "src/q.c", "src/resampling.c", "src/ui.c", "src/help.c", "src/file_explorer.c" }, rlplot_flags);
    if (opt != .Debug) {
        try genShaderhFile();
        rlplot.defineCMacro("RELEASE", null);
        rlplot.want_lto = true;
        raylib.want_lto = true;
    } else {
        rlplot.addCSourceFile(.{ .file = .{ .path = "src/desktop/linux/refresh_shaders.c" }, .flags = rlplot_flags });
    }

    switch (target.getOsTag()) {
        .linux => {
            rlplot.defineCMacro("PLATFORM_DESKTOP", null);
            rlplot.addCSourceFile(.{ .file = .{ .path = "./src/desktop/linux/read_input.c" }, .flags = rlplot_flags });
        },
        .windows => {
            rlplot.defineCMacro("PLATFORM_DESKTOP", null);
            rlplot.defineCMacro("_WIN32", "1");
            rlplot.addCSourceFile(.{ .file = .{ .path = "./src/desktop/win/read_input.c" }, .flags = rlplot_flags });
        },
        .wasi => {
            rlplot.defineCMacro("PLATFORM_WEB", null);
            rlplot.defineCMacroRaw("sUSE_GLFW=3");
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
