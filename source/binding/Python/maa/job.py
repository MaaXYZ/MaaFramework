from typing import Union

from .define import *


class Status:
    _status: MaaStatusEnum

    def __init__(self, status: Union[MaaStatus, MaaStatusEnum]):
        self._status = MaaStatusEnum(status)

    def done(self) -> bool:
        return self._status in [MaaStatusEnum.succeeded, MaaStatusEnum.failed]

    def succeeded(self) -> bool:
        return self._status == MaaStatusEnum.succeeded

    def failed(self) -> bool:
        return self._status == MaaStatusEnum.failed

    def pending(self) -> bool:
        return self._status == MaaStatusEnum.pending

    def running(self) -> bool:
        return self._status == MaaStatusEnum.running


class Job:
    job_id: MaaId

    def __init__(self, job_id: MaaId, status_func, wait_func):
        self.job_id = job_id
        self._status_func = status_func
        self._wait_func = wait_func

    def get_id(self) -> int:
        return int(self.job_id)

    def wait(self) -> "Job":
        self._wait_func(self.job_id)
        return self

    def status(self) -> Status:
        return Status(self._status_func(self.job_id))

    def done(self) -> bool:
        return self.status().done()

    def succeeded(self) -> bool:
        return self.status().succeeded()

    def failed(self) -> bool:
        return self.status().failed()

    def pending(self) -> bool:
        return self.status().pending()

    def running(self) -> bool:
        return self.status().running()


class JobWithResult(Job):
    def __init__(self, job_id: MaaId, status_func, wait_func, get_func):
        super().__init__(job_id, status_func, wait_func)
        self._get_func = get_func

    def wait(self) -> "JobWithResult":
        super().wait()
        return self

    def get(self):
        return self._get_func(self.job_id)
