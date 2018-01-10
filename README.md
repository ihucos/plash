[![Build Status](https://travis-ci.org/ihucos/plash-svg?branch=stable1)](https://travis-ci.org/ihucos/plash)

```
#
# Install 
#
$ pip3 install git+https://github.com/ihucos/plash.git


#
# Examples 
#

# build a simple image
$ plash-build --os ubuntu --run 'touch /file'
Container not found, trying to pull it
[0%|10%|20%|30%|40%|50%|60%|70%|80%|90%|100%]
--> touch /file
--:
a64

# second build is cached
$ plash-build --os ubuntu --run 'touch /file'
a64

# run something inside a container
$ plash-run a64 file /file
/file: empty

# layering is explicit
$ plash-build --os ubuntu --run 'touch /file' --layer --run 'touch /file2'
--> touch /file2
--:
858

# delete a container
$ plash-rm 858

# build and run in one command
# note how there is actually no building since it was already done
$ plash-run --os ubuntu -run 'touch /file' -- file /file
/file: empty







```
