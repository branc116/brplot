# Brplot api for julia

This is wrapper/binding of [brplot](https://github.com/branc116/brplot) for julia.

## Install

```julia
] add https://github.com/brplot/Brplot.jl
```

## Example

```julia
import Brplot

Brplot.plot.(sin.(1:0.01:3.14))
```
