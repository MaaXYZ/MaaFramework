import { Context } from './context'
import maa from './maa'

export interface CustomRecognizerSelf {
    context: Context
    id: maa.TaskId
    task: string
    name: string
    param: unknown
    image: maa.ImageData
    roi: maa.Rect
}

export interface CustomActionSelf {
    context: Context
    id: maa.TaskId
    task: string
    name: string
    param: unknown
    recoId: maa.RecoId
    box: maa.Rect
}

type CustomCallback<Self, Ret> = (this: Self, self: Self) => maa.MaybePromise<Ret>

export type CustomRecognizerCallback = CustomCallback<
    CustomRecognizerSelf,
    [out_box: maa.Rect, out_detail: string] | null
>

export type CustomActionCallback = CustomCallback<CustomActionSelf, boolean>
