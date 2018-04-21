### gfx

gfx is a BadScript library that uses SDL to provide simple 2D rendering.

```php
use "gfx";
```

---

#### init

Initializes the gfx library

**Arguments**

- none

**Returns**

- 0 if gfx failed to initialize, 1 otherwise

---

#### create_window

Creates a new window with a given size and title

**Arguments**

1. title - String
2. width - Number
3. height - Number
4. vsync - Number

**Returns**

* 0 if gfx failed to create the window, 1 otherwise

**Example**

```swift
gfx.create_window("Hello", 800, 600, 0);
```

---

#### update

Handles events, has to be called regularly

**Arguments**

* none

**Returns**

* null

---

#### present

Updates the window with all rendering since the last call to present

**Arguments**

* none

**Returns**

* null

---

#### should_close

Returns whether or not the user has tried to close the window

**Arguments**

* none

**Returns**

* 1 if the window should be closed, 0 otherwise

---

#### clear

Clears the screen with either black or the specified color

**Arguments**

* none

or

1. red - Number
2. green - Number
3. blue - Number
4. alpha - Number

**Returns**

* null

---

#### fill_rect

Draws a filled rectangle

**Arguments**

1. x - Number
2. y - Number
3. width - Number
4. height - Number
5. red - Number
6. green - Number
7. blue - Number

**Returns**

* null

```swift
// Draw as 50x50 green square at (20,20)
gfx.fill_rect(20, 20, 50, 50, 0, 255, 0);
```

---

#### create_texture

Creates and returns a texture

**Arguments**

1. path - String

**Returns**

* Table - {width, height, data}

or if the gfx failed to create the texture

* null

**Example**

```swift
var texture = gfx.create_texture("some_image.bmp");
println("width: ", texture.width, ", height: ", texture.height);
```

---

#### draw_texture

Draws a texture to the window

**Arguments**

1. texture
2. x - Number
3. y - Number

**Returns**

* null

**Example**

```swift
gfx.draw_texture(texture, 20, 20);
```

---

