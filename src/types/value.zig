const std = @import("std");

pub const ValueType = enum {
    integer,
    float,
    boolean,
    string,
    null,
};

pub const Value = union(ValueType) {
    integer: i64,
    float: f64,
    boolean: bool,
    string: []const u8,
    null: void,

    pub fn eql(self: Value, other: Value) bool {
        if (@as(ValueType, self) != @as(ValueType, other)) return false;
        
        return switch (self) {
            .integer => |v| v == other.integer,
            .float => |v| v == other.float,
            .boolean => |v| v == other.boolean,
            .string => |v| std.mem.eql(u8, v, other.string),
            .null => true,
        };
    }

    pub fn format(
        self: Value,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        switch (self) {
            .integer => |v| try writer.print("{}", .{v}),
            .float => |v| try writer.print("{d}", .{v}),
            .boolean => |v| try writer.print("{}", .{v}),
            .string => |v| try writer.print("\"{s}\"", .{v}),
            .null => try writer.print("null", .{}),
        }
    }
};

test "Value equality" {
    const testing = std.testing;
    
    try testing.expect((Value{ .integer = 42 }).eql(Value{ .integer = 42 }));
    try testing.expect(!(Value{ .integer = 42 }).eql(Value{ .integer = 43 }));
    
    try testing.expect((Value{ .string = "hello" }).eql(Value{ .string = "hello" }));
    try testing.expect(!(Value{ .string = "hello" }).eql(Value{ .string = "world" }));
    
    try testing.expect(!(Value{ .integer = 42 }).eql(Value{ .float = 42.0 }));
}

test "Value formatting" {
    const testing = std.testing;
    
    var buf: [100]u8 = undefined;
    
    var fba = std.heap.FixedBufferAllocator.init(&buf);
    const allocator = fba.allocator();
    
    const int_str = try std.fmt.allocPrint(allocator, "{}", .{Value{ .integer = 42 }});
    defer allocator.free(int_str);
    try testing.expectEqualStrings("42", int_str);
}
