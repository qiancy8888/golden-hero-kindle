-- Raw cyttsp event trace on KPW1, read-only (20 seconds).
-- FFI layout uses the KPW1 32-bit ARM Linux input_event ABI.
local ffi = require('ffi')
ffi.cdef[[
typedef long ssize_t;
struct timeval { long tv_sec; long tv_usec; };
struct input_event { struct timeval time; unsigned short type; unsigned short code; int value; };
int open(const char *path, int flags, ...);
ssize_t read(int fd, void *buf, unsigned long count);
int close(int fd);
int usleep(unsigned int usec);
int gettimeofday(struct timeval *tv, void *tz);
]]
local C = ffi.C
print('Raw cyttsp event trace v0.8; 20-second observation window')
print('input_event sizeof=' .. ffi.sizeof('struct input_event') .. ' (expected 16 on ARMv7)')
if ffi.sizeof('struct input_event') ~= 16 then
    print('ERROR: Unexpected 32-bit event structure ABI; exiting without reading.')
    os.exit(4)
end
local fd = C.open('/dev/input/event0', 0x800)
if fd < 0 then
    print('ERROR: Unable to open input event0; errno=' .. tonumber(ffi.errno()))
    os.exit(5)
end
local started = ffi.new('struct timeval[1]')
local now = ffi.new('struct timeval[1]')
local ev = ffi.new('struct input_event[1]')
C.gettimeofday(started, nil)
local count, coords, press = 0, 0, 0
while count < 800 do
    C.gettimeofday(now, nil)
    local elapsed = tonumber(now[0].tv_sec - started[0].tv_sec) +
        tonumber(now[0].tv_usec - started[0].tv_usec) / 1000000
    if elapsed >= 20 then break end
    local n = C.read(fd, ev, ffi.sizeof('struct input_event'))
    if n == ffi.sizeof('struct input_event') then
        local typ, code, value = tonumber(ev[0].type), tonumber(ev[0].code), tonumber(ev[0].value)
        if typ == 3 or typ == 1 or typ == 0 then
            print(string.format('T+%.3fs type=%d code=%d value=%d', elapsed, typ, code, value))
            count = count + 1
            if typ == 3 and (code == 0 or code == 1 or code == 53 or code == 54) then coords = coords + 1 end
            if (typ == 1 and code == 330) or (typ == 3 and code == 57) then press = press + 1 end
        end
    elseif n < 0 then
        local errno = tonumber(ffi.errno())
        if errno ~= 11 and errno ~= 4 then
            print('Read failed errno=' .. errno)
            break
        end
        C.usleep(20000)
    else
        C.usleep(20000)
    end
end
C.close(fd)
print(string.format('Summary events=%d coordinate_events=%d touch_state_events=%d', count, coords, press))
print('Keys: type=3 ABS; type=1 KEY; code 0/1 = ABS_X/Y; 53/54 = MT_X/Y; 57 = TRACKING_ID; 330 = BTN_TOUCH; type=0 code=0 = SYN_REPORT')
if count == 0 then print('NO RAW TOUCH EVENTS SEEN during the observation window.') end
