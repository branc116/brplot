const std = @import("std");

export fn zig_print_stacktrace() callconv(.C) void {
    std.debug.panic("ERROR", .{});
}

extern fn _main() void;

pub fn main() void {
    _main();
}
