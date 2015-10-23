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
#include "H264LiveFramedSource.hh"

using namespace std;

H264LiveFramedSource* H264LiveFramedSource::createNew(UsageEnvironment& env, 
    const char* device, int width, int height, int fps)
{
    H264LiveCaptureThread* thread = new H264LiveCaptureThread();
    if (NULL == thread)
    {
        return NULL;
    }

    if (!thread->Create(device, width, height, fps))
    {
        delete thread;
        return NULL;
    }

    H264LiveFramedSource* newSource = new H264LiveFramedSource(env, thread);

    return newSource;
}

H264LiveFramedSource::H264LiveFramedSource(UsageEnvironment& env, 
    H264LiveCaptureThread* thread)
    : FramedSource(env), mThread(thread)
{
}

H264LiveFramedSource::~H264LiveFramedSource()
{
    mThread->Destroy();
    envir().taskScheduler().unscheduleDelayedTask(mToken);
}

void H264LiveFramedSource::doGetNextFrame()
{ 
    mToken = envir().taskScheduler().scheduleDelayedTask(0,
        getNextFrame, this);
}

void H264LiveFramedSource::getNextFrame(void* ptr)
{  
    ((H264LiveFramedSource*)ptr)->getNextFrame1();  
} 

void H264LiveFramedSource::getNextFrame1()
{
    int frameSize, truncatedSize;

    mThread->Export(fTo, maxFrameSize(), &frameSize, &truncatedSize);
    fFrameSize = frameSize;
    fNumTruncatedBytes = truncatedSize;
    mThread->Capture();

    // notify  
    afterGetting(this); 
}

unsigned int H264LiveFramedSource::maxFrameSize() const
{
    return H264_MAX_FRAME_SIZE;
}




