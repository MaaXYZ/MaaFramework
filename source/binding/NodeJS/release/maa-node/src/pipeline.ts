import type { FlatRect } from './maa'

type NodeName = string

type OutputRemove<T, Output> = Output extends true ? never : T
type MaybeArray<T, Output> = T[] | OutputRemove<T, Output>
type FixedArray<T, K extends number, A extends T[] = []> = A['length'] extends K
    ? A
    : FixedArray<T, K, [...A, T]>

type OrderByMap = {
    TemplateMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
    FeatureMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
    ColorMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
    OCR: 'Horizontal' | 'Vertical' | 'Area' | 'Length' | 'Random'
    NeuralNetworkClassify: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
    NeuralNetworkDetect: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
}

export type RecognitionDirectHit = {}

export type RecognitionTemplateMatch<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    template?: MaybeArray<string, Output>
    template_?: MaybeArray<string, Output> // 玛丽玛不想写, 所以多了个键
    threshold?: MaybeArray<number, Output>
    order_by?: OrderByMap['TemplateMatch']
    index?: number
    method?: 1 | 3 | 5
    green_mask?: boolean
}

export type RecognitionFeatureMatch<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    template?: MaybeArray<string, Output>
    template_?: MaybeArray<string, Output>
    count?: number
    order_by?: OrderByMap['FeatureMatch']
    index?: number
    green_mask?: boolean
    detector?: 'SIFT' | 'KAZE' | 'AKAZE' | 'BRISK' | 'ORB'
    ratio?: number
}

export type RecognitionColorMatch<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
} & (
    | {
          method?: 4 | 40
          lower?: MaybeArray<FixedArray<number, 3>, Output>
          upper?: MaybeArray<FixedArray<number, 3>, Output>
      }
    | {
          method: 6
          lower?: MaybeArray<FixedArray<number, 1>, Output>
          upper?: MaybeArray<FixedArray<number, 1>, Output>
      }
) & {
        count?: number
        order_by?: OrderByMap['ColorMatch']
        index?: number
        connected?: boolean
    }

export type RecognitionOCR<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    expected?: MaybeArray<string, Output>
    threshold?: MaybeArray<number, Output>
    replace?: MaybeArray<FixedArray<string, 2>, Output>
    order_by?: OrderByMap['OCR']
    index?: number
    only_rec?: boolean
    model?: string
}

export type RecognitionNeuralNetworkClassify<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    labels?: string[]
    model?: string
    expected?: MaybeArray<number, Output>
    order_by?: OrderByMap['NeuralNetworkClassify']
    index?: number
}

export type RecognitionNeuralNetworkDetect<Output> = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    labels?: string[]
    model?: string
    expected?: MaybeArray<number, Output>
    threshold?: number
    order_by?: OrderByMap['NeuralNetworkDetect']
    index?: number
}

export type RecognitionCustom = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    custom_recognition?: string
    custom_recognition_param?: unknown
}

type MixReco<Type extends string, Param, Output> =
    | {
          recognition: {
              type: Type
              param?: Param
          }
      }
    | OutputRemove<
          {
              recognition: Type
          } & Param,
          Output
      >

export type Recognition<Output> =
    | OutputRemove<
          {
              recognition?: {
                  type?: never
                  param?: never
              }
          },
          Output
      >
    | MixReco<'DirectHit', RecognitionDirectHit, Output>
    | MixReco<'TemplateMatch', RecognitionTemplateMatch<Output>, Output>
    | MixReco<'FeatureMatch', RecognitionFeatureMatch<Output>, Output>
    | MixReco<'ColorMatch', RecognitionColorMatch<Output>, Output>
    | MixReco<'OCR', RecognitionOCR<Output>, Output>
    | MixReco<'NeuralNetworkClassify', RecognitionNeuralNetworkClassify<Output>, Output>
    | MixReco<'NeuralNetworkDetect', RecognitionNeuralNetworkDetect<Output>, Output>
    | MixReco<'Custom', RecognitionCustom, Output>

export type ActionDoNothing = {}

export type ActionClick = {
    target?: true | NodeName | FlatRect
    target_offset?: FlatRect
}

export type ActionLongPress = {
    target?: true | NodeName | FlatRect
    target_offset?: FlatRect
    duration?: number
}

export type ActionSwipe = {
    begin?: true | NodeName | FlatRect
    begin_offset?: FlatRect
    end?: true | NodeName | FlatRect
    end_offset?: FlatRect
    duration?: number
}

export type ActionMultiSwipe = {
    swipes?: {
        starting?: number
        begin?: true | NodeName | FlatRect
        begin_offset?: FlatRect
        end?: true | NodeName | FlatRect
        end_offset?: FlatRect
        duration?: number
    }[]
}

export type ActionKey<Output> = {
    key?: MaybeArray<number, Output>
}

export type ActionInputText = {
    input_text?: string
}

export type ActionStartApp = {
    package?: string
}

export type ActionStopApp = {
    package?: string
}

export type ActionStopTask = {}

export type ActionCommand = {
    exec?: string
    args?: string[]
    detach?: boolean
}

export type ActionCustom = {
    target?: true | NodeName | FlatRect
    target_offset?: FlatRect
    custom_action?: string
    custom_action_param?: unknown
}

type MixAct<Type extends string, Param, Output> =
    | OutputRemove<
          {
              action: Type
          } & Param,
          Output
      >
    | {
          action: {
              type: Type
              param?: Param
          }
      }

export type Action<Output> =
    | OutputRemove<
          {
              action?: {
                  type?: never
                  param?: never
              }
          },
          Output
      >
    | MixAct<'DoNothing', ActionDoNothing, Output>
    | MixAct<'Click', ActionClick, Output>
    | MixAct<'LongPress', ActionLongPress, Output>
    | MixAct<'Swipe', ActionSwipe, Output>
    | MixAct<'MultiSwipe', ActionMultiSwipe, Output>
    | MixAct<'Key', ActionKey<Output>, Output>
    | MixAct<'InputText', ActionInputText, Output>
    | MixAct<'StartApp', ActionStartApp, Output>
    | MixAct<'StopApp', ActionStopApp, Output>
    | MixAct<'StopTask', ActionStopTask, Output>
    | MixAct<'Command', ActionCommand, Output>
    | MixAct<'Custom', ActionCustom, Output>

export type WaitFreeze = {
    time?: number
    target?: true | NodeName | FlatRect
    target_offset?: FlatRect
    threshold?: number
    method?: 1 | 3 | 5
    rate_limit?: number
    timeout?: number
}

export type General<Output> = {
    next?: MaybeArray<NodeName, Output>
    interrupt?: MaybeArray<NodeName, Output>
    is_sub?: boolean
    rate_limit?: number
    timeout?: number
    on_error?: MaybeArray<string, Output>
    inverse?: boolean
    enabled?: boolean
    pre_delay?: boolean
    post_delay?: boolean
    pre_wait_freezes?: OutputRemove<number, Output> | WaitFreeze
    post_wait_freezes?: OutputRemove<number, Output> | WaitFreeze
    focus?: unknown
}

export type Task = Recognition<false> & Action<false> & General<false>

type RecursiveRequired<T> =
    T extends Record<string, unknown>
        ? {
              [key in keyof T]-?: RecursiveRequired<T[key]>
          }
        : T

export type DumpTask = RecursiveRequired<Recognition<true> & Action<true> & General<true>>
