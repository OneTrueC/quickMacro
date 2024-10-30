# NAME

**quickMacro** - a simple and fast macro program

# SYNOPSIS

**quickMacro** **-r**\|**-p** \[**-l**\] *file*

# OPTIONS

**-r, \--record**

>> Wait for `QUITKEY` to be pressed and then start recording until
   `QUITKEY` is pressed again. The time of the first `QUITKEY` press is the
   zero time offset for the macro.

**-p, \--play**

>> Play back the macro by sending all events as XTEST requests.

**-l, \--loop**

>> Play the macro until `QUITKEY` is pressed.

# IMPORTANT CONSIDERATIONS

Each event takes up 32 bytes in the macro file. Mouse events occur at a
high frequency and thus can quickly take up a lot of space.

The time between each loop of the macro is the time between starting
recording and the first event recorded.

# EXIT STATUS

**-1**

>> Unknown event type

**11**

>> X server connect failed or extension is not running

**21**

>> File is specified is not regular

**22**

>> Argument error

# CUSTOMIZATION

quickMacro can be customized by editing `QUITKEY` under
<ins>/\* config \*/</ins> near the top of the source, and recompiling.

`QUITKEY` is the keycode of the key that starts and ends macro recording.
It is set to the US right super key by default.

###### Adapted from quickMacro.1
