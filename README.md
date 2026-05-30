# CShort

CShort is a programming language.

## Building

Currently only the parser is implemented.
The build currently produces test executables under `build/src/tests/*_run` (or `build/src/tests/Debug/*_run` on Windows multi-config generators).
To build the project, you need CMake 3.12+ and a C++ compiler installed.

```bash
cmake -B build
cmake --build build
```

## Running

## Cleaning

```bash
rm -rf build/
```

## Requirements

- C++ compiler (clang++ or g++)
- CMake 3.12+


## 🛠 Development in Dev Containers

To protect your host machine from file pollution , all build artifacts are stored in /tmp by default inside the container:
```bash
cmake -B /tmp/build
cmake --build /tmp/build
/tmp/build/hello
```

### Quick Build
Once the Dev Container is started, you can build the project using the following script:
```bash
bash .devcontainer/build_in_devcontainer.bash
```



### Planned language syntax


```rust

public int:errorGroup
fn method() {


}


public bool or AErrors,Berror,Aeeow
fn afunc(int intA, double b, float c) {
    @Attr(awf="jofwie")
    int a = 342

    @Attr(awf="jofwie")
    #let b = 224
    #let b = 3142
    let c = 314
    
    if true {
        a = 314
    }

    for i = 0; i < 10; i++ {

    }

    for i = 0
        i < 10
        i++ {

    }

    let res2 = method()

    method()
    =let myVarTranslation
    =set existingVar

    let res = try stop()

    let gg = begin() catchs(err) {
        awe =>  {
            return 35
        }
        awef => {
            ret 213
        }
    }


    let res3 = try method() else 0

    if (hasError) {
        return Error.null_access
    }

    let k = when one {
        1 => {
            ret 324
        }
        2 => {
            ret 14
        }
    }


    when abc {
        3 => 45
        else => 54
    }
    
    0
    =let aboiajw

    32 / 123 - 4321 + 5
    =set ex
    =int wow


}

```