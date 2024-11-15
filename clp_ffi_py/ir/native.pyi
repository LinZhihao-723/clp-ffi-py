from __future__ import annotations

from datetime import tzinfo
from types import TracebackType
from typing import Any, Dict, IO, List, Optional, Type

from clp_ffi_py.wildcard_query import WildcardQuery

class DeserializerBuffer:
    def __init__(self, input_stream: IO[bytes], initial_buffer_capacity: int = 4096): ...
    def get_num_deserialized_log_messages(self) -> int: ...
    def _test_streaming(self, seed: int) -> bytearray: ...

class Metadata:
    def __init__(self, ref_timestamp: int, timestamp_format: str, timezone_id: str): ...
    def is_using_four_byte_encoding(self) -> bool: ...
    def get_ref_timestamp(self) -> int: ...
    def get_timestamp_format(self) -> str: ...
    def get_timezone_id(self) -> str: ...
    def get_timezone(self) -> tzinfo: ...

class LogEvent:
    def __init__(
        self,
        log_message: str,
        timestamp: int,
        index: int = 0,
        metadata: Optional[Metadata] = None,
    ): ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __getstate__(self) -> Dict[str, Any]: ...
    def __setstate__(self, state: Dict[str, Any]) -> None: ...
    def get_log_message(self) -> str: ...
    def get_timestamp(self) -> int: ...
    def get_index(self) -> int: ...
    def get_formatted_message(self, timezone: Optional[tzinfo] = None) -> str: ...
    def match_query(self, query: Query) -> bool: ...

class Query:
    @staticmethod
    def default_search_time_lower_bound() -> int: ...
    @staticmethod
    def default_search_time_upper_bound() -> int: ...
    @staticmethod
    def default_search_time_termination_margin() -> int: ...
    def __init__(
        self,
        search_time_lower_bound: int = default_search_time_lower_bound(),
        search_time_upper_bound: int = default_search_time_upper_bound(),
        wildcard_queries: Optional[List[WildcardQuery]] = None,
        search_time_termination_margin: int = default_search_time_termination_margin(),
    ): ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __getstate__(self) -> Dict[str, Any]: ...
    def __setstate__(self, state: Dict[str, Any]) -> None: ...
    def get_search_time_lower_bound(self) -> int: ...
    def get_search_time_upper_bound(self) -> int: ...
    def get_search_time_termination_margin(self) -> int: ...
    def get_wildcard_queries(self) -> Optional[List[WildcardQuery]]: ...
    def match_log_event(self, log_event: LogEvent) -> bool: ...

class FourByteSerializer:
    @staticmethod
    def serialize_preamble(
        ref_timestamp: int, timestamp_format: str, timezone: str
    ) -> bytearray: ...
    @staticmethod
    def serialize_message_and_timestamp_delta(timestamp_delta: int, msg: bytes) -> bytearray: ...
    @staticmethod
    def serialize_message(msg: bytes) -> bytearray: ...
    @staticmethod
    def serialize_timestamp_delta(timestamp_delta: int) -> bytearray: ...
    @staticmethod
    def serialize_end_of_ir() -> bytearray: ...

class FourByteDeserializer:
    @staticmethod
    def deserialize_preamble(decoder_buffer: DeserializerBuffer) -> Metadata: ...
    @staticmethod
    def deserialize_next_log_event(
        deserializer_buffer: DeserializerBuffer,
        query: Optional[Query] = None,
        allow_incomplete_stream: bool = False,
    ) -> Optional[LogEvent]: ...

class KeyValuePairLogEvent:
    def __init__(self, dictionary: Dict[Any, Any]): ...
    def to_dict(self) -> Dict[Any, Any]: ...

class Serializer:
    def __init__(self, output_stream: IO[bytes], buffer_size_limit: int = 65536): ...
    def __enter__(self) -> Serializer: ...
    def __exit__(
        self,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[BaseException],
        traceback: Optional[TracebackType],
    ) -> None: ...
    def serialize_msgpack_map(self, msgpack_map: bytes) -> int: ...
    def get_num_bytes_serialized(self) -> int: ...
    def flush(self) -> None: ...
    def close(self) -> None: ...

class Deserializer:
    def __init__(
        self,
        input_stream: IO[bytes],
        buffer_capacity: int = 65536,
        allow_incomplete_stream: bool = False,
    ): ...
    def deserialize_log_event(self) -> Optional[KeyValuePairLogEvent]: ...

class IncompleteStreamError(Exception): ...
