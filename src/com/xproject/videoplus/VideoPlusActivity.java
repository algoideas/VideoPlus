package com.xproject.videoplus;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.hardware.Camera.CameraInfo;
import android.hardware.Camera.Size;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.Toast;

import com.xproject.util.DialogUtils;
import com.xproject.util.NetUtil;
import com.xproject.videoplus.R;
import com.xproject.videoplus.BaseApplication.OnServiceConnectedListener;

import java.io.File;
import java.util.Calendar;
import java.util.List;

public class VideoPlusActivity extends Activity implements OnServiceConnectedListener {
	
    private static final String TAG = "VideoPlusActivity";
    private SharedPreferences mPrefs;
    private CameraPreview mPreview;
    private Camera mCamera;
    private Streamer mStreamer;
    private int mStreamFlag = 0;
    private long mExitTime;
    private final String mLocalIpAddr = NetUtil.getLocalIpAddress();
    
    @Override
    public void onCreate(Bundle savedInstanceState) {
        if (BaseApplication.DEBUG) {
            Log.d(TAG, "onCreate()");
        }
        
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_videoplus);

		File directory = new File(Environment.getExternalStorageDirectory()+File.separator+"VideoPlus");
		directory.mkdirs();
		
		DialogUtils.setCurContext(getApplicationContext());
        BaseApplication.getInstance().setOnServiceConnectedListener(this);
        
        mPrefs = PreferenceManager.getDefaultSharedPreferences(this);

        // Create an instance of Camera
        mCamera = getCameraInstance();
        if(null == mCamera) {
        	Log.e(TAG, "No Camera found");
			DialogUtils.showDialog(VideoPlusActivity.this,  "温馨提示",  "未找到摄像头！", new DialogInterface.OnClickListener()  {
	            @Override
	            public void onClick(DialogInterface dialog, int which) {
	
	            }
	        });
        }
        
        Camera.Parameters params = mCamera.getParameters();

        /**
         * 显示支持的格式
         */
        List<Integer> supportedPreviewFormats = params.getSupportedPreviewFormats();
        for (int i = 0; i < supportedPreviewFormats.size(); i++) {
        	if(0 == i) {
        		Log.d(TAG, "SupportedPreviewFormats:");
        	}
        	
            Log.d(TAG, "Format[" + i + "] = " + getImageFormatString(supportedPreviewFormats.get(i)));
        }
        
        params.setPreviewFormat(ImageFormat.NV21);

        /**
         * 显示支持的分辨率
         */
        List<Size> supportedPreviewSizes = params.getSupportedPreviewSizes();
        for (int i = 0; i < supportedPreviewSizes.size(); i++) {
        	if(0 == i) {
        		Log.d(TAG, "SupportedPreviewSizes:");
        	}
            Log.d(TAG, "PreviewSize[" + i + "] = " + supportedPreviewSizes.get(i).width + "x" + supportedPreviewSizes.get(i).height);
        }
        
        params.setPreviewSize(Streamer.WIDTH, Streamer.HEIGHT);
        
        /**
         * 显示支持的帧率
         */
        List<Integer> supportedPreviewFrameRates = params.getSupportedPreviewFrameRates();
        for (int i = 0; i < supportedPreviewFrameRates.size(); i++) {
        	if(0 == i) {
        		Log.d(TAG, "SupportedPreviewFrameRates:");
        	}
            Log.d(TAG, "FrameRate[" + i + "] = " + supportedPreviewFrameRates.get(i));
        }
        
        params.setPreviewFrameRate(Streamer.FRAME_RATE);
        mCamera.setParameters(params);

        mStreamer = new Streamer();
        mStreamer.Init(VideoPlusActivity.this);

        // Create our Preview view and set it as the content of our activity.
        mPreview = new CameraPreview(this, mCamera, mStreamer);
        FrameLayout preview = (FrameLayout) findViewById(R.id.camera_preview);
        preview.addView(mPreview);

