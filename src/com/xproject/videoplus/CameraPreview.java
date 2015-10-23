
package com.xproject.videoplus;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;

import com.xproject.util.FileUtil;
import com.xproject.util.ImageUtil;

/**
 * @类描述 
 * @项目名称 RTSPStreamer
 * @包名  com.xproject.videoplus
 * @类名称 CameraPreview
 * @author MinJie.Yu
 * @创建时间 2015-10-21下午4:07:57
 * @修改人 MinJie.Yu
 * @修改时间 2015-10-21下午4:07:57
 * @修改备注 
 * @version v1.0
 * @see [nothing]
 */
public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback {
    private static final String TAG = "CameraPreview";

    private SurfaceHolder mHolder;
    private Camera mCamera;
    private Streamer mStreamer;
    private boolean isdoTakePicture = false;

    public CameraPreview(Context context, Camera camera, Streamer streamer) {
        super(context);
        mCamera = camera;
        mStreamer = streamer;

        // Install a SurfaceHolder.Callback so we get notified when the
        // underlying surface is created and destroyed.
        mHolder = getHolder();
        mHolder.addCallback(this);
        // deprecated setting, but required on Android versions prior to 3.0
        mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
    }

    public void surfaceCreated(SurfaceHolder holder) {
        // The Surface has been created, now tell the camera where to draw the
        // preview.
        try {
            mCamera.setPreviewDisplay(holder);
            mCamera.setPreviewCallback(mStreamer);
            mCamera.startPreview();
        } catch (IOException e) {
            Log.d(TAG, "Error setting camera preview: " + e.getMessage());
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        // empty. Take care of releasing the Camera preview in your activity.
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
        // If your preview can change or rotate, take care of those events here.
        // Make sure to stop the preview before resizing or reformatting it.

        if (mHolder.getSurface() == null) {
            // preview surface does not exist
            return;
        }

        // stop preview before making changes
        try {
            mCamera.stopPreview();
        } catch (Exception e) {
            // ignore: tried to stop a non-existent preview
        }

        // set preview size and make any resize, rotate or
        // reformatting changes here

        // start preview with new settings
        try {
            mCamera.setPreviewDisplay(mHolder);
            mCamera.setPreviewCallback(mStreamer);
            mCamera.startPreview();

        } catch (Exception e) {
            Log.d(TAG, "Error starting camera preview: " + e.getMessage());
        }
    }
    
	/**
	 * 拍照
	 */
	public void doTakePicture(){
		if(mCamera != null) {
			isdoTakePicture = true;
			mCamera.takePicture(mShutterCallback, null, mJpegPictureCallback);
		}
	}

	/*为了实现拍照的快门声音及拍照保存照片需要下面三个回调变量*/
	ShutterCallback mShutterCallback = new ShutterCallback() {
		//快门按下的回调，在这里我们可以设置类似播放“咔嚓”声之类的操作。默认的就是咔嚓。
		public void onShutter() {
			Log.i(TAG, "myShutterCallback:onShutter...");
		}
	};
	
	//对jpeg图像数据的回调,最重要的一个回调
	PictureCallback mJpegPictureCallback = new PictureCallback() {
		public void onPictureTaken(byte[] data, Camera camera) {
			if(true == isdoTakePicture) {
				Log.i(TAG, "myJpegCallback:onPictureTaken...");
				Bitmap b = null;
				if(null != data){
					b = BitmapFactory.decodeByteArray(data, 0, data.length);//data是字节数据，将其解析成位图
					mCamera.stopPreview();
					isdoTakePicture = false;
				}
				
				//保存图片到sdcard
				if(null != b)
				{
					//设置FOCUS_MODE_CONTINUOUS_VIDEO)之后，myParam.set("rotation", 90)失效。
					//图片竟然不能旋转了，故这里要旋转下
					Bitmap rotaBitmap = ImageUtil.getRotateBitmap(b, 90.0f);
					FileUtil.saveBitmap(rotaBitmap);
				}
				//再次进入预览
				mCamera.startPreview();
				isdoTakePicture = false;
			}
		}
	};
	
}
