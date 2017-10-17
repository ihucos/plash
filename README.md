plash

### `plash add-layer CONTAINER`
Reads a shell script from stdin and returns a builded or cached container on top of the supplied container.

The new container is the only thing printed to stdout, building information and status messages are printed to stderr.

For must cases you would, use `plash build` for a higher level interface.

exit status:
same as `plash build`


### `plash build <ACTIONS>`
Builds a container. All command line options are interpreted as action calls from the `stdlib`. See insertdoclinkhere for available actions

Some short examples:
`plash build -o zesty --run 'touch a'`
`plash build -o zesty --run 'touch a' --layer --run 'touch b'`
`plash build -o zesty --apt nmap`

exit status:
1: Generic error code
2: Unrecognized arguments
3: Returning container from cache, not building
4: Build error - building returned non-zero exit status


### `plash getscript <ACTIONS>`
Prints the shell script generated from actions passed as command line parameters.
See insertdoclinkhere for available actions

exit status:
1: error


### do


### `export-tar export-tar CONTAINER [FILE]`
Export 


### import-linuxcontainers
### import-tar
### import-url
### init
### mount
### nodepath
### purge
### rm
### run
### runfile
### runp
