### Projectors

  | Manufacturer | Model  | native resolution | throw ratio (distance:width) | distance for 40" |
  |--------------|--------|-------------------|------------------------------|------------------|
  | LG           | HS200G | 800x600           | 1.49:1                       | 1.31m            |
  | Asus         | P3E    | 1280x800          | 0.80:1                       | 0.71m            |
  | Philips      | NPX640 | 1920x1080         | 1.31:1                       | 1.17m            |

### random notes

  * camera USB bandwidth allocation is a problem
    * needs uvcvideo quirks parameter (see `uvcvideo.conf`) and limited FPS
    * check with: `$ cat /sys/kernel/debug/usb/devices | grep "B: "`
  * inconsistent camera device naming is fixed by `99-camera-symlink.rules`
  * default SUR40 table display size: 89x50cm
