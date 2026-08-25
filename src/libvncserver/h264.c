/*
 * h264.c - H.264 encoding for noVNC (encoding 50)
 *
 * 包结构:
 *   Rectangle header (12字节, 标准VNC大端)
 *   + length (4字节大端) = H.264 payload 长度
 *   + flags  (4字节大端) = bit0:resetContext, bit1:resetAllContexts
 *   + H.264 Annex-B 码流
 */
#include <rfb/rfb.h>
#include "private.h"

/*
 * 由 TrollVNC 端实现: 获取最新一帧 H.264 码流
 * outData:  码流指针 (必须是 Annex-B 格式, 00 00 00 01 开头)
 * outLen:   码流字节数
 * outFlags: 输出 flags, 关键帧(含SPS+PPS+IDR)设 1, 普通P帧设 0
 * 返回: 1=有数据可发, 0=无新帧(会回退raw)
 */
extern int tvGetLatestH264Data(const uint8_t **outData, size_t *outLen, uint32_t *outFlags);

rfbBool
rfbSendRectEncodingH264(rfbClientPtr cl,
                        int x,
                        int y,
                        int w,
                        int h)
{
    rfbFramebufferUpdateRectHeader rect;
    const uint8_t *h264Data = NULL;
    size_t h264Len = 0;
    uint32_t flags = 0;
    uint32_t payloadLen;

    /* 客户端没开 H.264, 回退 raw */
    if (!cl->enableH264) {
        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    /* 从 TrollVNC 编码器拿最新一帧 */
    if (!tvGetLatestH264Data(&h264Data, &h264Len, &flags)) {
        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    /* 空帧保护 */
    if (h264Len == 0 || h264Data == NULL) {
        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    /* 先把缓冲区里残留的发掉 */
    rfbSendUpdateBuf(cl);

    /* ===== Rectangle header (12字节) ===== */
    rect.r.x = Swap16IfLE(x);
    rect.r.y = Swap16IfLE(y);
    rect.r.w = Swap16IfLE(w);
    rect.r.h = Swap16IfLE(h);
    rect.encoding = Swap32IfLE(rfbEncodingH264_noVNC);

    if (cl->ublen + sz_rfbFramebufferUpdateRectHeader > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }
    memcpy(&cl->updateBuf[cl->ublen], (char *)&rect,
           sz_rfbFramebufferUpdateRectHeader);
    cl->ublen += sz_rfbFramebufferUpdateRectHeader;

    /* ===== H.264 header: length(4) + flags(4), 都必须大端 ===== */
    payloadLen = Swap32IfLE((uint32_t)h264Len);
    flags = Swap32IfLE(flags);

    if (cl->ublen + 8 + h264Len > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }
    memcpy(&cl->updateBuf[cl->ublen], &payloadLen, 4);
    cl->ublen += 4;
    memcpy(&cl->updateBuf[cl->ublen], &flags, 4);
    cl->ublen += 4;

    /* ===== H.264 Annex-B 码流 ===== */
    memcpy(&cl->updateBuf[cl->ublen], h264Data, h264Len);
    cl->ublen += h264Len;

    rfbStatRecordEncodingSent(cl, rfbEncodingH264_noVNC,
                              sz_rfbFramebufferUpdateRectHeader + 8 + h264Len,
                              sz_rfbFramebufferUpdateRectHeader + w * h * (cl->format.bitsPerPixel / 8));
    return TRUE;
}
