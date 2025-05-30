| Code Name             | Numeric Value | Meaning                                                          |
| --------------------- | ------------: | ---------------------------------------------------------------- |
| `kOk`                 |             0 | Success (no error)                                               |
| `kCancelled`          |             1 | Operation was cancelled                                          |
| `kUnknown`            |             2 | Unknown error                                                    |
| `kInvalidArgument`    |             3 | Client specified an invalid argument                             |
| `kDeadlineExceeded`   |             4 | Deadline expired before operation could complete                 |
| `kNotFound`           |             5 | Some requested entity was not found                              |
| `kAlreadyExists`      |             6 | Entity that we attempted to create already exists                |
| `kPermissionDenied`   |             7 | Caller does not have permission                                  |
| `kResourceExhausted`  |             8 | Some resource has been exhausted                                 |
| `kFailedPrecondition` |             9 | Operation was rejected because system is not in a required state |
| `kAborted`            |            10 | Operation was aborted (e.g., concurrency issue)                  |
| `kOutOfRange`         |            11 | Operation was attempted past valid range                         |
| `kUnimplemented`      |            12 | Operation is not implemented or supported                        |
| `kInternal`           |            13 | Internal errors                                                  |
| `kUnavailable`        |            14 | Service is currently unavailable                                 |
| `kDataLoss`           |            15 | Unrecoverable data loss or corruption                            |
| `kUnauthenticated`    |            16 | Request does not have valid authentication credentials           |
