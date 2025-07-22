- For serial storage commands
  - list should return `NOT SUPPORTED`
  - write should be in the following format
  ```stream-format
  ------BEGIN---{filepath}------
  {hex stream with crlf every 128 bytes}
  ------END---{filepath}------
```
- Add a `read hex {file}` feature to the cli
  - based on the current storage device the content of the named file should be output to the serial connection as a hex string
  - if the current storage is Serial then it should just output `NOT SUPPORTED`
