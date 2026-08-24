/*
 * h264.c - H.264 encoding for noVNC (encoding 50)
 */

#include <rfb/rfb.h>
#include "private.h"

extern int tvGetLatestH264Data(const uint8_t **outData, size_t *outLen);

rfbBool
rfbSendRectEncodingH264(rfbClientPtr cl,
                        int x,
                        int y,
                        int w,
                        int h)
{
    rfbFramebufferUpdateRectHeader rect;

    if (!cl->enableH264) {
        return rfbSendRectEncodingRaw(cl, x, y, w, h);
    }

    rfbSendUpdateBuf(cl);

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

    const uint8_t *h264Data = NULL;
    size_t h264Len = 0;
    if (!tvGetLatestH264Data(&h264Data, &h264Len)) {
        rfbLog("H264: 无数据，回退到 Tight\n");
        return rfbSendRectEncodingTight(cl, x, y, w, h);  // ← 改这一行
    }

    uint32_t payloadLen = Swap32IfLE((uint32_t)h264Len);
    uint32_t flags = 0;

    if (cl->ublen + 8 + h264Len > UPDATE_BUF_SIZE) {
        if (!rfbSendUpdateBuf(cl))
            return FALSE;
    }

    memcpy(&cl->updateBuf[cl->ublen], &payloadLen, 4);
    cl->ublen += 4;
    memcpy(&cl->updateBuf[cl->ublen], &flags, 4);
    cl->ublen += 4;
    memcpy(&cl->updateBuf[cl->ublen], h264Data, h264Len);
    cl->ublen += h264Len;

    return TRUE;
}
