declare global {
    namespace maa {
        type DesktopHandle = string & { __brand: 'DesktopHandle' }

        type Uint64 = string
        type Id = Uint64
        type ScreencapOrInputMethods = Uint64

        type ResId = Id & { __brand: 'ResId' }
        type CtrlId = Id & { __brand: 'CtrlId' }
        type TaskId = Id & { __brand: 'TaskId' }
        type RecoId = Id & { __brand: 'RecoId' }
        type ActId = Id & { __brand: 'ActId' }
        type NodeId = Id & { __brand: 'NodeId' }
        type SinkId = Id & { __brand: 'SinkId' }

        type Status = number & { __brand: 'Status' }

        type ImageData = ArrayBuffer

        type Point = [x: number, y: number]
        type Rect = [x: number, y: number, width: number, height: number]

        type MaybePromise<T> = T | Promise<T>

        type NotifyMessage<Category extends string> =
            | `${Category}.Starting`
            | `${Category}.Succeeded`
            | `${Category}.Failed`

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

        type CustomActionCallback = CustomCallback<CustomActionSelf, boolean>
    }
}

export {}
