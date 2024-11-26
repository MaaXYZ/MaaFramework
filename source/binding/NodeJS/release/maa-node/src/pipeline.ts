type RecoType =
    | 'DirectHit'
    | 'TemplateMatch'
    | 'FeatureMatch'
    | 'ColorMatch'
    | 'OCR'
    | 'NeuralNetworkClassify'
    | 'NeuralNetworkDetect'
    | 'Custom'

type ActType =
    | 'DoNothing'
    | 'Click'
    | 'Swipe'
    | 'Key'
    | 'Text'
    | 'StartApp'
    | 'StopApp'
    | 'StopTask'
    | 'Custom'

type OrderByMap = {
    TemplateMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
    FeatureMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
    ColorMatch: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
    OCR: 'Horizontal' | 'Vertical' | 'Area' | 'Length' | 'Random'
    NeuralNetworkClassify: 'Horizontal' | 'Vertical' | 'Score' | 'Random'
    NeuralNetworkDetect: 'Horizontal' | 'Vertical' | 'Score' | 'Area' | 'Random'
}

type PipelineBuilderState<Json = {}> = {
    done: Json
} & ('recognition' extends keyof Json
    ? {}
    : {
          recognition<R extends RecoType>(
              reco: R
          ): PipelineRecognitionBuilderState<
              Json & {
                  recognition: R
              },
              R
          >
      }) &
    ('action' extends keyof Json
        ? {}
        : {
              action<A extends ActType>(
                  act: A
              ): PipelineActionBuilderState<
                  Json & {
                      action: A
                  },
                  A
              >
          }) &
    ('next' extends keyof Json
        ? {}
        : {
              next<N extends string[]>(...nxt: [...N]): PipelineBuilderState<Json & { next: N }>
          }) &
    ('interrupt' extends keyof Json
        ? {}
        : {
              interrupt<I extends string[]>(
                  ...int: [...I]
              ): PipelineBuilderState<Json & { interrupt: I }>
          }) &
    ('rate_limit' extends keyof Json
        ? {}
        : {
              rate_limit<R extends number>(rate: R): PipelineBuilderState<Json & { rate_limit: R }>
          }) &
    ('timeout' extends keyof Json
        ? {}
        : {
              timeout<R extends number>(time: R): PipelineBuilderState<Json & { timeout: R }>
          }) &
    ('on_error' extends keyof Json
        ? {}
        : {
              on_error<O extends string[]>(
                  ...err: [...O]
              ): PipelineBuilderState<Json & { on_error: O }>
          }) &
    ('inverse' extends keyof Json
        ? {}
        : {
              inverse<I extends boolean>(inv: I): PipelineBuilderState<Json & { inverse: I }>
          }) &
    ('enabled' extends keyof Json
        ? {}
        : {
              enabled<E extends boolean>(en: E): PipelineBuilderState<Json & { enabled: E }>
          }) &
    ('pre_delay' extends keyof Json
        ? {}
        : {
              pre_delay<P extends number>(pre: P): PipelineBuilderState<Json & { pre_delay: P }>
          }) &
    ('post_delay' extends keyof Json
        ? {}
        : {
              post_delay<P extends number>(post: P): PipelineBuilderState<Json & { post_delay: P }>
          }) &
    ('pre_wait_freezes' extends keyof Json
        ? {}
        : {
              pre_wait_freezes: PipelineWaitFreezeBuilderState<Json, 'pre_wait_freezes'>
          }) &
    ('post_wait_freezes' extends keyof Json
        ? {}
        : {
              post_wait_freezes: PipelineWaitFreezeBuilderState<Json, 'post_wait_freezes'>
          }) &
    ('focus' extends keyof Json
        ? {}
        : {
              focus<F extends boolean>(focus: F): PipelineBuilderState<Json & { focus: F }>
          })

