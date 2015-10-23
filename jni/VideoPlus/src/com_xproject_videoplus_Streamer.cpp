#include <jni.h>
#include <android/log.h>

extern "C" {
#include	"libavcodec/avcodec.h"
#include	"libavformat/avformat.h"
#include	"libavutil/opt.h"
#include	"libavutil/imgutils.h"
#include	"libswscale/swscale.h"
}

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "H264LiveServerMediaSubsession.hh"
#include "H264LiveCapture.h"
#include "com_xproject_videoplus_Streamer.h"

#define	TAG	"[JNI]Streamer"

#define STREAM_SAVE_FILE

void play();
void afterPlaying(void* /*clientData*/);

AVCodecContext *codecContext;
FILE *file;
AVFrame *frame;
AVFrame *tmpFrame;
AVPacket packet;
int count, got_output;
SwsContext *swsContext;

char frameBuff[100*1024];
int frameBuffLen = 0;
int liveFlag = 0;


UsageEnvironment* uEnv;
H264VideoStreamFramer* videoSource;
RTPSink* videoSink;

char const *inputFilename;

JavaVM *gJavaVM;
jmethodID mEventCallbackID;
jclass      mClass;     // Reference to jtxRemSkt class
jobject     mObject;    // Weak ref to jtxRemSkt Java object to call on

/**
 * return message string to the Java side
 *  @ch_mag char array of message
 */
void sendMessage(char * ch_msg)
{
	int status;
	JNIEnv *env;
	bool isAttached = false;

	env=NULL;

	status = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
	if(status < 0) {
		status = gJavaVM->AttachCurrentThread(&env, NULL);
		if(status < 0) {
			LOG_ERR(TAG, "callback_handler: failed to attach current thread");
		}
		isAttached = true;
	}
	
	jstring str_cb = env->NewStringUTF(ch_msg);
	env->CallStaticVoidMethod(mClass, mEventCallbackID, str_cb);
	(env)->DeleteLocalRef(str_cb);

	if(isAttached)
		gJavaVM->DetachCurrentThread();
}

static const char *classPathName = "com/xproject/videoplus/Streamer";

static JNINativeMethod methods[] = {
		{"start", "(Ljava/lang/String;III)V", (void *)Java_com_xproject_videoplus_Streamer_start},
		{"encode", "([B)I", (void *)Java_com_xproject_videoplus_Streamer_encode},
		{"stop", "()V", (void *)Java_com_xproject_videoplus_Streamer_stop},
		{"loop", "(Ljava/lang/String;)I", (void *)Java_com_xproject_videoplus_Streamer_loop},
		{"live", "(Ljava/lang/String;)I", (void *)Java_com_xproject_videoplus_Streamer_live}
};

/*
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
		JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOG_ERR(TAG, "Native registration unable to find class '%s'", className);
		return JNI_FALSE;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOG_ERR(TAG, "RegisterNatives failed for '%s'", className);
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

static int registerNatives(JNIEnv* env)
{
	if (!registerNativeMethods(env, classPathName,
			methods, sizeof(methods) / sizeof(methods[0]))) {
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	gJavaVM = vm;
	int result;

	LOG_INFO(TAG, "JNI_OnLoad call");
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOG_ERR(TAG, "(JNI_OnLoad()) .... Failed to get the environment using GetEnv()");
		return -1;
	}

	if (registerNatives(env) != JNI_TRUE) {
		LOG_ERR(TAG, "ERROR: registerNatives failed .... (JNI_OnLoad())");
		goto bail;
	}

	result = JNI_VERSION_1_4;
	LOG_INFO(TAG, "JNI_OnLoad success");
	
	bail:
	return result;
}

void play() {
	// Open the input file as a 'byte-stream file source':
	ByteStreamFileSource *fileSource = ByteStreamFileSource::createNew(*uEnv, inputFilename);
	if (fileSource == NULL) {
		LOG_ERR(TAG, "Unable to open file \"%s\" as a byte-stream file source", inputFilename);
		exit(1);
	}

	FramedSource* videoES = fileSource;
	
	// Create a framer for the Video Elementary Stream:
	videoSource = H264VideoStreamFramer::createNew(*uEnv, videoES);

	// Finally, start playing:
	//LOG_DEBUG(TAG, "Beginning to read from file...\n");
	videoSink->startPlaying(*videoSource, afterPlaying, videoSink);
}

void afterPlaying(void* /*clientData*/) {
	videoSink->stopPlaying();
	Medium::close(videoSource);
	// Note that this also closes the input file that this source read from.

	// Start playing once again:
	play();
}

