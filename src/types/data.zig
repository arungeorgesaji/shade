const std = @import("std");
const Value = @import("value.zig").Value;

pub const DataState = enum {
    living,
    ghost,
    exorcised,
};

pub const DataRecord = struct {
    id: u64,
    state: DataState,
    values: []Value,
    deleted_at: ?i64 = null, 
    ghost_strength: f32 = 1.0, 

    pub fn init(allocator: std.mem.Allocator, id: u64, values: []const Value) !DataRecord {
        const copied_values = try allocator.alloc(Value, values.len);
        @memcpy(copied_values, values);
        
        return DataRecord{
            .id = id,
            .state = .living,
            .values = copied_values,
        };
    }

    pub fn deinit(self: *DataRecord, allocator: std.mem.Allocator) void {
        allocator.free(self.values);
        self.* = undefined;
    }

    pub fn markGhost(self: *DataRecord, timestamp: i64) void {
        self.state = .ghost;
        self.deleted_at = timestamp;
        self.ghost_strength = 1.0;
    }

    pub fn decayGhost(self: *DataRecord, decay_rate: f32) void {
        if (self.state == .ghost) {
            self.ghost_strength -= decay_rate;
            if (self.ghost_strength <= 0.0) {
                self.state = .exorcised;
            }
        }
    }

    pub fn isQueryable(self: DataRecord) bool {
        return self.state != .exorcised;
    }
};

test "DataRecord lifecycle" {
    const testing = std.testing;
    const allocator = testing.allocator;
    
    const values = &[_]Value{ 
        Value{ .integer = 1 }, 
        Value{ .string = "test" } 
    };
    
    var record = try DataRecord.init(allocator, 1, values);
    defer record.deinit(allocator);
    
    try testing.expect(record.state == .living);
    try testing.expect(record.deleted_at == null);
    try testing.expect(record.isQueryable());
    
    record.markGhost(1234567890);
    try testing.expect(record.state == .ghost);
    try testing.expect(record.deleted_at.? == 1234567890);
    try testing.expect(record.ghost_strength == 1.0);
    try testing.expect(record.isQueryable());
    
    record.decayGhost(0.5);
    try testing.expect(record.ghost_strength == 0.5);
    try testing.expect(record.isQueryable());
    
    record.decayGhost(0.6);
    try testing.expect(record.state == .exorcised);
    try testing.expect(!record.isQueryable());
}
