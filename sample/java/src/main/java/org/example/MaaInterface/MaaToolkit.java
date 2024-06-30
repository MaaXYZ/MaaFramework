package org.example.MaaInterface;

import com.sun.jna.Library;
import com.sun.jna.Native;
import org.example.constant.DllDirConstant;


public interface MaaToolkit extends Library {

    public static MaaToolkit  INSTANCE =  Native.load(DllDirConstant.getDllDirPath()+"MaaToolkit.dll", MaaToolkit.class);


    //阻塞等待设备查找完成
    public Integer MaaToolkitWaitForFindDeviceToComplete();

    //判断设备查找是否已完成
    public Boolean MaaToolkitIsFindDeviceCompleted();

    //异步发起设备查找任务
    public Boolean MaaToolkitPostFindDevice();

    //设备数量
    public Integer MaaToolkitGetDeviceCount();


    // 根据索引获取设备名称
    public String MaaToolkitGetDeviceName(Long index);

    // 根据索引获取设备的ADB路径
    public String MaaToolkitGetDeviceAdbPath(Long index);

    // 根据索引获取设备的ADB序列号
    public String MaaToolkitGetDeviceAdbSerial(Long index);

    // 根据索引获取设备的ADB控制器类型
    public Integer MaaToolkitGetDeviceAdbControllerType(Long index);

    // 根据索引获取设备的ADB配置信息
    public String MaaToolkitGetDeviceAdbConfig(Long index);

    /**
     * // 初始化配置信息
     * @param userPath  路径
     * @param param   "{}"
     * @return
     */
    public Boolean MaaToolkitInitOptionConfig(String userPath,String param);
}
