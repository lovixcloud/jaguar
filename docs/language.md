# Jaguar Language Documentation

Jaguar is a statically typed, native-compiled programming language designed for extreme performance and live developer reload experience.

## Variable Declarations
```jaguar
var age: num = 33;
var score: decimal = 98.71;
var isMale: bool = true;
var name: string = "Joseph";
```

## Immutable Constants
```jaguar
fixed speedOfLight: decimal = 3.0E8;
```

## Functions
```jaguar
fun add(a: num, b: num): num {
    return a + b;
}
```

## Control Flow
```jaguar
if (age >= 18) {
    live.on("Adult");
} elif (age >= 13) {
    live.on("Teenager");
} else {
    live.on("Child");
}
```
