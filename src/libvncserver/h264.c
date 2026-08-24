/*
 * h264.c - H.264 encoding for noVNC (encoding 50)
 */

#include <rfb/rfb.h>
#include "private.h"

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

    /* TODO: 接入 TVH264Encoder */
    return TRUE;
}
