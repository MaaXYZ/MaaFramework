declare global {
    namespace maa {
        type Uint64 = string
        type Id = Uint64
        type ScreencapOrInputMethods = Uint64

        type ResId = Id & { __brand: 'ResId' }
        type CtrlId = Id & { __brand: 'CtrlId' }
        type TaskId = Id & { __brand: 'TaskId' }
        type RecoId = Id & { __brand: 'RecoId' }
        type NodeId = Id & { __brand: 'NodeId' }
        type SinkId = Id & { __brand: 'SinkId' }

        type Status = number & { __brand: 'Status' }
        type LoggingLevel = number & { __brand: 'LoggingLevel' }
        type InferenceDevice = number & { __brand: 'InferenceDevice' }
        type InferenceExecutionProvider = number & { __brand: 'InferenceExecutionProvider' }

        type ImageData = ArrayBuffer

        type Rect = {
            x: number
            y: number
            width: number
            height: number
        }

        type FlatRect = [x: number, y: number, width: number, height: number]

        type MaybePromise<T> = T | Promise<T>

        interface CustomRecognitionSelf {
            context: Context
            id: TaskId
            task: string
            name: string
            param: unknown
            image: ImageData
            roi: Rect
        }

        interface CustomActionSelf {
            context: Context
            id: TaskId
            task: string
            name: string
            param: unknown
            recoId: RecoId
            box: Rect
        }

        type CustomCallback<Self, Ret> = (this: Self, self: Self) => MaybePromise<Ret>

        type CustomRecognitionCallback = CustomCallback<
            CustomRecognitionSelf,
            [out_box: Rect, out_detail: string] | null
        >

        export type CustomActionCallback = CustomCallback<CustomActionSelf, boolean>
    }
}

export {}
