declare global {
    namespace maa {
        type NodeName = string

        type ModeFragment = 0
        type ModeStrict = 1
        type ModeDump = 2

        type RemoveIfDump<T, Mode> = Mode extends ModeDump ? never : T
        type MaybeArray<T, Mode> = T[] | RemoveIfDump<T, Mode>
        type FixedArray<T, K extends number, A extends T[] = []> = A['length'] extends K
            ? A
            : FixedArray<T, K, [...A, T]>
        type RequiredIfStrict<T, Keys, Mode> = Mode extends ModeStrict ? RequiredKeys<T, Keys> : T
        type RequiredKeys<T, Keys> = {
            [key in keyof T & Keys]: NonNullable<T[key]>
        } & {
            [key in Exclude<keyof T, Keys>]: T[key]
        }

        type OrderByMap = {
            TemplateMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
            FeatureMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
            ColorMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
            OCR: 'Horizontal' | 'Vertical' | 'Area' | 'Length' | 'Random'
            NeuralNetworkClassify: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
            NeuralNetworkDetect: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
        }

        type RecognitionDirectHit = {}

        type RecognitionTemplateMatch<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                template?: MaybeArray<string, Mode>
                threshold?: MaybeArray<number, Mode>
                order_by?: OrderByMap['TemplateMatch']
                index?: number
                method?: 1 | 3 | 5
                green_mask?: boolean
            },
            'template',
            Mode
        >

        type RecognitionFeatureMatch<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                template?: MaybeArray<string, Mode>
                count?: number
                order_by?: OrderByMap['FeatureMatch']
                index?: number
                green_mask?: boolean
                detector?: 'SIFT' | 'KAZE' | 'AKAZE' | 'BRISK' | 'ORB'
                ratio?: number
            },
            'template',
            Mode
        >

        type RecognitionColorMatch<Mode> = {
            roi?: FlatRect | NodeName
            roi_offset?: FlatRect
        } & RequiredIfStrict<
            | {
                  method?: 4 | 40
                  lower?: MaybeArray<FixedArray<number, 3>, Mode>
                  upper?: MaybeArray<FixedArray<number, 3>, Mode>
              }
            | {
                  method: 6
                  lower?: MaybeArray<FixedArray<number, 1>, Mode>
                  upper?: MaybeArray<FixedArray<number, 1>, Mode>
              },
            'lower' | 'upper',
            Mode
        > & {
                count?: number
                order_by?: OrderByMap['ColorMatch']
                index?: number
                connected?: boolean
            }

        type RecognitionOCR<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                expected?: MaybeArray<string, Mode>
                threshold?: MaybeArray<number, Mode>
                replace?: MaybeArray<FixedArray<string, 2>, Mode>
                order_by?: OrderByMap['OCR']
                index?: number
                only_rec?: boolean
                model?: string
            },
            'expected',
            Mode
        >

        type RecognitionNeuralNetworkClassify<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                labels?: string[]
                model?: string
                expected?: MaybeArray<number, Mode>
                order_by?: OrderByMap['NeuralNetworkClassify']
                index?: number
            },
            'model',
            Mode
        >

        type RecognitionNeuralNetworkDetect<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                labels?: string[]
                model?: string
                expected?: MaybeArray<number, Mode>
                threshold?: number
                order_by?: OrderByMap['NeuralNetworkDetect']
                index?: number
            },
            'model',
            Mode
        >

        type RecognitionCustom<Mode> = RequiredIfStrict<
            {
                roi?: FlatRect | NodeName
                roi_offset?: FlatRect
                custom_recognition?: string
                custom_recognition_param?: unknown
            },
            'custom_recognition',
            Mode
        >

        type MixReco<Type extends string, Param, Mode> =
            | {
                  recognition: {
                      type: Type
                      param?: Param
                  }
              }
            | RemoveIfDump<
                  {
                      recognition: Type
                  } & Param,
                  Mode
              >

        type Recognition<Mode> =
            | RemoveIfDump<
                  {
                      recognition?: {
                          type?: never
                          param?: never
                      }
                  },
                  Mode
              >
            | MixReco<'DirectHit', RecognitionDirectHit, Mode>
            | MixReco<'TemplateMatch', RecognitionTemplateMatch<Mode>, Mode>
            | MixReco<'FeatureMatch', RecognitionFeatureMatch<Mode>, Mode>
            | MixReco<'ColorMatch', RecognitionColorMatch<Mode>, Mode>
            | MixReco<'OCR', RecognitionOCR<Mode>, Mode>
            | MixReco<'NeuralNetworkClassify', RecognitionNeuralNetworkClassify<Mode>, Mode>
            | MixReco<'NeuralNetworkDetect', RecognitionNeuralNetworkDetect<Mode>, Mode>
            | MixReco<'Custom', RecognitionCustom<Mode>, Mode>

        type ActionDoNothing = {}

        type ActionClick = {
            target?: true | NodeName | FlatRect
            target_offset?: FlatRect
            contact?: number
        }

        type ActionLongPress = {
            target?: true | NodeName | FlatRect
            target_offset?: FlatRect
            duration?: number
            contact?: number
        }

        type ActionSwipe = {
            begin?: true | NodeName | FlatRect
            begin_offset?: FlatRect
            end?: true | NodeName | FlatRect
            end_offset?: FlatRect
            duration?: number
            contact?: number
        }

        type ActionMultiSwipe<Mode> = RequiredIfStrict<
            {
                swipes?: {
                    starting?: number
                    begin?: true | NodeName | FlatRect
                    begin_offset?: FlatRect
                    end?: true | NodeName | FlatRect
                    end_offset?: FlatRect
                    duration?: number
                    contact?: number
                }[]
            },
            'swipes',
            Mode
        >

        type ActionTouch<Mode> = RequiredIfStrict<
            {
                contact?: number
                target?: true | NodeName | FlatRect
                target_offset?: FlatRect
                pressure?: number
            },
            never,
            Mode
        >

        type ActionTouchUp<Mode> = RequiredIfStrict<
            {
                contact?: number
            },
            never,
            Mode
        >

        type ActionClickKey<Mode> = RequiredIfStrict<
            {
                key?: MaybeArray<number, Mode>
            },
            'key',
            Mode
        >

        type ActionLongPressKey<Mode> = RequiredIfStrict<
            {
                key?: number
                duration?: number
            },
            'key',
            Mode
        >

        type ActionSingleKey<Mode> = RequiredIfStrict<
            {
                key?: number
            },
            'key',
            Mode
        >

        type ActionInputText<Mode> = RequiredIfStrict<
            {
                input_text?: string
            },
            'input_text',
            Mode
        >

        type ActionStartApp<Mode> = RequiredIfStrict<
            {
                package?: string
            },
            'package',
            Mode
        >

        type ActionStopApp<Mode> = RequiredIfStrict<
            {
                package?: string
            },
            'package',
            Mode
        >

        type ActionStopTask = {}

        type ActionCommand<Mode> = RequiredIfStrict<
            {
                exec?: string
                args?: string[]
                detach?: boolean
            },
            'exec',
            Mode
        >

        type ActionCustom<Mode> = RequiredIfStrict<
            {
                target?: true | NodeName | FlatRect
                target_offset?: FlatRect
                custom_action?: string
                custom_action_param?: unknown
            },
            'custom_action',
            Mode
        >

        type MixAct<Type extends string, Param, Mode> =
            | RemoveIfDump<
                  {
                      action: Type
                  } & Param,
                  Mode
              >
            | {
                  action: {
                      type: Type
                      param?: Param
                  }
              }

        type Action<Mode> =
            | RemoveIfDump<
                  {
                      action?: {
                          type?: never
                          param?: never
                      }
                  },
                  Mode
              >
            | MixAct<'DoNothing', ActionDoNothing, Mode>
            | MixAct<'Click', ActionClick, Mode>
            | MixAct<'LongPress', ActionLongPress, Mode>
            | MixAct<'Swipe', ActionSwipe, Mode>
            | MixAct<'MultiSwipe', ActionMultiSwipe<Mode>, Mode>
            | MixAct<'TouchDown', ActionTouch<Mode>, Mode>
            | MixAct<'TouchMove', ActionTouch<Mode>, Mode>
            | MixAct<'TouchUp', ActionTouchUp<Mode>, Mode>
            | MixAct<'Key', ActionClickKey<Mode>, Mode>
            | MixAct<'ClickKey', ActionClickKey<Mode>, Mode>
            | MixAct<'LongPressKey', ActionLongPressKey<Mode>, Mode>
            | MixAct<'KeyDown', ActionSingleKey<Mode>, Mode>
            | MixAct<'KeyUp', ActionSingleKey<Mode>, Mode>
            | MixAct<'InputText', ActionInputText<Mode>, Mode>
            | MixAct<'StartApp', ActionStartApp<Mode>, Mode>
            | MixAct<'StopApp', ActionStopApp<Mode>, Mode>
            | MixAct<'StopTask', ActionStopTask, Mode>
            | MixAct<'Command', ActionCommand<Mode>, Mode>
            | MixAct<'Custom', ActionCustom<Mode>, Mode>

        type WaitFreeze = {
            time?: number
            target?: true | NodeName | FlatRect
            target_offset?: FlatRect
            threshold?: number
            method?: 1 | 3 | 5
            rate_limit?: number
            timeout?: number
        }

        type General<Mode> = {
            next?: MaybeArray<NodeName, Mode>
            interrupt?: MaybeArray<NodeName, Mode>
            is_sub?: boolean
            rate_limit?: number
            timeout?: number
            on_error?: MaybeArray<string, Mode>
            inverse?: boolean
            enabled?: boolean
            pre_delay?: boolean
            post_delay?: boolean
            pre_wait_freezes?: RemoveIfDump<number, Mode> | WaitFreeze
            post_wait_freezes?: RemoveIfDump<number, Mode> | WaitFreeze
            focus?: unknown
            attach?: Record<string, unknown> // 附加 JSON 对象
        }

        type Task = Recognition<ModeFragment> & Action<ModeFragment> & General<ModeFragment>
        type StrictTask = Recognition<ModeStrict> & Action<ModeStrict> & General<ModeStrict>

        type RecursiveRequired<T> = T extends Record<string, unknown>
            ? {
                  [key in keyof T]: NonNullable<RecursiveRequired<T[key]>>
              }
            : T

        type DumpTask = RecursiveRequired<
            Recognition<ModeDump> & Action<ModeDump> & General<ModeDump>
        >
    }
}

export {}
