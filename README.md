# BadScript - General Documentation

BadScript is a dynamically typed scripting language. It is backed by a mark-and-sweep GC.

One major difference between BadScript and other scripting languages is that BS requires an entry point like most conventional programming languages.



**VERY W.I.P**



### Types

---

#### Null

The null type represents and empty value

#### Number

Represented by a double precision floating point number

#### String

A simple string type, immutable

#### Table

The main data structure of BadScript. Combines hash tables and arrays.

#### Userdata

A type designed to hold data not representable in BadScript. Only used by native functions.



### Method calls

---

Method calls are made using the `:`operator.

```swift
func make_point(x, y) {
    return {x = x, y = y, set = point_set, tostring = point_tostring};
}

func point_set(p, x, y) {
    p.x = x;
    p.y = y;
}

func point_tostring(p) {
    return format("(", p.x, ", ", p.y, ")");
}

func main(args) {
    var p = make_point(1, 2);
    println("Before: ", p:tostring());
    p:set(3, 5);
    println("After: ", p:tostring());
}
```

The operator expects the left to evaluate to a table and the right to evaluate to a function within the table. The function is then called with the table passed as the first argument.