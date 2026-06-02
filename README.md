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
```

### Quick Build
Once the Dev Container is started, you can build the project using the following script:
```bash
bash .devcontainer/build_in_devcontainer.bash
```



## Language Specification

### Classes

A class is declared with the `class` keyword, followed by an identifier and a body enclosed in `{ }`.

```
class ClassName {
    // class body
}
```

- Class names are identifiers (letters, digits, underscores).
- The body may contain nested declarations (more classes, future constructs, etc.).
- The opening `{` and closing `}` are required.

### Functions

A function is declared with the `fn` keyword, followed by an identifier, parameters in parentheses, and a body enclosed in `{ }`.
Functions can be declared at the top level of a file or inside a class.

```
fn functionName(int a, bool b) {
    return false
}
```

- Parameters are written as `type name` pairs separated by commas.
- The body may contain statements such as declarations, assignments, and `return`.

### Comments

CShort has three kinds of comments.

#### Line comments

Start with `//` and extend to the end of the line.

```
// this is a line comment
```

#### Block comments (non-named)

Start with `/*` and end at the first `*/`.

```
/* this is a
   multi-line block comment */
```

- Nested block comments are **not** supported.

#### Named block comments

Start with `/*<[tag]` (where `tag` is any label) and end only at the matching `[tag]>*/`.

```
/*<[outer]
    This text is commented out.
    /*<[inner]
        This part is also commented out.
    [inner]>*/
    Back in the outer comment.
[outer]>*/
```

Rules:
- The opening delimiter is `/*<[tag]`; `<[` immediately follows `/*`.
- The tag name is everything between `[` and `]` in the opening delimiter.
- The comment is closed **only** by `[tag]>*/` using the **same** tag name; a bare `*/` inside a named block comment does **not** close it.
- Named block comments can be nested inside one another (each using a different tag name), allowing a form of comment nesting that ordinary `/* */` does not support.
- A non-named block comment (`/* ... */`) can wrap named block comments, because `[tag]>*/` is **not** treated as a closing `*/` for non-named block comments — only a bare `*/` (not preceded by `]>`) closes them.

### Numbers

Numeric literals are written as a sequence of digits, optionally preceded by a negative sign.

```
42
-100
1234L
-9876L
```

Rules:
- A number is a sequence of one or more digits (0-9).
- Numbers can be preceded by a negative sign (`-`).
- Numbers can optionally end with the suffix `L` to denote a 64-bit integer; without this suffix, numbers are 32-bit integers.
- The suffix `L` is case-sensitive; only uppercase `L` is supported.
- Numbers must be followed by a space, operator, closing parenthesis, or other terminable character; they cannot be followed by arbitrary characters.

Examples:
- `int x = 42` — a 32-bit integer literal
- `long y = 100L` — a 64-bit integer literal
- `int z = -21` — a negative 32-bit integer literal
- `long w = -500L` — a negative 64-bit integer literal

---

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