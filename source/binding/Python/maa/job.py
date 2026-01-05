from .define import *


class Job:
    """异步作业句柄 / Asynchronous job handle

    用于跟踪和管理异步操作的状态，如资源加载、控制器连接等。
    Used to track and manage the status of asynchronous operations
    such as resource loading, controller connection, etc.
    """

    _job_id: MaaId

    def __init__(self, job_id: MaaId, status_func, wait_func):
        self._job_id = job_id
        self._status_func = status_func
        self._wait_func = wait_func

    @property
    def job_id(self) -> int:
        """获取作业 ID / Get job ID

        Returns:
            int: 作业 ID / Job ID
        """
        return int(self._job_id)

    def wait(self) -> "Job":
        """等待作业完成 / Wait for job completion

        阻塞当前线程直到作业完成
        Blocks the current thread until the job is done

        Returns:
            Job: 返回自身，支持链式调用 / Returns self for method chaining
        """
        self._wait_func(self._job_id)
        return self

    @property
    def status(self) -> Status:
        """获取作业状态 / Get job status

        Returns:
            Status: 作业状态 / Job status
        """
        return Status(self._status_func(self._job_id))

    @property
    def done(self) -> bool:
        """判断作业是否已完成 / Check if job is done

        Returns:
            bool: 是否已完成（成功或失败） / Whether done (succeeded or failed)
        """
        return self.status.done

    @property
    def succeeded(self) -> bool:
        """判断作业是否成功 / Check if job succeeded

        Returns:
            bool: 是否成功 / Whether succeeded
        """
        return self.status.succeeded

    @property
    def failed(self) -> bool:
        """判断作业是否失败 / Check if job failed

        Returns:
            bool: 是否失败 / Whether failed
        """
        return self.status.failed

    @property
    def pending(self) -> bool:
        """判断作业是否等待中 / Check if job is pending

        Returns:
            bool: 是否等待中 / Whether pending
        """
        return self.status.pending

    @property
    def running(self) -> bool:
        """判断作业是否运行中 / Check if job is running

        Returns:
            bool: 是否运行中 / Whether running
        """
        return self.status.running


class JobWithResult(Job):
    """带结果的异步作业句柄 / Asynchronous job handle with result

    继承自 Job，额外提供获取作业结果的功能。
    Inherits from Job, additionally provides the ability to get job result.
    """

    def __init__(self, job_id: MaaId, status_func, wait_func, get_func):
        super().__init__(job_id, status_func, wait_func)
        self._get_func = get_func

    def wait(self) -> "JobWithResult":
        """等待作业完成 / Wait for job completion

        Returns:
            JobWithResult: 返回自身，支持链式调用 / Returns self for method chaining
        """
        super().wait()
        return self

    def get(self, wait: bool = False):
        """获取作业结果 / Get job result

        Args:
            wait: 是否在获取结果前等待作业完成，默认为 False。建议先显式调用 wait()（或传入 wait=True），
                确保异步操作已完成后再获取结果 / Whether to wait for job completion before getting result,
                default is False. It's recommended to call wait() first (or pass wait=True) to ensure the
                async operation is finished before getting the result.

        Returns:
            作业执行结果，类型取决于具体作业 / Job execution result, type depends on the specific job
        """

        if wait:
            self.wait()

        return self._get_func(self._job_id)