        Button captureButton = (Button) findViewById(R.id.record);
        captureButton.setOnClickListener(
                new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                        if (!mStreamer.isStarted()) {
                        	String filename= Environment.getExternalStorageDirectory().toString()+"/VideoPlus/"
                        			+ "VID_"
                        			+ Calendar.getInstance().get(Calendar.HOUR_OF_DAY) + "_"
                        			+ Calendar.getInstance().get(Calendar.MINUTE)  + "_"
                        			+ Calendar.getInstance().get(Calendar.SECOND) +".H264";
                        	
                        	mStreamer.Record(filename);
                        	((Button) v).setText("Stop");
                        } else {
                            mStreamer.Stop();
                            ((Button) v).setText("Record");
                        }
                    }
                }
                );

        Button mButtonStream = (Button) findViewById(R.id.stream);
        mButtonStream.setOnClickListener(
                new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                    	
	                        if (!mStreamer.isStarted()) {
	                        	if(0 == mStreamFlag) {
	                        		
	                        		/**
	                        		 *	显示播放地址对话框 
	                        		 */
	                    			DialogUtils.showDialog(VideoPlusActivity.this,  "播放地址",  "rtsp://" + mLocalIpAddr + ":8554/streamer", new DialogInterface.OnClickListener()  {
	                                    @Override
	                                    public void onClick(DialogInterface dialog, int which) {
	                                    	/**
	                                    	 * 流模式
	                                    	 */
	                                		mStreamer.Live(mLocalIpAddr);
	                                		Log.d(TAG, "LocalIpAddr = " + mLocalIpAddr);
	                                    }
	                                });
	                        	}
	                        } else {
	                        	if(1 == mStreamFlag) {
	                        		mStreamer.Stop();
	                        		((Button) v).setText("Stream ...");
	                        	}
	                        }
                    	}
                }
        		);
        
        Button mSnapButton = (Button) findViewById(R.id.snap);
        mSnapButton.setOnClickListener(
                new View.OnClickListener() {

                    @Override
                    public void onClick(View v) {
                    	mStreamer.SnapShot();
                        mCamera.autoFocus(new AutoFocusCallback() {
                            @Override
                            public void onAutoFocus(boolean success, Camera camera) {
                            	
                            }
                        });
                    }
                }
                );
    }

    @Override
    protected void onDestroy() {
        if (BaseApplication.DEBUG)
            Log.d(TAG, "onDestroy()");
        super.onDestroy();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_streamer, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.menu_exit:
                finish();
                BaseApplication.getInstance().requestExit();
                return true;
        }
        return false;
    }

    @Override
    public void onServiceConnected() {
        if (BaseApplication.DEBUG)
            Log.d(TAG, "onServiceConnected()");
    }

    @Override
    protected void onPause() {
        super.onPause();
        releaseCamera(); // release the camera immediately on pause event
    }

    /** A safe way to get an instance of the Camera object. */
    public static Camera getCameraInstance() {
        Camera c = null;
        try {
            c = Camera.open(); // attempt to get a Camera instance
        } catch (Exception e) {
            // Camera is not available (in use or does not exist)
        }

        return c; // returns null if camera is unavailable
    }
    
    private void releaseCamera() {
        if (mCamera != null) {
            mCamera.release(); // release the camera for other applications
            mCamera = null;
        }
    }

    public static String getImageFormatString(int imageFormat) {
        switch (imageFormat) {
            case ImageFormat.JPEG:
                return "JPEG";
            case ImageFormat.NV16:
                return "NV16";
            case ImageFormat.NV21:
                return "NV21";
            case ImageFormat.RGB_565:
                return "RGB_565";
            case ImageFormat.YUY2:
                return "YUY2";
            case ImageFormat.YV12:
                return "YV12";
            default:
                return "UNKNOWN";
        }
    }
    
    private void showAddrDialog(final View v) {
        final EditText input = new EditText(this);
        input.setText(mPrefs.getString("addr", ""));
        new AlertDialog.Builder(this)
                .setTitle("Destination IP")
                .setView(input)
                .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int whichButton) {
                    	/**
                    	 * 流模式
                    	 */
                		mStreamer.Live(mLocalIpAddr);
                		
                    	/**
                    	 * 文件模式
                    	 */
                        /*String addr = input.getText().toString();
                        if (addr.matches("^([1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])(\\.([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}$")) {
                            mPrefs.edit().putString("addr", addr).commit();
                            ((Button) v).setText("Stop");
                            
                        	String filename= Environment.getExternalStorageDirectory().toString()+"/Stream/"+"streamer.H264";
                            mStreamer.Start(addr, filename);
                            
                            mStreamFlag = 1;
                        } else {
                            Toast.makeText(VideoPlusActivity.this, "Check IP!", Toast.LENGTH_SHORT).show();
                        }*/
                    }
                }).setNegativeButton(android.R.string.cancel, null).show();
    }
    
    /**
     * @描述:	判断是否存在前置摄像头
     * @方法名 checkCameraFacing
     * @param facing
     * @return
     * @return boolean
     * @author MinJie.Yu
     * @创建时间 2015-10-23下午12:17:55
     * @修改人 MinJie.Yu
     * @修改时间 2015-10-23下午12:17:55
     * @修改备注
     * @since
     * @throws
     */
    private static boolean checkCameraFacing(final int facing) {
        if (getSdkVersion() < Build.VERSION_CODES.GINGERBREAD) {
            return false;
        }
        
        final int cameraCount = Camera.getNumberOfCameras();
        CameraInfo info = new CameraInfo();
        for (int i = 0; i < cameraCount; i++) {
            Camera.getCameraInfo(i, info);
            if (facing == info.facing) {
                return true;
            }
        }
        return false;
    }
    
    public static boolean hasBackFacingCamera() {
        final int CAMERA_FACING_BACK = 0;
        return checkCameraFacing(CAMERA_FACING_BACK);
    }
    
    public static boolean hasFrontFacingCamera() {
        final int CAMERA_FACING_BACK = 1;
        return checkCameraFacing(CAMERA_FACING_BACK);
    }
    
    public static int getSdkVersion() {
        return android.os.Build.VERSION.SDK_INT;
    }

    private boolean checkCameraHardware(Context context) {
        if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_CAMERA)){
            // this device has a camera
            return true;
        } else {
            // no camera on this device
            return false;
        }
    }

	public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
				if ((System.currentTimeMillis() - mExitTime ) > 2000) {
                        Toast.makeText(this, "再按一次退出程序", Toast.LENGTH_SHORT).show();
                        mExitTime = System.currentTimeMillis();
                } else {
	                    if (mStreamer.isStarted()) {
	                        mStreamer.Stop();
	                    }
	                    
                        finish();
                        BaseApplication.getInstance().requestExit();
                }
                return true;
        }
        
        return super.onKeyDown(keyCode, event);
	}
}
