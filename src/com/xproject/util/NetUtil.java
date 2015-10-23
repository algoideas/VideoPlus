package com.xproject.util;

import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.Enumeration;

import org.apache.http.conn.util.InetAddressUtils;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

/**
 * @类描述 
 * @项目名称 VideoPlus
 * @包名  com.xproject.util
 * @类名称 NetUtil
 * @author MinJie.Yu
 * @创建时间 2015-10-23下午2:09:39
 * @修改人 MinJie.Yu
 * @修改时间 2015-10-23下午2:09:39
 * @修改备注 
 * @version v1.0
 * @see [nothing]
 */
public class NetUtil {
		
	static final String TAG = "NetUtil";
	
	/**
	 * @描述:
	 * @方法名 isNetAvailable
	 * @param context
	 * @return
	 * @return boolean
	 * @author MinJie.Yu
	 * @创建时间 2015-10-23下午2:09:51
	 * @修改人 MinJie.Yu
	 * @修改时间 2015-10-23下午2:09:51
	 * @修改备注
	 * @since
	 * @throws
	 */
	public static boolean isNetAvailable(Context context) {
		ConnectivityManager connectivityManager = (ConnectivityManager) context
				.getSystemService(Context.CONNECTIVITY_SERVICE);
		NetworkInfo activeNetworkInfo = connectivityManager
				.getActiveNetworkInfo();
		return activeNetworkInfo != null && activeNetworkInfo.isAvailable();
	}

	/**
	 * @描述:
	 * @方法名 isWIFIActivate
	 * @param context
	 * @return
	 * @return boolean
	 * @author MinJie.Yu
	 * @创建时间 2015-10-23下午2:09:28
	 * @修改人 MinJie.Yu
	 * @修改时间 2015-10-23下午2:09:28
	 * @修改备注
	 * @since
	 * @throws
	 */
	public static boolean isWIFIActivate(Context context) {
		return ((WifiManager) context.getSystemService(Context.WIFI_SERVICE))
				.isWifiEnabled();
	}
	
	/**
	 * @描述:
	 * @方法名 changeWIFIStatus
	 * @param context
	 * @param status
	 * @return void
	 * @author MinJie.Yu
	 * @创建时间 2015-10-23下午2:09:21
	 * @修改人 MinJie.Yu
	 * @修改时间 2015-10-23下午2:09:21
	 * @修改备注
	 * @since
	 * @throws
	 */
	public static void changeWIFIStatus(Context context, boolean status) {
		((WifiManager) context.getSystemService(Context.WIFI_SERVICE))
				.setWifiEnabled(status);
	}
	
    /**
     * Get local IP address of IPV4
     * CN:获取本地IPv4地址。
     * @return ipv4 address.
     */
	public static String getHostIpAddress() {
        String ipv4 = null;
        
        try
        {
            Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces();
            if (en != null)
            {
                while (en.hasMoreElements())
                {
                    NetworkInterface intf = en.nextElement();
                    for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr
                        .hasMoreElements();)
                    {
                        InetAddress inetAddress = enumIpAddr.nextElement();
                        if (!inetAddress.isLoopbackAddress()
                            && InetAddressUtils.isIPv4Address(ipv4 = inetAddress.getHostAddress()))
                        {
                            return ipv4;
                        }
                    }
                }
            }

        }
        catch (SocketException ex)
        {
            Log.e(TAG, "getHostIpAddress()" + ex);
        }
        catch (NullPointerException ex)
        {
        	Log.e(TAG, "getHostIpAddress()" + ex);
        }
        return ipv4;
    }

	public static boolean isLocalIpAddress(String checkip)  {  
  		boolean ret=false;
  		if(checkip != null)
  		{
  			try 
  			{  
                for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) 
                {  
                    NetworkInterface intf = en.nextElement();  
                    for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) 
                    {
                        InetAddress inetAddress = enumIpAddr.nextElement();  
                        if (!inetAddress.isLoopbackAddress()) 
                        {
                      	  String ip = inetAddress.getHostAddress().toString();

                      	  if(ip == null)
                      	  {
                      		  continue;
                      	  }
                      	  if(checkip.equals(ip))
                      	  {
                      		  return true;
                      	  }
                        }
                    }
                }
            }
  			catch (SocketException ex) 
            {
          	  ex.printStackTrace();
            }
  		}
  		
		Log.d(TAG,"IP = " + checkip);
  		return ret;
    } 
	
	public static String getLocalIpAddress() {
		try {
			// 遍历网络接口
			Enumeration<NetworkInterface> infos = NetworkInterface
					.getNetworkInterfaces();
			while (infos.hasMoreElements()) {
				// 获取网络接口
				NetworkInterface niFace = infos.nextElement();
				Enumeration<InetAddress> enumIpAddr = niFace.getInetAddresses();
				while (enumIpAddr.hasMoreElements()) {
					InetAddress mInetAddress = enumIpAddr.nextElement();
					// 所获取的网络地址不是127.0.0.1时返回得得到的IP
					if (!mInetAddress.isLoopbackAddress()
							&& InetAddressUtils.isIPv4Address(mInetAddress
									.getHostAddress())) {
						return mInetAddress.getHostAddress().toString();
					}
				}
			}
		} catch (SocketException e) {

		}
		return null;
	}
}
