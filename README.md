## Assumption

- In copy command, if we already have destination folder. In regular cp command, the source folder
  gets copied inside the destination folder. But in our implementation, the contents of the source folder get
  copied to destination folder.
- In async write we are assuming that if we are taking stdin input, we are doing a priority write even though
  normal write is used. We chose this
  approach because it becomes hard to calculate the filesize.
- The ack2 in the async write goes to the client directly without involving the nameserver.
- Delete file operation is immediate and does not respect any read/write operation already running.
