#ifndef UYVYCONVERT_H
#define UYVYCONVERT_H

#include <QtGlobal>

// GX 系列相机直接从 sensor 出 UYVY，量程是 full range（0~255 全部有效，等价于 JPEG/BT.601 完整量程），
// 不是视频常用的 limited range。所以这里：
//   1) 不把 Y 钳到 16~235、也不把 U/V 钳到 16~240；
//   2) 不做 limited->full 的 *255/219 拉伸；
//   3) 只保留 8bit RGB 落地时无法避免的 0~255 饱和。
// 公式取 full-range BT.601：
//   R = Y + 1.402000 * (V - 128)
//   G = Y - 0.344136 * (U - 128) - 0.714136 * (V - 128)
//   B = Y + 1.772000 * (U - 128)
// 色度三项只跟 0~255 的原始字节有关，全部预生成 Q4 定点查表，热循环里一次除法都没有。
namespace UyvyConvert {

// 把 [y0, y1) 这几行 UYVY 转成 32bit RGB（内存字节序 B,G,R,X，正好就是 QImage::Format_RGB32）。
// 拆成行区间是为了上层可以把同一帧分给多个线程并行转。
//
// 注意 src / dst 都必须是**整帧的起点**：行地址按绝对行号算（src + y*srcBytesPerLine、
// dst + y*dstBytesPerLine），y0 只决定转哪几行、不会拿去当指针偏移。并行时各线程传的
// 是同一个帧首地址，只是 y0/y1 不同，所以线程之间不会重叠加。
//
// 调用方需要保证：
//   srcWidth 为实际像素宽（奇数也可以，尾巴走标量）；
//   srcBytesPerLine >= srcWidth * 2，且 src 至少有 y1 * srcBytesPerLine 字节可读；
//   dst 至少有 y1 * dstBytesPerLine 字节可写，dstBytesPerLine >= srcWidth * 4。
// 满足以上两条后，函数内部不再做任何越界判断（判断放在这里是最外层的职责）。
void fillRgb32(const unsigned char *src,
               int srcWidth,
               int y0,
               int y1,
               int srcBytesPerLine,
               unsigned char *dst,
               int dstBytesPerLine);
}

#endif // UYVYCONVERT_H
