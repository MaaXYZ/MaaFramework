import abc
from typing import Union

from .define import MaaId, MaaStatus, MaaStatusEnum


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


class Job(abc.ABC):
    _maaid: MaaId

    def __init__(self, maaid: MaaId, status_func, wait_func):
        self._maaid = maaid
        self._status_func = status_func
        self._wait_func = wait_func

    def get_id(self) -> int:
        return int(self._maaid)

    async def wait(self) -> "Job":
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