type PipelineRecognitionBuilderState<PBJson, Reco extends RecoType, Json = {}> = {
    done: PipelineBuilderState<PBJson & Json>
} & (Reco extends 'DirectHit'
    ? {}
    : ('roi' extends keyof Json
          ? {}
          : {
                roi<R extends [string] | [number, number, number, number]>(
                    ...roi: R
                ): PipelineRecognitionBuilderState<
                    PBJson,
                    Reco,
                    Json & { roi: R extends [number, number, number, number] ? R : R[0] }
                >
            }) &
          ('roi_offset' extends keyof Json
              ? {}
              : {
                    roi_offset<R extends [number, number, number, number]>(
                        ...roi: R
                    ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { roi_offset: R }>
                })) &
    (Reco extends 'TemplateMatch' | 'FeatureMatch'
        ? 'template' extends keyof Json
            ? {}
            : {
                  template<T extends string[]>(
                      ...templ: [...T]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { template: T }>
              }
        : {}) &
    (Reco extends 'TemplateMatch' | 'NeuralNetworkDetect'
        ? 'threshold' extends keyof Json
            ? {}
            : {
                  threshold<T extends number[]>(
                      ...thres: [...T]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { threshold: T }>

                  threshold$<T extends number>(
                      thres: T
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { threshold: T }>
              }
        : {}) &
    (Reco extends keyof OrderByMap
        ? 'order_by' extends keyof Json
            ? {}
            : {
                  order_by<O extends OrderByMap[Reco]>(
                      order: O
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { order_by: O }>
              }
        : {}) &
    (Reco extends keyof OrderByMap
        ? 'index' extends keyof Json
            ? {}
            : {
                  index<T extends number>(
                      idx: T
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { index: T }>
              }
        : {}) &
    (Reco extends 'TemplateMatch'
        ? 'method' extends keyof Json
            ? {}
            : {
                  method<M extends 1 | 3 | 5>(
                      method: M
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { method: M }>
              }
        : {}) &
    (Reco extends 'ColorMatch'
        ? 'method' extends keyof Json
            ? {}
            : {
                  method<M extends 4 | 40 | 6>(
                      method: M
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { method: M }>
              }
        : {}) &
    (Reco extends 'TemplateMatch' | 'FeatureMatch'
        ? 'green_mask' extends keyof Json
            ? {}
            : {
                  green_mask<G extends boolean>(
                      mask: G
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { green_mask: G }>
              }
        : {}) &
    (Reco extends 'FeatureMatch' | 'ColorMatch'
        ? 'count' extends keyof Json
            ? {}
            : {
                  count<C extends number>(
                      count: C
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { count: C }>
              }
        : {}) &
    (Reco extends 'FeatureMatch'
        ? 'detector' extends keyof Json
            ? {}
            : {
                  detector<D extends 'SIFT' | 'KAZE' | 'AKAZE' | 'BRISK' | 'ORB'>(
                      det: D
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { detector: D }>
              }
        : {}) &
    (Reco extends 'FeatureMatch'
        ? 'ratio' extends keyof Json
            ? {}
            : {
                  ratio<R extends number>(
                      ratio: R
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { ratio: R }>
              }
        : {}) &
    (Reco extends 'ColorMatch'
        ? 'method' extends keyof Json
            ? ('lower' extends keyof Json
                  ? {}
                  : Json['method'] extends 4 | 40
                    ? {
                          lower<L extends [number, number, number][]>(
                              ...lower: [...L]
                          ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { lower: L }>
                      }
                    : {
                          lower<L extends [number][]>(
                              ...lower: [...L]
                          ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { lower: L }>
                      }) &
                  ('upper' extends keyof Json
                      ? {}
                      : Json['method'] extends 4 | 40
                        ? {
                              upper<L extends [number, number, number][]>(
                                  ...upper: [...L]
                              ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { upper: L }>
                          }
                        : {
                              upper<U extends [number][]>(
                                  ...upper: [...U]
                              ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { upper: U }>
                          })
            : {}
        : {}) &
    (Reco extends 'ColorMatch'
        ? 'connected' extends keyof Json
            ? {}
            : {
                  connected<C extends boolean>(
                      conn: C
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { connected: C }>
              }
        : {}) &
    (Reco extends 'OCR'
        ? 'expected' extends keyof Json
            ? {}
            : {
                  expected<E extends string[]>(
                      ...exp: [...E]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { expected: E }>
              }
        : {}) &
    (Reco extends 'NeuralNetworkClassify' | 'NeuralNetworkDetect'
        ? 'expected' extends keyof Json
            ? {}
            : {
                  expected<E extends number[]>(
                      ...exp: [...E]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { expected: E }>
              }
        : {}) &
    (Reco extends 'OCR'
        ? 'replace' extends keyof Json
            ? {}
            : {
                  replace<R extends [string, string][]>(
                      ...exp: [...R]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { replace: R }>
              }
        : {}) &
    (Reco extends 'OCR'
        ? 'only_rec' extends keyof Json
            ? {}
            : {
                  only_rec<O extends boolean>(
                      rec: O
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { only_rec: O }>
              }
        : {}) &
    (Reco extends 'OCR' | 'NeuralNetworkClassify' | 'NeuralNetworkDetect'
        ? 'model' extends keyof Json
            ? {}
            : {
                  model<M extends string>(
                      model: M
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { model: M }>
              }
        : {}) &
    (Reco extends 'NeuralNetworkClassify' | 'NeuralNetworkDetect'
        ? 'labels' extends keyof Json
            ? {}
            : {
                  labels<L extends string[]>(
                      ...label: [...L]
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { labels: L }>
              }
        : {}) &
    (Reco extends 'Custom'
        ? 'custom_recognition' extends keyof Json
            ? {}
            : {
                  custom_recognition<C extends string>(
                      reco: C
                  ): PipelineRecognitionBuilderState<PBJson, Reco, Json & { custom_recognition: C }>
              }
        : {}) &
    (Reco extends 'Custom'
        ? 'custom_recognition_param' extends keyof Json
            ? {}
            : {
                  custom_recognition_param<C extends Record<string, unknown>>(
                      param: C
                  ): PipelineRecognitionBuilderState<
                      PBJson,
                      Reco,
                      Json & { custom_recognition_param: C }
                  >
              }
        : {})

type PipelineActionBuilderState<PBJson, Act extends ActType, Json = {}> = {
    done: PipelineBuilderState<PBJson & Json>
} & (Act extends 'Click' | 'Custom'
    ? 'target' extends keyof Json
        ? {}
        : {
              target<T extends [true] | [string] | [number, number, number, number]>(
                  ...target: T
              ): PipelineActionBuilderState<
                  PBJson,
                  Act,
                  Json & { target: T extends [number, number, number, number] ? T : T[0] }
              >
          }
    : {}) &
    (Act extends 'Click' | 'Custom'
        ? 'target_offset' extends keyof Json
            ? {}
            : {
                  target_offset<O extends [number, number, number, number]>(
                      ...offset: O
                  ): PipelineActionBuilderState<PBJson, Act, Json & { target_offset: O }>
              }
        : {}) &
    (Act extends 'Swipe'
        ? 'begin' extends keyof Json
            ? {}
            : {
                  begin<B extends [true] | [string] | [number, number, number, number]>(
                      ...begin: B
                  ): PipelineActionBuilderState<
                      PBJson,
                      Act,
                      Json & { begin: B extends [number, number, number, number] ? B : B[0] }
                  >
              }
        : {}) &
    (Act extends 'Swipe'
        ? 'begin_offset' extends keyof Json
            ? {}
            : {
                  begin_offset<B extends [number, number, number, number]>(
                      ...offset: B
                  ): PipelineActionBuilderState<PBJson, Act, Json & { begin_offset: B }>
              }
        : {}) &
    (Act extends 'Swipe'
        ? 'end' extends keyof Json
            ? {}
            : {
                  end<E extends [true] | [string] | [number, number, number, number]>(
                      ...end: E
                  ): PipelineActionBuilderState<
                      PBJson,
                      Act,
                      Json & { end: E extends [number, number, number, number] ? E : E[0] }
                  >
              }
        : {}) &
    (Act extends 'Swipe'
        ? 'end_offset' extends keyof Json
            ? {}
            : {
                  end_offset<E extends [number, number, number, number]>(
                      ...offset: E
                  ): PipelineActionBuilderState<PBJson, Act, Json & { end_offset: E }>
              }
        : {}) &
    (Act extends 'Key'
        ? 'key' extends keyof Json
            ? {}
            : {
                  key<K extends number[]>(
                      ...key: [...K]
                  ): PipelineActionBuilderState<PBJson, Act, Json & { key: K }>
              }
        : {}) &
    (Act extends 'InputText'
        ? 'input_text' extends keyof Json
            ? {}
            : {
                  input_text<T extends string>(
                      text: T
                  ): PipelineActionBuilderState<PBJson, Act, Json & { input_text: T }>
              }
        : {}) &
    (Act extends 'StartApp' | 'StopApp'
        ? 'package' extends keyof Json
            ? {}
            : {
                  package<P extends string>(
                      pkg: P
                  ): PipelineActionBuilderState<PBJson, Act, Json & { package: P }>
              }
        : {}) &
    (Act extends 'Custom'
        ? 'custom_action' extends keyof Json
            ? {}
            : {
                  custom_action<C extends string>(
                      act: C
                  ): PipelineActionBuilderState<PBJson, Act, Json & { custom_action: C }>
              }
        : {}) &
    (Act extends 'Custom'
        ? 'custom_action_param' extends keyof Json
            ? {}
            : {
                  custom_action_param<C extends Record<string, unknown>>(
                      param: C
                  ): PipelineActionBuilderState<PBJson, Act, Json & { custom_action_param: C }>
              }
        : {})

type PipelineWaitFreezeBuilderState<
    PBJson,
    Key extends 'pre_wait_freezes' | 'post_wait_freezes',
    Json = {}
> = {
    done: PipelineBuilderState<
        PBJson & {
            [key in Key]: Json
        }
    >
} & ('time' extends keyof Json
    ? {}
    : {
          time<T extends number>(
              time: T
          ): PipelineWaitFreezeBuilderState<PBJson, Key, Json & { time: T }>
      }) &
    ('target' extends keyof Json
        ? {}
        : {
              target<T extends [true] | [string] | [number, number, number, number]>(
                  ...target: T
              ): PipelineWaitFreezeBuilderState<
                  PBJson,
                  Key,
                  Json & { target: T extends [number, number, number, number] ? T : T[0] }
              >
          }) &
    ('target_offset' extends keyof Json
        ? {}
        : {
              target_offset<O extends [number, number, number, number]>(
                  ...offset: O
              ): PipelineWaitFreezeBuilderState<PBJson, Key, Json & { target_offset: O }>
          }) &
    ('threshold' extends keyof Json
        ? {}
        : {
              threshold<T extends number>(
                  thres: T
              ): PipelineWaitFreezeBuilderState<PBJson, Key, Json & { threshold: T }>
          }) &
    ('method' extends keyof Json
        ? {}
        : {
              method<M extends 1 | 3 | 5>(
                  met: M
              ): PipelineWaitFreezeBuilderState<PBJson, Key, Json & { method: M }>
          }) &
    ('rate_limit' extends keyof Json
        ? {}
        : {
              rate_limit<R extends number>(
                  rate: R
              ): PipelineWaitFreezeBuilderState<PBJson, Key, Json & { rate_limit: R }>
          })

export function pp(): PipelineBuilderState {
    const self: any = new Proxy(
        {
            __json: {}
        },
        {
            get(target: { __json: any }, key: string) {
                switch (key) {
                    case 'done':
                        return target.__json
                    case 'recognition':
                        return (val: unknown) => {
                            target.__json[key] = val

                            const sub_self: any = new Proxy(
                                { __json: {} },
                                {
                                    get(sub_target: { __json: any }, sub_key: string) {
                                        switch (sub_key) {
                                            case 'done':
                                                Object.assign(target.__json, sub_target.__json)
                                                // target.__json[key] = sub_target.__json
                                                return self
                                            case 'threshold$':
                                            case 'order_by':
                                            case 'index':
                                            case 'method':
                                            case 'green_mask':
                                            case 'count':
                                            case 'detector':
                                            case 'ratio':
                                            case 'connected':
                                            case 'only_rec':
                                            case 'model':
                                            case 'custom_recognition':
                                            case 'custom_recognition_param':
                                                return (val: unknown) => {
                                                    sub_target.__json[sub_key] = val
                                                    return sub_self
                                                }
                                            case 'roi':
                                                return (...val: unknown[]) => {
                                                    sub_target.__json[sub_key] =
                                                        val.length === 4 ? val : val[0]
                                                    return sub_self
                                                }
                                            case 'roi_offset':
                                            case 'template':
                                            case 'threshold':
                                            case 'lower':
                                            case 'upper':
                                            case 'expected':
                                            case 'replace':
                                            case 'labels':
                                                return (...val: unknown[]) => {
                                                    sub_target.__json[sub_key] = val
                                                    return sub_self
                                                }
                                        }
                                        return null
                                    }
                                }
                            )
                            return sub_self
                        }
                    case 'action':
                        return (val: unknown) => {
                            target.__json[key] = val

                            const sub_self: any = new Proxy(
                                { __json: {} },
                                {
                                    get(sub_target: { __json: any }, sub_key: string) {
                                        switch (sub_key) {
                                            case 'done':
                                                Object.assign(target.__json, sub_target.__json)
                                                // target.__json[key] = sub_target.__json
                                                return self
                                            case 'key':
                                            case 'input_text':
                                            case 'package':
                                            case 'custom_action':
                                            case 'custom_action_param':
                                                return (val: unknown) => {
                                                    sub_target.__json[sub_key] = val
                                                    return sub_self
                                                }
                                            case 'target':
                                            case 'begin_target':
                                            case 'end_target':
                                                return (...val: unknown[]) => {
                                                    sub_target.__json[sub_key] =
                                                        val.length === 4 ? val : val[0]
                                                    return sub_self
                                                }
                                            case 'target_offset':
                                            case 'begin_offset':
                                            case 'end_offset':
                                                return (...val: unknown[]) => {
                                                    sub_target.__json[sub_key] = val
                                                    return sub_self
                                                }
                                        }
                                        return null
                                    }
                                }
                            )
                            return sub_self
                        }
                    case 'rate_limit':
                    case 'timeout':
                    case 'inverse':
                    case 'enabled':
                    case 'pre_delay':
                    case 'post_delay':
                    case 'focus':
                        return (val: unknown) => {
                            target.__json[key] = val
                            return self
                        }
                    case 'next':
                    case 'interrupt':
                    case 'on_error':
                        return (...val: string[]) => {
                            target.__json[key] = val
                            return self
                        }
                    case 'pre_wait_freezes':
                    case 'post_wait_freezes': {
                        const sub_self: any = new Proxy(
                            { __json: {} },
                            {
                                get(sub_target: { __json: any }, sub_key: string) {
                                    switch (sub_key) {
                                        case 'done':
                                            target.__json[key] = sub_target.__json
                                            return self
                                        case 'time':
                                        case 'threshold':
                                        case 'method':
                                        case 'rate_limit':
                                            return (val: unknown) => {
                                                sub_target.__json[sub_key] = val
                                                return sub_self
                                            }
                                        case 'target':
                                            return (...val: unknown[]) => {
                                                sub_target.__json[sub_key] =
                                                    val.length === 4 ? val : val[0]
                                                return sub_self
                                            }
                                        case 'target_offset':
                                            return (...val: unknown[]) => {
                                                sub_target.__json[sub_key] = val
                                                return sub_self
                                            }
                                    }
                                    return null
                                }
                            }
                        )
                        return sub_self
                    }
                }
                return null
            }
        }
    )
    return self
}
