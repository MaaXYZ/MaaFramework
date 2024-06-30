package org.example.MaaInterface;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import org.example.constant.DllDirConstant;


public interface MaaFramework extends Library {

    //    public MaaFramework INSTANCE = Native.load(DllDirConstant.getDllDirPath()+"\\MaaFramework.dll", MaaFramework.class);
    MaaFramework INSTANCE = Native.load(DllDirConstant.getDllDirPath()+"MaaFramework.dll", MaaFramework.class);


    public Pointer MaaAdbControllerCreateV2(String adb_path, String address, Integer type, String config, String agent_path, Pointer callback, Pointer callback_arg);

    public Long MaaControllerPostConnection(Pointer ctrl);


    public Pointer MaaResourceCreate(Pointer ctrl, Pointer MaaTransparentArg);

    public Long MaaResourcePostPath(Pointer resource_handle, String resource_dir);

    public Pointer MaaCreate(Pointer maaAPICallback, Pointer MaaTransparentArg);


    public Integer MaaControllerWait(Pointer controller_handle, Long ctrl_id);

    public Integer MaaResourceWait(Pointer resource_handle, Long ctrl_id);

    public byte MaaBindResource(Pointer maaInstanceAPI, Pointer resource_handle);

    public byte MaaBindController(Pointer maaInstanceAPI, Pointer resource_handle);


    public void MaaDestroy(Pointer controllerHandle);

    public void MaaResourceDestroy(Pointer resource_handle);

    public void MaaControllerDestroy(Pointer controllerHandle);

    public Boolean MaaInited(Pointer controllerHandle);


    public Long MaaPostTask(Pointer maaHandle, String entry, String param);

    public Long MaaWaitTask(Pointer maaHandle, Long taskId);

    public Boolean MaaPostStop(Pointer maaHandle);


    public Boolean MaaSetGlobalOption(Integer MaaGlobalOption, Long MaaOptionValue, Long MaaOptionValueSize);

}
