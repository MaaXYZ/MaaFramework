from enum import Enum
from typing import Union

from .define import MaaStatus


class StatusEnum(Enum):
    invalid = 0
    pending = 1000
    running = 2000
    success = 3000
    failure = 4000


class Status:
    _status: StatusEnum

    def __init__(self, status: Union[MaaStatus, StatusEnum]):
        self._status = StatusEnum(status)

    def done(self) -> bool:
        """
        Check if the status is done.

        :return: True if the status is done, False otherwise.
        """

        return self._status in [StatusEnum.success, StatusEnum.failure]

    def success(self) -> bool:
        """
        Check if the status is success.

        :return: True if the status is success, False otherwise.
        """

        return self._status == StatusEnum.success

    def failure(self) -> bool:
        """
        Check if the status is failure.

        :return: True if the status is failure, False otherwise.
        """

        return self._status == StatusEnum.failure

    def pending(self) -> bool:
        """
        Check if the status is pending.

        :return: True if the status is pending, False otherwise.
        """

        return self._status == StatusEnum.pending

    def running(self) -> bool:
        """
        Check if the status is running.

        :return: True if the status is running, False otherwise.
        """

        return self._status == StatusEnum.running
