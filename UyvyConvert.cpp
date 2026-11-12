#include "UyvyConvert.h"

#include <cmath>
#include <cstring>

// ARMv8 一律带 NEON；留 UYVY_FORCE_SCALAR 是为了能在同一台机器上把标量路径单独跑出来对拍。
#if !defined(UYVY_FORCE_SCALAR) && (defined(__ARM_NEON) || defined(__ARM_NEON__))
#define UYVY_USE_NEON 1
#include <arm_neon.h>
#endif

namespace {

// 定点精度取 Q4（1/16 LSB）：量化误差 ≤ 1/32 LSB，而系数本身的最大相对误差
// （1.772 -> 453.632/256 这档）折算到输出不到 0.06 LSB，肉眼和逐位对拍都可以当成精确值。
// 最大中间量：Y*16 + cb*16 = 255*16 + 1.772*128*16 = 7709，int16 装得下，不会溢出。
constexpr int kShift = 4;
constexpr int kRound = 1 << (kShift - 1); // 四舍五入用的 +8

struct Tables {
    int16_t cr[256];   // 1.402000 * (V - 128)
    int16_t cb[256];   // 1.772000 * (U - 128)
    int16_t cgU[256];  // 0.344136 * (U - 128)
    int16_t cgV[256];  // 0.714136 * (V - 128)