int StreamerLiveCpature(void **data, int *len) {
	
	// Note that this also closes the input file that this source read from.
	*data = &frameBuff;
	*len = frameBuffLen;
	if(0 == frameBuffLen)
	{
		usleep(2000);
		//LOG_ERR(TAG, "No frameBuff");
		return H264_LIVE_CAPTURE_ERROR_READFRAME;
	}

	usleep(1000);
	return H264_LIVE_CAPTURE_SUCCESS;
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    nativeSetup
 * Signature: (Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_com_xproject_videoplus_Streamer_init(JNIEnv *env,jobject thiz, jobject weak_this)
{
	LOG_INFO(TAG, "init()");
	jclass clazz = env->GetObjectClass(thiz);
	mClass = (jclass)env->NewGlobalRef(clazz);
	mObject  = env->NewGlobalRef(weak_this);
	mEventCallbackID = env->GetStaticMethodID(mClass, "EventCallback", "(Ljava/lang/String;)V");
	return;
}

JNIEXPORT void JNICALL Java_com_xproject_videoplus_Streamer_start(JNIEnv *env,
		jobject obj, jstring filename, jint width, jint height,
		jint frameRate) {
	LOG_DEBUG(TAG, "start()");

	av_register_all();

	AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec) {
		LOG_ERR(TAG, "AV_CODEC_ID_H264 codec not found!");
		exit(1);
	}

	codecContext = avcodec_alloc_context3(codec);
	if (!codec) {
		LOG_ERR(TAG, "couldn't allocate codec context");
		exit(1);
	}

	/* put sample parameters */
	codecContext->bit_rate = 400000;
	/* resolution must be a multiple of two */
	codecContext->width = width;
	codecContext->height = height;
	/* frames per second */
	codecContext->time_base = (AVRational ) {1, frameRate};
	codecContext->gop_size = frameRate; /* emit one intra frame every ten frames */
	codecContext->max_b_frames = 1;
	codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

	av_opt_set(codecContext->priv_data, "profile", "baseline", 0);
	av_opt_set(codecContext->priv_data, "preset", "ultrafast", 0);

	if (avcodec_open2(codecContext, codec, NULL) < 0) {
		LOG_ERR(TAG, "couldn't open codec");
		exit(1);
	}

	inputFilename = env->GetStringUTFChars(filename, NULL);
	if(0 == strcmp("live", inputFilename)) {
		liveFlag = 1;
	}
	
#ifdef STREAM_SAVE_FILE
	if(0 == liveFlag) {
		file = fopen(inputFilename, "wb");
		if (!file) {
			LOG_ERR(TAG, "couldn't open %s", inputFilename);
			exit(1);
		}
	}
#endif

	frame = avcodec_alloc_frame();
	if (!frame) {
		LOG_ERR(TAG, "couldn't allocate frame");
		exit(1);
	}

	frame->format = codecContext->pix_fmt;
	frame->width = codecContext->width;
	frame->height = codecContext->height;

	if (av_image_alloc(frame->data, frame->linesize, codecContext->width,
			codecContext->height, codecContext->pix_fmt, 32) < 0) {
		LOG_ERR(TAG, "couldn't allocate raw picture buffer");
		exit(1);
	}

	tmpFrame = avcodec_alloc_frame();
	if (!tmpFrame) {
		LOG_ERR(TAG, "couldn't allocate frame");
		exit(1);
	}

	if (av_image_alloc(tmpFrame->data, tmpFrame->linesize, codecContext->width,
			codecContext->height, AV_PIX_FMT_NV21, 32) < 0) {
		LOG_ERR(TAG, "couldn't allocate raw picture buffer");
		exit(1);
	}

	count = 0;
}

JNIEXPORT int JNICALL Java_com_xproject_videoplus_Streamer_encode(JNIEnv *env,
		jobject obj, jbyteArray data) {
	av_init_packet(&packet);
	packet.data = NULL;
	packet.size = 0;

	swsContext = sws_getCachedContext(swsContext, codecContext->width,
			codecContext->height, AV_PIX_FMT_NV21, codecContext->width,
			codecContext->height, codecContext->pix_fmt, SWS_BILINEAR, NULL,
			NULL, NULL);

	jbyte *_data = env->GetByteArrayElements(data, NULL);

	avpicture_fill((AVPicture*) tmpFrame, (const unsigned char*) _data,
			AV_PIX_FMT_NV21, codecContext->width, codecContext->height);

	env->ReleaseByteArrayElements(data, _data, 0);

	sws_scale(swsContext, tmpFrame->data, tmpFrame->linesize, 0,
			codecContext->height, frame->data, frame->linesize);

	frame->pts = count;

	/* encode the image */
	if (avcodec_encode_video2(codecContext, &packet, frame, &got_output)) {
		LOG_ERR(TAG, "couldn't encode frame");
		exit(1);
	}

	if (got_output) {

		memset(frameBuff, 0x0, 100*1024);
		memcpy(frameBuff, packet.data, packet.size);
		frameBuffLen = packet.size;

		if(0 == liveFlag) {
	//		LOG_DEBUG(TAG, "write frame %3d (size=%5d)", count, packet.size);
		#ifdef STREAM_SAVE_FILE
			fwrite(packet.data, 1, packet.size, file);
		#endif
		}
		
		av_free_packet(&packet);
	}

	count++;
	return 0;
}

JNIEXPORT void JNICALL Java_com_xproject_videoplus_Streamer_snapshot(JNIEnv *env,
		jobject obj) {
	LOG_DEBUG(TAG, "snapshot()");
	sendMessage("snapshot");
}

JNIEXPORT void JNICALL Java_com_xproject_videoplus_Streamer_stop(JNIEnv *env,
		jobject obj) {
	LOG_DEBUG(TAG, "stop()");

	/* get the delayed frames */
	for (got_output = 1; got_output; count++) {
		if (avcodec_encode_video2(codecContext, &packet, NULL, &got_output) < 0) {
			LOG_ERR(TAG, "couldn't encode frame");
			exit(1);
		}

		if (got_output) {
			if(0 == liveFlag) {
			//LOG_DEBUG(TAG, "write frame %3d (size=%5d)", count, packet.size);
			#ifdef STREAM_SAVE_FILE
				fwrite(packet.data, 1, packet.size, file);
			#endif
			}
			av_free_packet(&packet);
		}
	}

	if(0 == liveFlag) {
#ifdef STREAM_SAVE_FILE
		uint8_t endcode[] = { 0, 0, 1, 0xb7 };

		/* add sequence end code to have a real mpeg file */
		fwrite(endcode, 1, sizeof(endcode), file);
		fclose(file);
#endif
	}

	liveFlag = 0;

	avcodec_close(codecContext);
	av_free(codecContext);
	av_freep(&frame->data[0]);
	avcodec_free_frame(&frame);
	avcodec_free_frame(&tmpFrame);
}

JNIEXPORT int JNICALL Java_com_xproject_videoplus_Streamer_loop(JNIEnv *env,
		jobject obj, jstring addr) {
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	uEnv = BasicUsageEnvironment::createNew(*scheduler);

	// Create 'groupsocks' for RTP and RTCP:
	struct in_addr destinationAddress;
	const char *_addr = env->GetStringUTFChars(addr, NULL);
	LOG_DEBUG(TAG, "Destination IP: \"%s\"", _addr);
	destinationAddress.s_addr = our_inet_addr(_addr); /*chooseRandomIPv4SSMAddress(*uEnv);*/	
	env->ReleaseStringUTFChars(addr, _addr);
	
	// Note: This is a multicast address.  If you wish instead to stream
	// using unicast, then you should use the "testOnDemandRTSPServer"
	// test program - not this test program - as a model.

	//多播地址
	//destinationAddress.s_addr = our_inet_addr("239.255.42.42");
	//LOG_DEBUG(TAG, "Destination IP: \"239.255.42.42\"");
	
	const unsigned short rtpPortNum = 18888;
	const unsigned short rtcpPortNum = rtpPortNum + 1;
	const unsigned char ttl = 8;

	const Port rtpPort(rtpPortNum);
	const Port rtcpPort(rtcpPortNum);

	Groupsock rtpGroupsock(*uEnv, destinationAddress, rtpPort, ttl);
	Groupsock rtcpGroupsock(*uEnv, destinationAddress, rtcpPort, ttl);

	// Create a 'H264 Video RTP' sink from the RTP 'groupsock':
	OutPacketBuffer::maxSize = 100000;
	videoSink = H264VideoRTPSink::createNew(*uEnv, &rtpGroupsock, 96);

	// Create (and start) a 'RTCP instance' for this RTP sink:
	const unsigned estimatedSessionBandwidth = 500; // in kbps; for RTCP b/w share
	const unsigned maxCNAMElen = 100;
	unsigned char CNAME[maxCNAMElen + 1];
	gethostname((char*) CNAME, maxCNAMElen);
	CNAME[maxCNAMElen] = '\0'; // just in case

	RTCPInstance* rtcp = RTCPInstance::createNew(*uEnv, &rtcpGroupsock,
			estimatedSessionBandwidth, CNAME, videoSink, NULL /* we're a server */, True /* we're a SSM source */);

	// Note: This starts RTCP running automatically
	RTSPServer* rtspServer = RTSPServer::createNew(*uEnv, 8554);
	if (rtspServer == NULL) {
		LOG_ERR(TAG, "Failed to create RTSP server: %s", uEnv->getResultMsg());
		exit(1);
	}

	ServerMediaSession* sms = ServerMediaSession::createNew(*uEnv, "streamer",
			inputFilename, "Session streamed by \"Streamer\"", True /*SSM*/);
	sms->addSubsession(PassiveServerMediaSubsession::createNew(*videoSink, rtcp));
	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	LOG_DEBUG(TAG, "Play this Stream using the URL: \"%s\"", url);
	delete[] url;

	// Start the streaming:
	LOG_DEBUG(TAG, "Now Beginning streaming\n");
	play();

	uEnv->taskScheduler().doEventLoop(); // does not return
}

JNIEXPORT int JNICALL Java_com_xproject_videoplus_Streamer_live(JNIEnv *env,
		jobject obj, jstring addr) {
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	uEnv = BasicUsageEnvironment::createNew(*scheduler);

	// Create 'groupsocks' for RTP and RTCP:
	struct in_addr destinationAddress;
	const char *_addr = env->GetStringUTFChars(addr, NULL);
	LOG_DEBUG(TAG, "Destination IP: \"%s\"", _addr);
	destinationAddress.s_addr = our_inet_addr(_addr); /*chooseRandomIPv4SSMAddress(*uEnv);*/
	env->ReleaseStringUTFChars(addr, _addr);

	// Note: This is a multicast address.  If you wish instead to stream
	// using unicast, then you should use the "testOnDemandRTSPServer"
	// test program - not this test program - as a model.

	//多播地址
	destinationAddress.s_addr = our_inet_addr("239.255.42.42");
	LOG_DEBUG(TAG, "Destination IP: \"239.255.42.42\"");

	// Note: This starts RTCP running automatically
	portNumBits rtspServerPortNum = 554;
	RTSPServer* rtspServer = RTSPServer::createNew(*uEnv, 8554);
    if (rtspServer == NULL) 
    {
        rtspServerPortNum = 8554;
        rtspServer = RTSPServer::createNew(*uEnv, rtspServerPortNum, NULL);
    }
	
	if (rtspServer == NULL) {
		LOG_ERR(TAG, "Failed to create RTSP server: %s", uEnv->getResultMsg());
		sendMessage("CreateRtspServerFail");
		exit(1);
	}

	ServerMediaSession* sms = ServerMediaSession::createNew(*uEnv, "streamer",
			0, "Camera server, streamed by the LIVE555 Media Server");

	sms->addSubsession(H264LiveServerMediaSubsession::createNew(*uEnv,
		"/dev/video0", 640, 480, 25));

	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);
	LOG_DEBUG(TAG, "Play this Stream using the URL: \"%s\"", url);
	delete[] url;

	// Start the streaming:
	LOG_DEBUG(TAG, "Now Beginning streaming\n");
	uEnv->taskScheduler().doEventLoop(); // does not return
}



