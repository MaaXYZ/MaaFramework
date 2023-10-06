from enum import Enum


class Status(Enum):
    invalid = 0
    pending = 1000
    running = 2000
    success = 3000
    failure = 4000
