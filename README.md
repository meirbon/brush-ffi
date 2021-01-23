# brush-ffi

A C-interface for [glyph-brush](https://github.com/alexheretic/glyph-brush).
This library does not expose all functionality of glyph-brush but it should be enough to get one started with using glyph-brush in non-Rust languages.
An example for OpenGL is included.

## Usage
- Include header (can be found in target folder after compiling).
- Load a font using:
```C
int default_font_id = br_create_brush(filename);
```
- Load any subsequent fonts using
```C
int other_font_id = br_add_font(filename);
```
- Queue text to be drawn:
```C
TextSection section;
section.text = text.c_str();
section.screen_position_x = (float)x; // Screen position in pixels
section.screen_position_y = (float)y;
section.bounds_x = INFINITY; // For unlimited bounds, set to INFINITY
section.bounds_y = INFINITY;
section.scale_x = 24.0f; // Font size
section.scale_y = 24.0f;
section.color_r = 1.0f;
section.color_g = 1.0f;
section.color_b = 1.0f;
section.color_a = 1.0f;
section.font_id = my_font_id;
br_queue_text(section);
```
- Update vertices and texture data by calling `br_update();`
- Upload vertices and texture data in your application:
```C
uint32_t needs_update = 0; // Will be set to 1 if texture requires an update
const uint8_t *tex_data = br_get_texture_data(&needs_update);

uint32_t count = 0;
const BrushVertex *vertices = br_get_vertices(&count);

// Create vertices in your own format.
// Each vertex contains a min, max and typically you should generate 4 vertices from each BrushVertex.
// E.g. for counter-clockwise winding:
// v0 = (min_x, min_y),
// v1 = (max_x, min_y),
// v2 = (max_x, max_y),
// v3 = (min_x, max_y)
// Create 2 triangles: (v0, v1, 2), (v3, v0, v2)
```

## License
The license of this project is Apache 2.