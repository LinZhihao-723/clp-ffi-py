from datetime import tzinfo
from typing import Any, Dict, Optional

class Metadata:
    timezone: tzinfo

    def __init__(self, ref_timestamp: int, timestamp_format: str, timezone_id: str): ...
    def is_using_four_byte_encoding(self) -> bool: ...
    def get_ref_timestamp(self) -> int: ...
    def get_timestamp_format(self) -> str: ...
    def get_timezone_id(self) -> str: ...

class Message:
    def __init__(
        self,
        message: str,
        timestamp: int,
        message_idx: int = 0,
        metadata: Optional[Metadata] = None,
    ): ...
    def __str__(self) -> str: ...
    def __repr__(self) -> Dict[Any]: ...
    def __getstate__(self) -> Dict[Any]: ...
    def __setstate__(self, state: Dict[Any]): ...
    def get_message(self) -> str: ...
    def get_timestamp(self) -> int: ...
    def get_message_idx(self) -> int: ...
    def get_raw_message(self, timezone: Optional[tzinfo] = None) -> str: ...
