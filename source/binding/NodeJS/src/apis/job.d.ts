declare global {
    namespace maa {
        /**
         * 作业源接口 / Job source interface
         */
        interface JobSource<Id> {
            /**
             * 查询操作状态 / Query status of operation
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            status(id: Id): Status
            /**
             * 等待操作完成 / Wait for operation to complete
             * @param id 操作 id / Operation id
             * @returns 状态 / Status
             */
            wait(id: Id): Promise<Status>
        }

        /**
         * 作业类 / Job class
         */
        class Job<Id, Source extends JobSource<Id>, Result = void> {
            /**
             * 创建作业 / Create job
             * @param source 作业源 / Job source
             * @param id 操作 id / Operation id
             */
            constructor(source: Source, id: Id)

            /** 获取作业源 / Get job source */
            get source(): Source
            /** 获取操作 id / Get operation id */
            get id(): Id
            /** 获取状态 / Get status */
            get status(): Status
            /**
             * 等待操作完成 / Wait for operation to complete
             * @returns Promise，包含状态、完成状态、结果等 / Promise with status, done, succeeded, failed, and result
             */
            wait(): Promise<this> & {
                status: Promise<Status>
                done: Promise<boolean>
                succeeded: Promise<boolean>
                failed: Promise<boolean>
                get(): Promise<Result>
            }
            /** 获取结果 / Get result */
            get(): Result
            /** 判断是否已完成 / Check if done */
            get done(): boolean
            /** 判断是否成功 / Check if succeeded */
            get succeeded(): boolean
            /** 判断是否失败 / Check if failed */
            get failed(): boolean
            /** 判断是否正在运行 / Check if running */
            get running(): boolean
            /** 判断是否等待中 / Check if pending */
            get pending(): boolean
        }
    }
}

export {}
