## Hypothesis

The players' playing states are not changed when syncing, e.g. speed, play or pause state.

## KEY

Sending and Recieving the sync requests takes time. The delay must be computed

## Solution



### Players in 1 Process

Use AVClock. It's easy.

### Players in Network

Pass 2 value. One is aboslute time when sending the sync request. Another is the play time of the video.  
This method also works for players in 1 process