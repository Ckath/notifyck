# notifyck
having broken and messed with everything so much I longer need central/builtin/freedesktop notifications working. I figured I could just make a small util that displays a notification popup without any backend, this is that.

## expecations
what you actually get by using this, realistically
- X11 tested only
- extremely minimal featureset (display timed, or untimed for ugent popup on location with colors from xresources)
- many unhandled edgecases
- minimal to no sensable config (config.h compile time only)
- a usecase only known to ck

## install
`make install`

## usage
```
usage: notifyck [-u] [-t title] text
-u	urgent, no disappearing, urgent coloring
-t	optinial title/header
```
