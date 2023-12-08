# How to Build
==============

Building the GDExtension requires the following:
- a Godot 4.X executable
- a C++ compiler
- python and SCons build tool
- a copy of the [godot-cpp](https://github.com/godotengine/godot-cpp) repository


Clone the matching version of the [godot-cpp](https://github.com/godotengine/godot-cpp)
repository to this directory. Replace 4.1 with the version that matches your godot version.
```
git clone -b 4.1 https://github.com/godotengine/godot-cpp

```
You can find the by running `godot --version` the output will look something like the following:
```
4.1.1.stable.official.bd6af8e0e
```
Only use the first two numbers (`4.1`) and discard the rest (`.1.stable.official.bd6af8e0e`).


Before building [godot-cpp](https://github.com/godotengine/godot-cpp) a copy of
the extension api metadata needs to be created using the following command executed in this directory:
```
godot --dump-extension-api
```

Now the GDExtension libraries can be compiled for your specific platform.
Replace `<platform>` with `windows`, `linux`, or `macos` depending on your OS.
```
cd godot-cpp
scons platform=<platform> custom_api_file=../extension_api.json
cd ..
scons platform=<platform>
```

