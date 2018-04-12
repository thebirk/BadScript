# Functions

## print(args) <a name="print"></a>
---
Prints all the arguments to stdout
#### Returns
* null
#### Example
```lua
print("1 + 2 = ", 1+2);
```

## println(args)
---
Same as [print](#print) but prints a newline at the end
#### Returns
* null
#### Example
```lua
println("Hello, world!");
```

## type(v)
---
Returns the type of an element as a string
#### Returns
* string - Representing the type
#### Example

```lua
type(2) == "number"
```

## input(args)
---
Prints args as if they were passed to print then read input from stdin until a newline is found or the string exceeds 4096 character.
#### Returns
* string - User input
#### Example
```lua
var a = input("Input something: ");
println(a);
```