    Tables()
    {
        for (int i = 0; i < 256; ++i) {
            const int d = i - 128;
            cr[i] = static_cast<int16_t>(std::lround(d * 1.402000 * 16.0));
            cb[i] = static_cast<int16_t>(std::lround(d * 1.772000 * 16.0));
            cgU[i] = static_cast<int16_t>(std::lround(d * 0.344136 * 16.0));
            cgV[i] = static_cast<int16_t>(std::lround(d * 0.714136 * 16.0));
        }
    }
};

// C++11 起局部静态变量初始化是线程安全的，workThread 里第一次调用不用额外加锁
const Tables &tables()
{
    static const Tables t;
    return t;
}

inline int clamp8(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

// 单个像素的标量算法。NEON 版本必须和它在任何输入下逐位一致，所以两边都走同一套
// “查表 -> 加 Y*16 -> +8 -> >>4 -> 饱和到 0..255”的步骤。
inline void onePixel(const unsigned char *row,
                     int x,
                     const Tables &tb,
                     uint32_t *dst)
{
    const int base = (x >> 1) << 2;            // 该像素所属像素对的起始字节
    const int Y = row[base + ((x & 1) ? 3 : 1)];
    const int U = row[base + 0];
    const int V = row[base + 2];
    const int yq = Y << kShift;
    const int cg = tb.cgU[U] + tb.cgV[V];
    dst[x] = 0xFF000000u
             | (static_cast<uint32_t>(clamp8((yq + tb.cr[V] + kRound) >> kShift)) << 16)
             | (static_cast<uint32_t>(clamp8((yq - cg + kRound) >> kShift)) << 8)
             | static_cast<uint32_t>(clamp8((yq + tb.cb[U] + kRound) >> kShift));
}

#if defined(UYVY_USE_NEON)
// 关掉 NEON 对拍时才用到，开着的时候别让 -Wunused-function 叫
[[maybe_unused]]
#endif
void fillRowScalar(const unsigned char *row,
                   int srcWidth,
                   const Tables &tb,
                   uint32_t *dst)
{
    int x = 0;
    for (; x < srcWidth; ++x) {
        onePixel(row, x, tb, dst);
    }
}

#ifdef UYVY_USE_NEON
// 一次 8 像素。整条路径都在 int16x8 上做，和标量版本用的是同一张表、同一个移位，
// 因此不存在“快了但颜色变了”的情况（对拍验证过逐位一致）。
void fillRowNeon(const unsigned char *row,
                 int srcWidth,
                 const Tables &tb,
                 uint32_t *dst)
{
    const int16x8_t rnd = vdupq_n_s16(kRound);
    const uint32x4_t alpha = vdupq_n_u32(0xFF000000u);

    int x = 0;
    for (; x + 8 <= srcWidth; x += 8) {
        const unsigned char *p = row + (static_cast<size_t>(x) << 1); // 8 像素 = 16 字节

        // 色度是 4:2:2，一组两个像素共用同一对 U/V，所以按像素展开成 8 项再整体载入
        int16_t crA[8], cbA[8], cgA[8];
        for (int k = 0; k < 4; ++k) {
            const int U = p[k * 4 + 0];
            const int V = p[k * 4 + 2];
            const int cr = tb.cr[V];
            const int cb = tb.cb[U];
            const int cg = tb.cgU[U] + tb.cgV[V];
            crA[k * 2] = crA[k * 2 + 1] = static_cast<int16_t>(cr);
            cbA[k * 2] = cbA[k * 2 + 1] = static_cast<int16_t>(cb);
            cgA[k * 2] = cgA[k * 2 + 1] = static_cast<int16_t>(cg);
        }
        const int16x8_t cr = vld1q_s16(crA);
        const int16x8_t cb = vld1q_s16(cbA);
        const int16x8_t cg = vld1q_s16(cgA);

        // UYVY 的奇数字节全是 Y，去交织后 val[1] 正好是 8 个连续的 Y
        const uint8x8x2_t deint = vuzp_u8(vld1_u8(p), vld1_u8(p + 8));
        const int16x8_t yq = vshlq_n_s16(vreinterpretq_s16_u16(vmovl_u8(deint.val[1])), kShift);

        const int16x8_t rq = vshrq_n_s16(vaddq_s16(vaddq_s16(yq, cr), rnd), kShift);
        const int16x8_t gq = vshrq_n_s16(vsubq_s16(vaddq_s16(yq, rnd), cg), kShift);
        const int16x8_t bq = vshrq_n_s16(vaddq_s16(vaddq_s16(yq, cb), rnd), kShift);

        // vqmovun：有符号 16 位饱和窄化到无符号 8 位，顺手完成 0..255 钳位
        const uint8x8_t r8 = vqmovun_s16(rq);
        const uint8x8_t g8 = vqmovun_s16(gq);
        const uint8x8_t b8 = vqmovun_s16(bq);

        const uint16x8_t r16 = vmovl_u8(r8);
        const uint16x8_t g16 = vmovl_u8(g8);
        const uint16x8_t b16 = vmovl_u8(b8);

        uint32x4_t olo = vshlq_n_u32(vmovl_u16(vget_low_u16(r16)), 16);
        uint32x4_t ohi = vshlq_n_u32(vmovl_u16(vget_high_u16(r16)), 16);
        olo = vorrq_u32(olo, vshlq_n_u32(vmovl_u16(vget_low_u16(g16)), 8));
        ohi = vorrq_u32(ohi, vshlq_n_u32(vmovl_u16(vget_high_u16(g16)), 8));
        olo = vorrq_u32(olo, vmovl_u16(vget_low_u16(b16)));
        ohi = vorrq_u32(ohi, vmovl_u16(vget_high_u16(b16)));

        vst1q_u32(dst + x, vorrq_u32(olo, alpha));
        vst1q_u32(dst + x + 4, vorrq_u32(ohi, alpha));
    }
    for (; x < srcWidth; ++x) { // 宽度不是 8 的倍数时收尾
        onePixel(row, x, tb, dst);
    }
}
#endif // UYVY_USE_NEON

} // namespace

namespace UyvyConvert {

void fillRgb32(const unsigned char *src,
               int srcWidth,
               int y0,
               int y1,
               int srcBytesPerLine,
               unsigned char *dst,
               int dstBytesPerLine)
{
    if (!src || !dst || srcWidth <= 0 || y0 < 0 || y1 <= y0) {
        return;
    }
    const Tables &tb = tables();
    for (int y = y0; y < y1; ++y) {
        const unsigned char *row = src + static_cast<size_t>(y) * srcBytesPerLine;
        uint32_t *dstRow = reinterpret_cast<uint32_t *>(dst + static_cast<size_t>(y) * dstBytesPerLine);
#ifdef UYVY_USE_NEON
        fillRowNeon(row, srcWidth, tb, dstRow);
#else
        fillRowScalar(row, srcWidth, tb, dstRow);
#endif
    }
}

} // namespace UyvyConvert
