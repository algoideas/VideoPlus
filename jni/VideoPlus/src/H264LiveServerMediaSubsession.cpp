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
#include "H264LiveServerMediaSubsession.hh"
#include "H264LiveFramedSource.hh"

using namespace std;

H264LiveServerMediaSubsession* H264LiveServerMediaSubsession::createNew(UsageEnvironment& env, 
    const char* device, int width, int height, int fps)
{
    return new H264LiveServerMediaSubsession(env, device, width, height, fps);
}

H264LiveServerMediaSubsession::H264LiveServerMediaSubsession(UsageEnvironment& env, 
    const char* device, int width, int height, int fps)
    : OnDemandServerMediaSubsession(env, True),
      mAuxSDPLine(NULL), mDoneFlag(0), mDummyRTPSink(NULL),
      mWidth(width), mHeight(height), mFps(fps)
{
    mDevice = strDup(device);
}

H264LiveServerMediaSubsession::~H264LiveServerMediaSubsession()
{
    if (mAuxSDPLine)
    {
        delete[] mAuxSDPLine;
    }

    if (mDevice)
    {
        delete[] mDevice;
    }
}

static void afterPlayingDummy(void* clientData) 
{
    H264LiveServerMediaSubsession* subsess = (H264LiveServerMediaSubsession*)clientData;
    subsess->afterPlayingDummy1();
}

void H264LiveServerMediaSubsession::afterPlayingDummy1() 
{
    // Unschedule any pending 'checking' task:
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    // Signal the event loop that we're done:
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) 
{
    H264LiveServerMediaSubsession* subsess = (H264LiveServerMediaSubsession*)clientData;
    subsess->checkForAuxSDPLine1();
}

void H264LiveServerMediaSubsession::checkForAuxSDPLine1() 
{
    char const* dasl;

    if (mAuxSDPLine != NULL) 
    {
        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (mDummyRTPSink != NULL && (dasl = mDummyRTPSink->auxSDPLine()) != NULL) 
    {

        mAuxSDPLine = strDup(dasl);
        mDummyRTPSink->stopPlaying();
        mDummyRTPSink = NULL;

        // Signal the event loop that we're done:
        setDoneFlag();
    } 
    else if (!mDoneFlag) 
    {
        // try again after a brief delay:
        double delay = 10;  // ms  
        int uSecsToDelay = delay * 1000;  // us  
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
            (TaskFunc*)checkForAuxSDPLine, this);
    }
}

char const* H264LiveServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) 
{
    if (mAuxSDPLine != NULL) return mAuxSDPLine; // it's already been set up (for a previous client)

    if (mDummyRTPSink == NULL) 
    { 
        // we're not already setting it up for another, concurrent stream
        // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
        // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
        // and we need to start reading data from our file until this changes.
        mDummyRTPSink = rtpSink;

        // Start reading the file:
        //mDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);
        mDummyRTPSink->startPlaying(*inputSource, NULL, NULL);

        // Check whether the sink's 'auxSDPLine()' is ready:
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&mDoneFlag);

    return mAuxSDPLine;
}

FramedSource* H264LiveServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    estBitrate = 500;  

    // Create the video source:
    H264LiveFramedSource* cameraSource = H264LiveFramedSource::createNew(envir(), 
        mDevice, mWidth, mHeight, mFps);
    if (cameraSource == NULL) return NULL;

    return H264VideoStreamFramer::createNew(envir(), cameraSource);
}

RTPSink* H264LiveServerMediaSubsession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
