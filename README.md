Hailo Playground
===

## Building

Builds the following binaries:
1. detect
2. classify

### Dependencies
OpenCV

HailoRT

### Binaries
CMake is the metabuild system used.

Debug builds are default

```
git clone https://github.com/iancohee/hailo_playground.git
cd hailo_playground
mkdir build
cd build
cmake ..
make
```

Release builds can be specified

```
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

## detect: Usage
Example help text

```bash
Usage: detect [params] device 

        -?, -h, --help (value:true)
                print this message
        -e, --email
                email account for SMTP authentication and "MAIL FROM:"
        --hef, -m, --model (value:yolov8n.hef)
                path of the model to load in HEF format. Only yolov8n.hef has been tested
        -s, --smtp (value:smtp://smtp.gmail.com:587)
                SMTP server address
        -t, --to
                "RCPT:" field for sending email

        device (value:auto)
                video device to open. Can be IP address or device path
```

### A Notes on Gmail Application Passwords
So yeah, this is still a thing, and they are kind of a security hole.

**Do not leak your gmail application password** because it provides full access to your email account (no scopes/roles to limit permissions), and it completely circumvents Oauth and MFA. **Use Gmail application passwords at your own risk.**

### Example

Runs with video capture device set to "auto" which is the first device found, usually a USB camera.

```bash
SMTP_PASS="abc 123 def 456" ./bin/Debug/detect --hef=yolov8n.hef --email=e@mail.com --to=me@mail.com 
```

We can also specify a stream

```bash
SMTP_PASS="abc 124 def 456" ./bin/Debug/detect --hef=yolov8n.hef --email=e@mail.com --to=me@mail.com http://127.0.0.1:4747/mjpeg?640x420
```

or (possibly) a device path

```bash
SMTP_PASS="abc 124 def 456" ./bin/Debug/detect --hef=yolov8n.hef --email=e@mail.com --to=me@mail.com /dev/usbcamera
```