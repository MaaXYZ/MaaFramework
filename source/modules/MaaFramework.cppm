module;

#include <MaaFramework/MaaAPI.h>

export module maa.core;

// MaaDef.h

export using ::MaaBool;
export using ::MaaSize;
export constexpr auto _MaaNullSize = MaaNullSize;

export using ::MaaId;
export using ::MaaCtrlId;
export using ::MaaResId;
export using ::MaaTaskId;
export using ::MaaRecoId;
export using ::MaaActId;
export using ::MaaNodeId;
export constexpr auto _MaaInvalidId = MaaInvalidId;

export using ::MaaStringBuffer;
export using ::MaaImageBuffer;
export using ::MaaStringListBuffer;
export using ::MaaImageListBuffer;

export using ::MaaResource;
export using ::MaaController;
export using ::MaaTasker;
export using ::MaaContext;

export using ::MaaStatus;
export using ::MaaStatusEnum;

export using ::MaaLoggingLevel;
export using ::MaaLoggingLevelEnum;

export using ::MaaOption;
export using ::MaaOptionValue;
export using ::MaaOptionValueSize;

export using ::MaaGlobalOption;
export using ::MaaGlobalOptionEnum;

export using ::MaaResOption;
export using ::MaaResOptionEnum;
export using ::MaaInferenceDevice;
export using ::MaaInferenceDeviceEnum;
export using ::MaaInferenceExecutionProvider;
export using ::MaaInferenceExecutionProviderEnum;

export using ::MaaCtrlOption;
export using ::MaaCtrlOptionEnum;

export using ::MaaTaskerOption;
export using ::MaaTaskerOptionEnum;

export using ::MaaAdbScreencapMethod;
export constexpr auto _MaaAdbScreencapMethod_EncodeToFileAndPull = MaaAdbScreencapMethod_EncodeToFileAndPull;
export constexpr auto _MaaAdbScreencapMethod_Encode = MaaAdbScreencapMethod_Encode;
export constexpr auto _MaaAdbScreencapMethod_RawWithGzip = MaaAdbScreencapMethod_RawWithGzip;
export constexpr auto _MaaAdbScreencapMethod_RawByNetcat = MaaAdbScreencapMethod_RawByNetcat;
export constexpr auto _MaaAdbScreencapMethod_MinicapDirect = MaaAdbScreencapMethod_MinicapDirect;
export constexpr auto _MaaAdbScreencapMethod_MinicapStream = MaaAdbScreencapMethod_MinicapStream;
export constexpr auto _MaaAdbScreencapMethod_EmulatorExtras = MaaAdbScreencapMethod_EmulatorExtras;
export constexpr auto _MaaAdbScreencapMethod_None = MaaAdbScreencapMethod_None;
export constexpr auto _MaaAdbScreencapMethod_All = MaaAdbScreencapMethod_All;
export constexpr auto _MaaAdbScreencapMethod_Default = MaaAdbScreencapMethod_Default;

export using ::MaaAdbInputMethod;
export constexpr auto _MaaAdbInputMethod_AdbShell = MaaAdbInputMethod_AdbShell;
export constexpr auto _MaaAdbInputMethod_MinitouchAndAdbKey = MaaAdbInputMethod_MinitouchAndAdbKey;
export constexpr auto _MaaAdbInputMethod_Maatouch = MaaAdbInputMethod_Maatouch;
export constexpr auto _MaaAdbInputMethod_EmulatorExtras = MaaAdbInputMethod_EmulatorExtras;
export constexpr auto _MaaAdbInputMethod_None = MaaAdbInputMethod_None;
export constexpr auto _MaaAdbInputMethod_All = MaaAdbInputMethod_All;
export constexpr auto _MaaAdbInputMethod_Default = MaaAdbInputMethod_Default;

export using ::MaaWin32ScreencapMethod;
export constexpr auto _MaaWin32ScreencapMethod_None = MaaWin32ScreencapMethod_None;
export constexpr auto _MaaWin32ScreencapMethod_GDI = MaaWin32ScreencapMethod_GDI;
export constexpr auto _MaaWin32ScreencapMethod_FramePool = MaaWin32ScreencapMethod_FramePool;
export constexpr auto _MaaWin32ScreencapMethod_DXGI_DesktopDup = MaaWin32ScreencapMethod_DXGI_DesktopDup;

export using ::MaaWin32InputMethod;
export constexpr auto _MaaWin32InputMethod_None = MaaWin32InputMethod_None;
export constexpr auto _MaaWin32InputMethod_Seize = MaaWin32InputMethod_Seize;
export constexpr auto _MaaWin32InputMethod_SendMessage = MaaWin32InputMethod_SendMessage;

export using ::MaaDbgControllerType;
export constexpr auto _MaaDbgControllerType_None = MaaDbgControllerType_None;
export constexpr auto _MaaDbgControllerType_CarouselImage = MaaDbgControllerType_CarouselImage;
export constexpr auto _MaaDbgControllerType_ReplayRecording = MaaDbgControllerType_ReplayRecording;

export using ::MaaRect;

export using ::MaaNotificationCallback;
export using ::MaaCustomRecognitionCallback;
export using ::MaaCustomActionCallback;

// Instance/MaaContext.h

export using ::MaaContextRunTask;
export using ::MaaContextRunRecognition;
export using ::MaaContextRunAction;
export using ::MaaContextOverridePipeline;
export using ::MaaContextOverrideNext;
export using ::MaaContextGetTaskId;
export using ::MaaContextGetTasker;
export using ::MaaContextClone;

