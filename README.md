# ReasonML Mobile

## Requirements

- Docker

### Linux Warning

If you're using Linux you will need to setup binfmt + qemu to run ARM and ARMv8

## How to try it?

```shell
git clone git@github.com:EduardoRFS/reason-mobile.git
cd reason-mobile
./toolchain.sh
```

Now you should be inside of docker
```
cd /app/hello-reason
esy # the first time can take some time
esy start
```

## iOS

Right know it only works with Android, I want to make it works also on iOS but that's a lot of work
