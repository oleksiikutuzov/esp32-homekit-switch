Import("env")
from shutil import copyfile

def move_bin(*args, **kwargs):
    print("Copying bin output to project directory...")
    target = str(kwargs['target'][0])
    copyfile(target, 'esp32_switch.bin')
    print("Done.")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", move_bin)   #post action for .bin