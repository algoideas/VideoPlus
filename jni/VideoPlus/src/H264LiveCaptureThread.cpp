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

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "H264LiveCaptureThread.hh"

using namespace std;

H264LiveCaptureThread::H264LiveCaptureThread()
    : mRunning(false), mExitFlag(false), mFrameBufLen(0), mTruncatedLen(0)
{
    memset(&mCtx, 0, sizeof(mCtx));
    memset(mFrameBuf, 0, sizeof(mFrameBuf));
}

H264LiveCaptureThread::~H264LiveCaptureThread()
{

}

bool H264LiveCaptureThread::Create(const char* device, int width, int height, int fps)
{
    if (mRunning)
    {
        return false;
    }

    memset(&mCtx, 0, sizeof(mCtx));
    if (H264_LIVE_CAPTURE_SUCCESS != H264LiveCaptureInit(&mCtx, device, width, height, fps))
    {
        return false;
    }

    if (0 != pthread_cond_init(&mCondThread, NULL))
    {
        H264LiveCaptureClose(&mCtx);
        return false;
    }

    if (0 != pthread_mutex_init(&mLockThread, NULL))
    {
        H264LiveCaptureClose(&mCtx);
        return false;
    }

    if (0 != pthread_mutex_init(&mLockBuf, NULL))
    {
        H264LiveCaptureClose(&mCtx);
        return false;
    }

    if (0 != pthread_create(&mThread, NULL, H264LiveCaptureThread::H264LiveCaptureProc, this))
    {
        H264LiveCaptureClose(&mCtx);
        return false;
    }

    mRunning = true;

    Capture();

    return true;
}

void H264LiveCaptureThread::Destroy()
{
    if (mRunning)
    {
        mExitFlag = true;
        pthread_cond_signal(&mCondThread);

        pthread_join(mThread, NULL);

        pthread_mutex_destroy(&mLockBuf);
        pthread_mutex_destroy(&mLockThread);
        pthread_cond_destroy(&mCondThread);

        H264LiveCaptureClose(&mCtx);
        
        mRunning = false;
    }
}

void H264LiveCaptureThread::Capture()
{
    /*int ret;
    void* outBuf = NULL;  
    int outLen = 0;*/

    if (!mRunning)
    {
        return;
    }

    pthread_cond_signal(&mCondThread);
}

void H264LiveCaptureThread::Export(void* buf, int len, int* frameLen, int* truncatedLen)
{
    int exportLen = mFrameBufLen;

    pthread_mutex_lock(&mLockBuf);
    if (mFrameBufLen > len) 
    {   
        *truncatedLen = mTruncatedLen + mFrameBufLen - len;
        exportLen = len;
    }
    else
    {
        *truncatedLen = mTruncatedLen;
    }
    memcpy(buf, mFrameBuf, exportLen);
    *frameLen = exportLen;

    pthread_mutex_unlock(&mLockBuf);
}

bool H264LiveCaptureThread::GetExitFlag()
{
    return mExitFlag;
}

void* H264LiveCaptureThread::H264LiveCaptureProc(void* ptr)
{
    H264LiveCaptureThread* thread = (H264LiveCaptureThread*)ptr;

    while (1)
    {
        if (thread->GetExitFlag())
        {
            break;
        }

        thread->CaptureProc();
    }

    pthread_exit(NULL);
}

void H264LiveCaptureThread::CaptureProc()
{
    int ret;
    void* outBuf = NULL;  
    int outLen = 0;

    pthread_mutex_lock(&mLockThread);
    pthread_cond_wait(&mCondThread, &mLockThread);
    pthread_mutex_unlock(&mLockThread);

    ret = H264LiveCapture(&mCtx, &outBuf, &outLen);
    if (H264_LIVE_CAPTURE_SUCCESS == ret)
    {
        unsigned int frameSize = outLen;
        int truncatedSize = 0;
        if (frameSize > sizeof(mFrameBuf))
        {
            truncatedSize = frameSize - sizeof(mFrameBuf);
            frameSize = sizeof(mFrameBuf); 
        }  

        pthread_mutex_lock(&mLockBuf);
        memcpy(mFrameBuf, outBuf, frameSize);
        mFrameBufLen = frameSize;
        mTruncatedLen = truncatedSize;
        pthread_mutex_unlock(&mLockBuf);
    }
    else
    {
        pthread_mutex_lock(&mLockBuf);
        mFrameBufLen = 0;
        mTruncatedLen = 0;
        pthread_mutex_unlock(&mLockBuf);
    }
}
