# TOOL roadmap

## where we are

- lexer ✅
- parser ✅
- type checker — in progress
- transpiler (TOOL → C) — next
- LLVM backend — later, after language is stable

---

## language features to add

- generics `<T>`
- `Result<T>` + `?` error propagation
- optional types `?`
- `defer` — run on scope exit
- closures / lambdas
- protocols / interfaces
- pattern matching
- operator overloading
- named arguments `draw(color: red, size: 10)`
- `for (N) { }` repeat loop
- range for `for (i in 0..n)`
- `??` null coalescing, `?=` conditional assign
- string interpolation `f"hello {name}"`
- switch range cases
- `do-while`, labeled loops
- nested functions
- UFCS — `length(p)` and `p.length()` are the same thing
- `comptime` — run code at compile time, bake result into binary
- SOA layout for arrays (cache friendly)
- bit manipulation `x.bits[0..7]`
- typedef / type aliases
- access modifiers on fields
- optional borrow checker (opt-in, zero ref count overhead when enabled)
- concurrency — threads, async, channels (not designed yet)

---

## memory

- decide on manual free (allow or not)
- weak keyword tooling for cycle detection
- optional ownership model (borrow checker lite)

---

## optimizations (future)

- escape analysis → stack promote objects that don't leave scope
- skip ref count when compiler proves single owner
- non-atomic ref count in single-threaded scopes
- arena allocation for types
- small structs in registers
- compile time perfect hashing for fixed-key maps
- string interning

---

## tooling

- LSP — completions, hover types, dot access suggestions
  (probably written in a different language, after compiler is done)
- formatter
- package manager (not designed yet)
- debugger integration
