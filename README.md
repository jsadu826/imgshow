# imgshow

## Overview
`imgshow` is a Linux kernel module. It registers a character device which, together with `imgreader.c`, allows you to display an characterized image in the terminal.

## Usage
In the project directory:
```sh
gcc imgreader.c -o imgreader
make
# load module
insmod mod_imgshow.ko
# write image information to device
./imgreader [path to image] > sample
cat sample > /dev/imgshow
# display characterized image
cat /dev/imgshow
# remove module
rmmod mod_imgshow
```