// Instance/MaaController.h

export using ::MaaCustomControllerCallbacks;
export using ::MaaAdbControllerCreate;
export using ::MaaWin32ControllerCreate;
export using ::MaaCustomControllerCreate;
export using ::MaaDbgControllerCreate;
export using ::MaaControllerDestroy;
export using ::MaaControllerSetOption;
export using ::MaaControllerPostConnection;
export using ::MaaControllerPostClick;
export using ::MaaControllerPostSwipe;
export using ::MaaControllerPostClickKey;
export using ::MaaControllerPostKeyDown;
export using ::MaaControllerPostKeyUp;
export using ::MaaControllerPostInputText;
export using ::MaaControllerPostStartApp;
export using ::MaaControllerPostStopApp;
export using ::MaaControllerPostTouchDown;
export using ::MaaControllerPostTouchMove;
export using ::MaaControllerPostTouchUp;
export using ::MaaControllerPostScreencap;
export using ::MaaControllerStatus;
export using ::MaaControllerWait;
export using ::MaaControllerConnected;
export using ::MaaControllerCachedImage;
export using ::MaaControllerGetUuid;

// Instance/MaaCustomController.h

export using ::MaaCustomControllerCallbacks;

// Instance/MaaResource.h

export using ::MaaResourceCreate;
export using ::MaaResourceDestroy;
export using ::MaaResourceRegisterCustomRecognition;
export using ::MaaResourceUnregisterCustomRecognition;
export using ::MaaResourceClearCustomRecognition;
export using ::MaaResourceRegisterCustomAction;
export using ::MaaResourceUnregisterCustomAction;
export using ::MaaResourceClearCustomAction;
export using ::MaaResourcePostBundle;
export using ::MaaResourcePostOcrModel;
export using ::MaaResourcePostPipeline;
export using ::MaaResourcePostImage;
export using ::MaaResourceOverridePipeline;
export using ::MaaResourceOverrideNext;
export using ::MaaResourceClear;
export using ::MaaResourceStatus;
export using ::MaaResourceWait;
export using ::MaaResourceLoaded;
export using ::MaaResourceSetOption;
export using ::MaaResourceGetHash;
export using ::MaaResourceGetNodeList;

// Instance/MaaTasker.h

export using ::MaaTaskerCreate;
export using ::MaaTaskerDestroy;
export using ::MaaTaskerSetOption;
export using ::MaaTaskerBindResource;
export using ::MaaTaskerBindController;
export using ::MaaTaskerInited;
export using ::MaaTaskerPostTask;
export using ::MaaTaskerStatus;
export using ::MaaTaskerWait;
export using ::MaaTaskerRunning;
export using ::MaaTaskerPostStop;
export using ::MaaTaskerStopping;
export using ::MaaTaskerGetResource;
export using ::MaaTaskerGetController;
export using ::MaaTaskerClearCache;
export using ::MaaTaskerGetRecognitionDetail;
export using ::MaaTaskerGetActionDetail;
export using ::MaaTaskerGetNodeDetail;
export using ::MaaTaskerGetTaskDetail;
export using ::MaaTaskerGetLatestNode;

// Utility/MaaBuffer.h

export using ::MaaImageRawData;
export using ::MaaImageEncodedData;

export using ::MaaStringBufferCreate;
export using ::MaaStringBufferDestroy;
export using ::MaaStringBufferIsEmpty;
export using ::MaaStringBufferClear;
export using ::MaaStringBufferGet;
export using ::MaaStringBufferSize;
export using ::MaaStringBufferSet;
export using ::MaaStringBufferSetEx;
export using ::MaaStringListBufferCreate;
export using ::MaaStringListBufferDestroy;
export using ::MaaStringListBufferIsEmpty;
export using ::MaaStringListBufferSize;
export using ::MaaStringListBufferAt;
export using ::MaaStringListBufferAppend;
export using ::MaaStringListBufferRemove;
export using ::MaaStringListBufferClear;
export using ::MaaImageBufferCreate;
export using ::MaaImageBufferDestroy;
export using ::MaaImageBufferIsEmpty;
export using ::MaaImageBufferClear;
export using ::MaaImageBufferGetRawData;
export using ::MaaImageBufferWidth;
export using ::MaaImageBufferHeight;
export using ::MaaImageBufferChannels;
export using ::MaaImageBufferType;
export using ::MaaImageBufferSetRawData;
export using ::MaaImageBufferGetEncoded;
export using ::MaaImageBufferGetEncodedSize;
export using ::MaaImageBufferSetEncoded;
export using ::MaaImageListBufferCreate;
export using ::MaaImageListBufferDestroy;
export using ::MaaImageListBufferIsEmpty;
export using ::MaaImageListBufferSize;
export using ::MaaImageListBufferAt;
export using ::MaaImageListBufferAppend;
export using ::MaaImageListBufferRemove;
export using ::MaaImageListBufferClear;
// export using ::MaaRectCreate;
// export using ::MaaRectDestroy;
// export using ::MaaRectGetX;
// export using ::MaaRectGetY;
// export using ::MaaRectGetW;
// export using ::MaaRectGetH;
// export using ::MaaRectSet;

// Utility/MaaUtility.h

export using ::MaaVersion;
export using ::MaaGlobalSetOption;
