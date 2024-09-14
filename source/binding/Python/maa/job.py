from typing import Union

from .define import *


class Status:
    _status: MaaStatusEnum

    def __init__(self, status: Union[MaaStatus, MaaStatusEnum]):
        self._status = MaaStatusEnum(status)

    def done(self) -> bool:
        return self._status in [MaaStatusEnum.success, MaaStatusEnum.failure]

    def success(self) -> bool:
        return self._status == MaaStatusEnum.success

    def failure(self) -> bool:
        return self._status == MaaStatusEnum.failure

    def pending(self) -> bool:
        return self._status == MaaStatusEnum.pending

    def running(self) -> bool:
        return self._status == MaaStatusEnum.running


class Job:
    _maaid: MaaId

    def __init__(self, maaid: MaaId, status_func, wait_func):
        self._maaid = maaid
        self._status_func = status_func
        self._wait_func = wait_func

    def get_id(self) -> int:
        return int(self._maaid)

    def wait(self) -> "Job":
        self._wait_func(self._maaid)
        return self

    def status(self) -> Status:
        return Status(self._status_func(self._maaid))

    def done(self) -> bool:
        return self.status().done()

    def success(self) -> bool:
        return self.status().success()

    def failure(self) -> bool:
        return self.status().failure()

    def pending(self) -> bool:
        return self.status().pending()

    def running(self) -> bool:
        return self.status().running()


class JobWithResult(Job):
    def __init__(self, maaid: MaaId, status_func, wait_func, get_func):
        super().__init__(maaid, status_func, wait_func)
        self._get_func = get_func

    def wait(self) -> "JobWithResult":
        super().wait()
        return self

    def get(self):
        return self._get_func(self._maaid)
