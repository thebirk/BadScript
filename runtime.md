Only native functions have variable arguments support. This is represented here as `*args`

# Functions

## print
---
Prints all the arguments to stdout

#### Arguments
* *args

#### Returns
* null

#### Example
```lua
print("1 + 2 = ", 1+2);
```

## println
---
Same as [print](#print) but prints a newline at the end

#### Arguments
* *args

#### Returns
* null

#### Example
```lua
println("Hello, world!");
```

## type
---
Returns the type of an element as a string
#### Arguments
* v - Any value

#### Returns
* string - Representing the type

#### Example
```lua
type(2) == "number"
```

## input
---
Prints args as if they were passed to [print](#print) then read input from stdin until a newline is found or the string exceeds 4096 character.

#### Arguments
* *args

#### Returns
* string - User input

#### Example
```lua
var a = input("Input something: ");
println(a);
```

