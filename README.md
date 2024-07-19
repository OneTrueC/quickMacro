quickMacro
==========
A simple and fast program to record and play macros using the X window
system.

Usage
-----
`$ quickMacro MODE FILE [LOOP]`

Info
----
`QUITKEY` is the keycode of the key that starts and ends macro
recording. It is set to the US right super key by default.

When recording, it waits for `QUITKEY` to be pressed and then starts
recording until `QUITKEY` is pressed again. The time of the first
`QUITKEY` press is the zero time offset for the macro.

When playing, it plays back the macro by sending all events as XTEST
requests.

Loop repeatedly plays the macro until `QUITKEY` is pressed.

Important Considerations
------------------------
Each event takes up 32 bytes in the macro file. Mouse events occur at
a very fast frequency and thus can quickly take up space.

Because the whole macro is sent at once, there is no way to stop a
macro while its playing.

The time between each loop of the macro is the time between starting
recording and the first event recorded.

Exit Status
-----------
| Return Code |                                                Meaning |
| :---------- | -----------------------------------------------------: |
| -1          |                                     Unknown event type |
| 11          |    X server connect failed or extension is not running |
| 21          |                       File is specified is not regular |
| 22          |                                         Argument error |

Customization
-------------
**quickMacro** can be customized by editing `QUITKEY` under
<ins>/* config */</ins> and recompiling.

Dependencies
------------
- X11, XInput, and XTEST development headers

###### Adapted from quickMacro.1
