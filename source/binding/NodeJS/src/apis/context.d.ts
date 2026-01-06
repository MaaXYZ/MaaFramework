declare global {
    namespace maa {
        class Context {
            constructor(handle?: string)

            run_task(
                entry: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<TaskDetail | null>
            run_recognition(
                entry: string,
                image: ImageData,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<RecoDetail | null>
            run_action(
                entry: string,
                box: Rect,
                reco_detail: string,
                pipeline_override?: Record<string, unknown> | Record<string, unknown>[],
            ): Promise<NodeDetail | null>
            override_pipeline(
                pipeline_override: Record<string, unknown> | Record<string, unknown>[],
            ): void
            override_next(node_name: string, next: string[]): void
            override_image(image_name: string, image: ImageData): void
            get_node_data(node_name: string): string | null
            get_node_data_parsed(node_name: string): DumpTask | null
            get task_id(): TaskId
            get tasker(): Tasker
            clone(): Context
            set_anchor(anchor_name: string, node_name: string): void
            get_anchor(anchor_name: string): string | null
            get_hit_count(node_name: string): number
            clear_hit_count(node_name: string): void
        }
    }
}

export {}
