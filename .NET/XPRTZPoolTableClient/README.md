# Pool data socket server client

Known issues:
- Black ball can not be detected (almost always)
- Sometimes a ball in pocket is not detected
- Sometimes a pocket triggers multiple times or random
- When player 1 and 2 both hit the cue at the same time only one messages will be parsed. Only one hit message can be read at once.

The issues regarding pocketed balls are to be fixed with new and better sensors. The socketserver will work the same though.

If you stand near the pooltable by its long side with your back facing the temporary workspaces
the pockets are numbered as followed:

```
0-----------5-----------2
|                       |
|    x                  |
|                       |
1-----------4-----------3
```

Where x is the place where you'd normally place the white cue ball.

The socketserver sends the following messages:

### Pocket messages
```
pocket,x
```
Where x is the pocket as described in the information above.

### Hit messages
When a player hits a ball with a cue (fitted with the hit detector) you can receive the following message:
```
hit,p,x,y,z
```

Where p is the player number (1 or 2, matching to the device on the cue) and x,y,z are integer values indicating
how hard the hit was in those axis. For now it seems axis x is along the length of the cue but multiple axis need to be combined
to calculate the total hit strength.