local ffi = require("ffi")

ffi.cdef[[
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef int int32_t;

struct fb_bitfield {
    uint32_t offset;
    uint32_t length;
    uint32_t msb_right;
};

struct fb_var_screeninfo {
    uint32_t xres, yres, xres_virtual, yres_virtual;
    uint32_t xoffset, yoffset, bits_per_pixel, grayscale;
    struct fb_bitfield red, green, blue, transp;
    uint32_t nonstd, activate;
    uint32_t height, width;
    uint32_t accel_flags;
    uint32_t pixclock, left_margin, right_margin, upper_margin, lower_margin;
    uint32_t hsync_len, vsync_len;
    uint32_t sync, vmode, rotate, colorspace;
    uint32_t reserved[4];
};

struct fb_fix_screeninfo {
    char id[16];
    unsigned long smem_start;
    uint32_t smem_len;
    uint32_t type, type_aux, visual;
    uint16_t xpanstep, ypanstep, ywrapstep;
    uint32_t line_length;
    unsigned long mmio_start;
    uint32_t mmio_len, accel;
    uint16_t capabilities;
    uint16_t reserved[2];
};

struct input_absinfo {
    int32_t value;
    int32_t minimum;
    int32_t maximum;
    int32_t fuzz;
    int32_t flat;
    int32_t resolution;
};

int open(const char *pathname, int flags, ...);
int close(int fd);
int ioctl(int fd, unsigned long request, ...);
]]

local C = ffi.C
local function out(s) io.write(s, "\n") end
local function errstr()
    return tostring(ffi.errno())
end

out("sizeof(fb_var_screeninfo)=" .. ffi.sizeof("struct fb_var_screeninfo"))
out("sizeof(fb_fix_screeninfo)=" .. ffi.sizeof("struct fb_fix_screeninfo"))
out("sizeof(input_absinfo)=" .. ffi.sizeof("struct input_absinfo"))
out("")
out("=== FRAMEBUFFER ioctl ===")
local fbfd = C.open("/dev/fb0", 0) -- O_RDONLY
if fbfd < 0 then
    out("open /dev/fb0 failed, errno=" .. errstr())
else
    local v = ffi.new("struct fb_var_screeninfo[1]")
    local f = ffi.new("struct fb_fix_screeninfo[1]")
    if C.ioctl(fbfd, 0x4600, v) == 0 then
        local a = v[0]
        out(string.format("xres=%u yres=%u", tonumber(a.xres), tonumber(a.yres)))
        out(string.format("xres_virtual=%u yres_virtual=%u", tonumber(a.xres_virtual), tonumber(a.yres_virtual)))
        out(string.format("xoffset=%u yoffset=%u", tonumber(a.xoffset), tonumber(a.yoffset)))
        out(string.format("bits_per_pixel=%u grayscale=%u rotate=%u", tonumber(a.bits_per_pixel), tonumber(a.grayscale), tonumber(a.rotate)))
        out(string.format("red=%u/%u green=%u/%u blue=%u/%u", tonumber(a.red.offset), tonumber(a.red.length), tonumber(a.green.offset), tonumber(a.green.length), tonumber(a.blue.offset), tonumber(a.blue.length)))
    else
        out("FBIOGET_VSCREENINFO failed, errno=" .. errstr())
    end
    if C.ioctl(fbfd, 0x4602, f) == 0 then
        local a = f[0]
        local id = ffi.string(a.id, 16):gsub("%z.*$", "")
        out("id=" .. id)
        out(string.format("smem_len=%u line_length=%u type=%u visual=%u", tonumber(a.smem_len), tonumber(a.line_length), tonumber(a.type), tonumber(a.visual)))
        out(string.format("xpanstep=%u ypanstep=%u ywrapstep=%u", tonumber(a.xpanstep), tonumber(a.ypanstep), tonumber(a.ywrapstep)))
    else
        out("FBIOGET_FSCREENINFO failed, errno=" .. errstr())
    end
    C.close(fbfd)
end

out("")
out("=== TOUCH ABS ioctl ===")
local tfd = C.open("/dev/input/event0", 0)
if tfd < 0 then
    out("open /dev/input/event0 failed, errno=" .. errstr())
else
    local function read_axis(name, code)
        local a = ffi.new("struct input_absinfo[1]")
        -- EVIOCGABS(code) = _IOR('E', 0x40 + code, struct input_absinfo)
        local request = ffi.new("unsigned long", 0x80000000 + 0x00180000 + 0x4500 + 0x40 + code)
        if C.ioctl(tfd, request, a) == 0 then
            local v = a[0]
            out(string.format("%s value=%d min=%d max=%d fuzz=%d flat=%d resolution=%d",
                name, tonumber(v.value), tonumber(v.minimum), tonumber(v.maximum), tonumber(v.fuzz), tonumber(v.flat), tonumber(v.resolution)))
        else
            out(name .. " ioctl failed, errno=" .. errstr())
        end
    end
    read_axis("ABS_MT_POSITION_X (53)", 0x35)
    read_axis("ABS_MT_POSITION_Y (54)", 0x36)
    read_axis("ABS_X (0)", 0x00)
    read_axis("ABS_Y (1)", 0x01)
    C.close(tfd)
end

out("")
out("=== DEVICE NOTES ===")
out("KPW1 model resolution is commonly 758x1024, but use the ioctl xres/yres above for this device.")
out("The sysfs rotate value reported earlier was 3; compare it with ioctl rotate.")
out("No framebuffer pixels were written and no refresh ioctl was sent.")
