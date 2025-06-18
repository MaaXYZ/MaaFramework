import type { FlatRect } from './maa'

type NodeName = string

type MaybeArray<T> = T | T[]
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

export type RecognitionTemplateMatch = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    template?: MaybeArray<string>
    template_?: MaybeArray<string> // 玛丽玛不想写, 所以多了个键
    threshold?: MaybeArray<number>
    order_by?: OrderByMap['TemplateMatch']
    index?: number
    method?: 1 | 3 | 5
    green_mask?: boolean
}

export type RecognitionFeatureMatch = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    template?: MaybeArray<string>
    template_?: MaybeArray<string>
    count?: number
    order_by?: OrderByMap['FeatureMatch']
    index?: number
    green_mask?: boolean
    detector?: 'SIFT' | 'KAZE' | 'AKAZE' | 'BRISK' | 'ORB'
    ratio?: number
}

export type RecognitionColorMatch = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
} & (
    | {
          method?: 4 | 40
          lower?: MaybeArray<FixedArray<number, 3>>
          upper?: MaybeArray<FixedArray<number, 3>>
      }
    | {
          method: 6
          lower?: MaybeArray<FixedArray<number, 1>>
          upper?: MaybeArray<FixedArray<number, 1>>
      }
) & {
        count?: number
        order_by?: OrderByMap['ColorMatch']
        index?: number
        connected?: boolean
    }

export type RecognitionOCR = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    expected?: MaybeArray<string>
    threshold?: MaybeArray<number>
    replace?: MaybeArray<FixedArray<string, 2>>
    order_by?: OrderByMap['OCR']
    index?: number
    only_rec?: boolean
    model?: string
}

export type RecognitionNeuralNetworkClassify = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    labels?: string[]
    model?: string
    expected?: MaybeArray<number>
    order_by?: OrderByMap['NeuralNetworkClassify']
    index?: number
}

export type RecognitionNeuralNetworkDetect = {
    roi?: FlatRect | NodeName
    roi_offset?: FlatRect
    labels?: string[]
    model?: string
    expected?: MaybeArray<number>
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

type MixReco<Type extends string, Param> =
    | ({
          recognition: Type
      } & Param)
    | {
          recognition: {
              type: Type
              param?: Param
          }
      }

export type Recognition =
    | {
          recognition?: {}
      }
    | MixReco<'DirectHit', RecognitionDirectHit>
    | MixReco<'TemplateMatch', RecognitionTemplateMatch>
    | MixReco<'FeatureMatch', RecognitionFeatureMatch>
    | MixReco<'ColorMatch', RecognitionColorMatch>
    | MixReco<'OCR', RecognitionOCR>
    | MixReco<'NeuralNetworkClassify', RecognitionNeuralNetworkClassify>
    | MixReco<'NeuralNetworkDetect', RecognitionNeuralNetworkDetect>
    | MixReco<'Custom', RecognitionCustom>

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

export type ActionKey = {
    key?: MaybeArray<number>
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

type MixAct<Type extends string, Param> =
    | ({
          action: Type
      } & Param)
    | {
          action: {
              type: Type
              param?: Param
          }
      }

export type Action =
    | {
          action?: {}
      }
    | MixAct<'DoNothing', ActionDoNothing>
    | MixAct<'Click', ActionClick>
    | MixAct<'LongPress', ActionLongPress>
    | MixAct<'Swipe', ActionSwipe>
    | MixAct<'MultiSwipe', ActionMultiSwipe>
    | MixAct<'Key', ActionKey>
    | MixAct<'InputText', ActionInputText>
    | MixAct<'StartApp', ActionStartApp>
    | MixAct<'StopApp', ActionStopApp>
    | MixAct<'StopTask', ActionStopTask>
    | MixAct<'Command', ActionCommand>
    | MixAct<'Custom', ActionCustom>

export type WaitFreeze = {
    time?: number
    target?: true | NodeName | FlatRect
    target_offset?: FlatRect
    threshold?: number
    method?: 1 | 3 | 5
    rate_limit?: number
    timeout?: number
}

export type General = {
    next?: MaybeArray<NodeName>
    interrupt?: MaybeArray<NodeName>
    is_sub?: boolean
    rate_limit?: number
    timeout?: number
    on_error?: MaybeArray<string>
    inverse?: boolean
    enabled?: boolean
    pre_delay?: boolean
    post_delay?: boolean
    pre_wait_freezes?: number | WaitFreeze
    post_wait_freezes?: number | WaitFreeze
    focus?: unknown
}

export type Task = Recognition & Action & General
