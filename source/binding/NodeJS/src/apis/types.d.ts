declare module '@nekosu/maa' {
    type Uint64 = string
    type Id = Uint64
    type ScreencapOrInputMethods = Uint64

    type ResId = Id & { __brand: 'ResId' }
    type CtrlId = Id & { __brand: 'CtrlId' }
    type TaskId = Id & { __brand: 'TaskId' }
    type RecoId = Id & { __brand: 'RecoId' }
    type NodeId = Id & { __brand: 'NodeId' }

    type Status = number & { __brand: 'Status' }
    type LoggingLevel = number & { __brand: 'LoggingLevel' }
    type InferenceDevice = number & { __brand: 'InferenceDevice' }
    type InferenceExecutionProvider = number & { __brand: 'InferenceExecutionProvider' }

    type ImageData = ArrayBuffer
}
