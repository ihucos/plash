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

### do


### `plash export-tar CONTAINER [FILE]`
Export the file system of a container to the given file as a compressed tar archive. If no file is supplied or the file is '-' the tar archive wil be printed to stdout instead.

### `plash import-linuxcontainers IMAGE-NAME`
Pull an image from linuxcontainers.org

### `plash import-tar TARFILE IMAGE-ID`
Create a container from a tar file.

### `plash import-url URL IMAGE_ID`
Create a container from a url pointing to a tar file.


### init

### mount

### nodepath
### purge
### rm
### run
### runfile
### runp
