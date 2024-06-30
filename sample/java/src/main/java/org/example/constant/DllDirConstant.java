package org.example.constant;

import com.sun.jna.Platform;

import java.net.URL;

public class DllDirConstant {

    public static String getDllDirPath() {
        URL maaFramework_url = DllDirConstant.class.getClassLoader().getResource("");

        String path = null;
        if (Platform.isWindows()) {
            path = maaFramework_url.getPath().substring(1).replaceAll("/", "\\\\\\\\");
            path += "maadll\\\\_x64\\\\";
        } else if (Platform.isLinux()) {
            //未测试
            path += "/maaDll/_liunx/";
        } else if (Platform.isMac()) {
            //未测试
            path += "/maaDll/_x64/";
        } else {
            throw new UnsupportedOperationException(" 未知平台 platform");
        }

        return path;
    }

}
