FFmpeg supports playing media streams from pipe. Pipe is a protocol in FFmpeg. To use it, you just need to replace the file name with 'pipe:'. for example

    cat hello.avi |player pipe:

It also supports streams from file number

    pipe:number

'number' can be 0, 1, 2, .... 0 is stdin, 1 is stdout and 2 is stderr. If 'number' is empty, it means stdin.

Using pipe in AVPlayer class is almost the same. Just use "pipe:N" as file name.

#### Named Pipe

    player \\.\pipe\name


FFmpeg is powerful, do you think so?