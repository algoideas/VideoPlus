/*
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <jni.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "H264LiveCapture.h"
#include "com_xproject_videoplus_Streamer.h"

#define TAG     "H264LiveCapture"


int H264LiveCaptureInit(H264LiveCaptureContext* ctx, 
    const char* device, int width, int height, int fps)
{
    if (ctx->ready)
    {
        LOG_DEBUG(TAG, "H264LiveCaptureInit, Inited ok.\n");
        return H264_LIVE_CAPTURE_SUCCESS;
    }
	
    ctx->ready = 1;
    
    LOG_DEBUG(TAG, "H264LiveCaptureInit ok.\n");
    return H264_LIVE_CAPTURE_SUCCESS;
}

int H264LiveCapture(H264LiveCaptureContext* ctx, void** output, int* len)
{
    int s32ret = 0;
    if (!ctx->ready)
    {
        LOG_ERR(TAG, "H264LiveCapture, No Init!");
        return H264_LIVE_CAPTURE_ERROR_NOTREADY;
    }

    s32ret = StreamerLiveCpature(output, len);
    if(H264_LIVE_CAPTURE_SUCCESS != s32ret) 
    {
        LOG_ERR(TAG, "call StreamerLiveCpature fail");
        return H264_LIVE_CAPTURE_ERROR_NOTREADY; 
    }
    
    return s32ret;
}

void H264LiveCaptureClose(H264LiveCaptureContext* ctx)
{
    if (!ctx->ready)
    {
        LOG_ERR(TAG, "H264LiveCaptureClose, No Init!");
        return;
    }

    ctx->ready = 0;
    LOG_DEBUG(TAG, "H264LiveCaptureClose ok.\n");
}


