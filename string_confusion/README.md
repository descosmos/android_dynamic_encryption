

## Usage
```shell
# build the project
mkdir build 
cd build
../make.sh

# Run the project
adb push ./build/main /data/local/tmp
adb push ./build/minidl_encrypt /data/local/tmp
adb push ./build/libcaculator.so /data/local/tmp
adb shell chmod 777 main
adb shell chmod 777 minidl_encrypt

adb shell
> phone: cd /data/local/tmp
> phone: ./minidl_encrypt libcaculator.so
> phone: ./main ./libcaculator.ss.so

```