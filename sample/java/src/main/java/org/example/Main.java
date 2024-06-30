package org.example;

import com.sun.jna.Platform;
import com.sun.jna.Pointer;
import org.example.MaaInterface.MaaFramework;
import org.example.MaaInterface.MaaToolkit;
import org.example.constant.DllDirConstant;

import java.net.URL;

public class Main {

    public static void main(String[] args) throws Exception {
        //  resources/maaDll  编译的 maa 动态链接库
        //  maaResource/maaResource   为自定义任务

        System.out.println("初始化配置信息 = " + MaaToolkit.INSTANCE.MaaToolkitInitOptionConfig("./", "{}"));
        System.out.println("异步发起设备查找任务 = " + MaaToolkit.INSTANCE.MaaToolkitPostFindDevice());
        System.out.println("阻塞等待设备查找完成 = " + MaaToolkit.INSTANCE.MaaToolkitWaitForFindDeviceToComplete());

        //安卓设备数量
        long i = MaaToolkit.INSTANCE.MaaToolkitWaitForFindDeviceToComplete().longValue();
        if (i == 0){
            System.out.println("未打开模拟器!");
            return;
        }
        String agent_path = "share/MaaAgentBinary";

        String MaaToolkitGetDeviceAdbPath_v = MaaToolkit.INSTANCE.MaaToolkitGetDeviceAdbPath(0L);
        System.out.println("MaaToolkitGetDeviceAdbPath_v = " + MaaToolkitGetDeviceAdbPath_v);
        String MaaToolkitGetDeviceAdbSerial_v = MaaToolkit.INSTANCE.MaaToolkitGetDeviceAdbSerial(0L);
        System.out.println("MaaToolkitGetDeviceAdbSerial_v = " + MaaToolkitGetDeviceAdbSerial_v);
        Integer MaaToolkitGetDeviceAdbControllerType_v = MaaToolkit.INSTANCE.MaaToolkitGetDeviceAdbControllerType(0L);
        System.out.println("MaaToolkitGetDeviceAdbControllerType_v = " + MaaToolkitGetDeviceAdbControllerType_v);
        String MaaToolkitGetDeviceAdbConfig_v = MaaToolkit.INSTANCE.MaaToolkitGetDeviceAdbConfig(0L);
        System.out.println("MaaToolkitGetDeviceAdbConfig_v = " + MaaToolkitGetDeviceAdbConfig_v);
        Pointer controllerHandle = MaaFramework.INSTANCE.MaaAdbControllerCreateV2(
                MaaToolkitGetDeviceAdbPath_v,
                MaaToolkitGetDeviceAdbSerial_v,
                MaaToolkitGetDeviceAdbControllerType_v,
                MaaToolkitGetDeviceAdbConfig_v,
                agent_path,
                null,
                null
        );

        System.out.println("controllerHandle = " + controllerHandle);
        Long ctrl_id = MaaFramework.INSTANCE.MaaControllerPostConnection(controllerHandle);
        System.out.println("ctrl_id = " + ctrl_id);
        Pointer resource_handle = MaaFramework.INSTANCE.MaaResourceCreate(null, null);
        System.out.println("resource_handle = " + resource_handle);

//        String resource_dir = "D:\\dev\\java_jni\\test_jna-maa\\test-jna-maa\\src\\main\\java\\org\\example\\resource";
        String resource_dir = null;
        if (Platform.isWindows()){

            resource_dir += DllDirConstant.getDllDirPath()+"maaResource\\\\";
        }else {
            //其他平台

        }

        Long resource_id = MaaFramework.INSTANCE.MaaResourcePostPath(resource_handle, resource_dir);


        Integer i2 = MaaFramework.INSTANCE.MaaControllerWait(controllerHandle, ctrl_id);
        System.out.println("i2 = " + i2);
        Integer i1 = MaaFramework.INSTANCE.MaaResourceWait(resource_handle, resource_id);
        System.out.println("i1 = " + i1);


        Pointer maa_handle = MaaFramework.INSTANCE.MaaCreate(null, null);
        System.out.println("maa_handle = " + maa_handle);
        byte b = MaaFramework.INSTANCE.MaaBindResource(maa_handle, resource_handle);
        byte b1 = MaaFramework.INSTANCE.MaaBindController(maa_handle, controllerHandle);


        Boolean b2 = MaaFramework.INSTANCE.MaaInited(controllerHandle);
        if (!b2) {
            System.out.println(" 初始化 MAA 失败");

            MaaFramework.INSTANCE.MaaDestroy(maa_handle);
            MaaFramework.INSTANCE.MaaResourceDestroy(resource_handle);
            MaaFramework.INSTANCE.MaaControllerDestroy(controllerHandle);
            return;
        }

        for (int i3 = 0; i3 < 2; i3++) {
            //执行任务
            Long taskId = MaaFramework.INSTANCE.MaaPostTask(maa_handle, "test22", "{}");
            System.out.println("taskId = " + taskId);
            //等待任务执行完成
            Long test22 = MaaFramework.INSTANCE.MaaWaitTask(maa_handle, taskId);
            Thread.sleep(1000);
            if (test22 != null) {
                System.out.println("test22 = " + test22);
            }
        }

        // 创建的 adb 无法关闭 未解决
        MaaFramework.INSTANCE.MaaPostStop(maa_handle);

        MaaFramework.INSTANCE.MaaDestroy(maa_handle);

        MaaFramework.INSTANCE.MaaResourceDestroy(resource_handle);

        MaaFramework.INSTANCE.MaaControllerDestroy(controllerHandle);


        System.out.println("结束");
    }
}