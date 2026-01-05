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
            OCR: 'Horizontal' | 'Vertical' | 'Area' | 'Length' | 'Random' | 'Expected'
            NeuralNetworkClassify: 'Horizontal' | 'Vertical' | 'Score' | 'Random' | 'Expected'
            NeuralNetworkDetect:
                | 'Horizontal'
                | 'Vertical'
                | 'Score'
                | 'Area'
                | 'Random'
                | 'Expected'
        }

        type RecognitionDirectHit = {
            roi?: Rect | NodeName
            roi_offset?: Rect
        }

        type RecognitionTemplateMatch<Mode> = RequiredIfStrict<
            {
                roi?: Rect | NodeName
                roi_offset?: Rect
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
                roi?: Rect | NodeName
                roi_offset?: Rect
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
            roi?: Rect | NodeName
            roi_offset?: Rect
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
                roi?: Rect | NodeName
                roi_offset?: Rect
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
                roi?: Rect | NodeName
                roi_offset?: Rect
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
                roi?: Rect | NodeName
                roi_offset?: Rect
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
                roi?: Rect | NodeName
                roi_offset?: Rect
                custom_recognition?: string
                custom_recognition_param?: unknown
            },
            'custom_recognition',
            Mode
        >

        type RecognitionAnd<Mode> = RequiredIfStrict<
            {
                all_of?: Recognition<Mode>['recognition'][]
                box_index?: number
            },
            'all_of',
            Mode
        >

        type RecognitionOr<Mode> = RequiredIfStrict<
            {
                any_of: Recognition<Mode>['recognition'][]
            },
            'any_of',
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

        type RecognitionType =
            | 'DirectHit'
            | 'TemplateMatch'
            | 'FeatureMatch'
            | 'ColorMatch'
            | 'OCR'
            | 'NeuralNetworkClassify'
            | 'NeuralNetworkDetect'
            | 'And'
            | 'Or'
            | 'Custom'

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
            | MixReco<'And', RecognitionAnd<Mode>, Mode>
            | MixReco<'Or', RecognitionOr<Mode>, Mode>
            | MixReco<'Custom', RecognitionCustom<Mode>, Mode>

        type ActionDoNothing = {}

        type ActionClick = {
            target?: true | NodeName | Rect
            target_offset?: Rect
            contact?: number
        }

        type ActionLongPress = {
            target?: true | NodeName | Rect
            target_offset?: Rect
            duration?: number
            contact?: number
        }

        type ActionSwipe = {
            begin?: true | NodeName | Rect
            begin_offset?: Rect
            end?: true | NodeName | Rect | (true | NodeName | Rect)[]
            end_offset?: Rect | Rect[]
            duration?: number | number[]
            end_hold?: number | number[]
            only_hover?: boolean
            contact?: number
        }

        type ActionMultiSwipe<Mode> = RequiredIfStrict<
            {
                swipes?: {
                    starting?: number
                    begin?: true | NodeName | Rect
                    begin_offset?: Rect
                    end?: true | NodeName | Rect | (true | NodeName | Rect)[]
                    end_offset?: Rect | Rect[]
                    duration?: number | number[]
                    end_hold?: number | number[]
                    only_hover?: boolean
                    contact?: number
                }[]
            },
            'swipes',
            Mode
        >

        type ActionTouch<Mode> = RequiredIfStrict<
            {
                contact?: number
                target?: true | NodeName | Rect
                target_offset?: Rect
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

        type ActionScroll = {
            dx?: number
            dy?: number
        }

        type ActionCommand<Mode> = RequiredIfStrict<
            {
                exec?: string
                args?: string[]
                detach?: boolean
            },
            'exec',
            Mode
        >

        type ActionShell<Mode> = RequiredIfStrict<
            {
                cmd?: string
                timeout?: number
            },
            'cmd',
            Mode
        >

        type ActionCustom<Mode> = RequiredIfStrict<
            {
                target?: true | NodeName | Rect
                target_offset?: Rect
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

        type ActionType =
            | 'DoNothing'
            | 'Click'
            | 'LongPress'
            | 'Swipe'
            | 'MultiSwipe'
            | 'TouchDown'
            | 'TouchMove'
            | 'TouchUp'
            | 'Key'
            | 'ClickKey'
            | 'LongPressKey'
            | 'KeyDown'
            | 'KeyUp'
            | 'InputText'
            | 'StartApp'
            | 'StopApp'
            | 'Scroll'
            | 'StopTask'
            | 'Command'
            | 'Shell'
            | 'Custom'

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
            | MixAct<'Scroll', ActionScroll, Mode>
            | MixAct<'StopTask', ActionStopTask, Mode>
            | MixAct<'Command', ActionCommand<Mode>, Mode>
            | MixAct<'Shell', ActionShell<Mode>, Mode>
            | MixAct<'Custom', ActionCustom<Mode>, Mode>

        type NodeAttr = {
            name: string
            jump_back: boolean
            anchor: boolean
        }

        type WaitFreeze = {
            time?: number
            target?: true | NodeName | Rect
            target_offset?: Rect
            threshold?: number
            method?: 1 | 3 | 5
            rate_limit?: number
            timeout?: number
        }

        type General<Mode> = {
            next?: MaybeArray<NodeAttr, Mode>
            rate_limit?: number
            timeout?: number
            on_error?: MaybeArray<NodeAttr, Mode>
            anchor?: MaybeArray<string, Mode>
            inverse?: boolean
            enabled?: boolean
            max_hit?: number
            pre_delay?: number
            post_delay?: number
            pre_wait_freezes?: RemoveIfDump<number, Mode> | WaitFreeze
            post_wait_freezes?: RemoveIfDump<number, Mode> | WaitFreeze
            repeat?: number
            repeat_delay?: number
            repeat_wait_freezes?: RemoveIfDump<number, Mode> | WaitFreeze
            focus?: unknown
            attach?: Record<string, unknown>
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
