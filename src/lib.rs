use glyph_brush::{
    ab_glyph::{point, FontArc, PxScale},
    BrushAction, BrushError, FontId, GlyphBrush, GlyphBrushBuilder,
};

pub use glyph_brush::{Section, Text};

use std::{ffi::CStr, os::raw::c_char};
use std::{fs::File, sync::RwLock};

#[repr(C)]
pub struct TextSection {
    text: *const c_char,
    screen_position_x: f32,
    screen_position_y: f32,
    bounds_x: f32, // Default is inf
    bounds_y: f32, // Default is inf
    scale_x: f32,
    scale_y: f32,
    color_r: f32,
    color_g: f32,
    color_b: f32,
    color_a: f32,
    font_id: u32,
}

#[derive(Debug)]
struct Brush {
    brush: GlyphBrush<BrushVertex>,
    tex_width: u32,
    tex_height: u32,
    tex_data: Vec<u8>,
    vertices: Vec<BrushVertex>,
    tex_changed: bool,
}

static mut BRUSH: Option<RwLock<Brush>> = None;

#[no_mangle]
pub unsafe extern "C" fn br_create_brush(file: *const c_char) -> i32 {
    if BRUSH.is_some() {
        return br_add_font(file);
    }

    use std::io::prelude::*;
    let mut file = if let Ok(file) = File::open(CStr::from_ptr(file).to_str().unwrap()) {
        file
    } else {
        return -1;
    };

    let mut buffer = Vec::new();
    if let Err(_) = file.read_to_end(&mut buffer) {
        return -1;
    }

    let font = if let Ok(f) = FontArc::try_from_vec(buffer) {
        f
    } else {
        return -1;
    };

    let brush = GlyphBrushBuilder::using_font(font).build();
    let (tex_width, tex_height) = brush.texture_dimensions();

    let brush = Brush {
        brush,
        tex_width,
        tex_height,
        tex_data: vec![0; (tex_width * tex_height) as usize],
        vertices: Vec::new(),
        tex_changed: false,
    };

    BRUSH = Some(RwLock::new(brush));
    0
}

#[no_mangle]
pub unsafe extern "C" fn br_get_texture_dimensions(tex_width: *mut u32, tex_height: *mut u32) {
    let brush = if let Some(b) = BRUSH.as_ref() {
        b.read().unwrap()
    } else {
        return;
    };

    if tex_width.is_null() || tex_height.is_null() {
        return;
    }

    *tex_width = brush.tex_width;
    *tex_height = brush.tex_height;
}

#[no_mangle]
pub extern "C" fn br_update() {
    let mut brush = unsafe {
        if let Some(b) = BRUSH.as_ref() {
            b.write().unwrap()
        } else {
            return;
        }
    };

    let tex_width = brush.tex_width as usize;
    let mut tex_data = Vec::new();
    std::mem::swap(&mut tex_data, &mut brush.tex_data);
    let mut tex_changed = false;

    match brush.brush.process_queued(
        |rect, t_data| {
            let offset: [u32; 2] = [rect.min[0], rect.min[1]];
            let size: [u32; 2] = [rect.width(), rect.height()];

            let width = size[0] as usize;
            let height = size[1] as usize;

            for y in 0..height {
                for x in 0..width {
                    let index = x + y * width;
                    let alpha = t_data[index];
                    let index = (x + offset[0] as usize) + (offset[1] as usize + y) * tex_width;
                    tex_data[index] = alpha;
                }
            }

            tex_changed = true;
        },
        to_vertex,
    ) {
        Ok(BrushAction::Draw(vertices)) => {
            brush.vertices = vertices;
        }
        Ok(BrushAction::ReDraw) => {}
        Err(BrushError::TextureTooSmall { suggested }) => {
            tex_changed = true;
            brush
                .tex_data
                .resize((suggested.0 * suggested.1) as usize, 0);
            brush.tex_width = suggested.0;
            brush.tex_height = suggested.1;
        }
    }

    std::mem::swap(&mut tex_data, &mut brush.tex_data);
    brush.tex_changed |= tex_changed;
}

#[no_mangle]
pub unsafe extern "C" fn br_get_texture_data(needs_update: *mut u32) -> *const u8 {
    let mut brush = if let Some(b) = BRUSH.as_ref() {
        b.write().unwrap()
    } else {
        return std::ptr::null();
    };

    if let Some(u) = needs_update.as_mut() {
        *u = if brush.tex_changed { 1 } else { 0 };
        brush.tex_changed = false;
    }

    brush.tex_data.as_ptr() as *const u8
}

#[no_mangle]
pub unsafe extern "C" fn br_add_font_from_bytes(bytes: *const u8, count: u32) -> i32 {
    let bytes = if !bytes.is_null() {
        std::slice::from_raw_parts(bytes, count as _)
    } else {
        return -1;
    };

    let bytes = bytes.to_vec();
    let font = if let Ok(f) = FontArc::try_from_vec(bytes) {
        f
    } else {
        return -1;
    };

    if let Some(b) = BRUSH.as_ref() {
        let mut b = b.write().unwrap();
        b.brush.add_font(font).0 as i32
    } else {
        -1
    }
}

#[no_mangle]
pub unsafe extern "C" fn br_add_font(file: *const c_char) -> i32 {
    use std::io::prelude::*;
    let mut file = if let Ok(file) = File::open(CStr::from_ptr(file).to_str().unwrap()) {
        file
    } else {
        return -1;
    };

    let mut buffer = Vec::new();
    if let Err(_) = file.read_to_end(&mut buffer) {
        return -1;
    }

    br_add_font_from_bytes(buffer.as_ptr(), buffer.len() as _)
}

#[no_mangle]
pub unsafe extern "C" fn br_queue_text(section: TextSection) {
    let text = if let Ok(s) = CStr::from_ptr(section.text).to_str() {
        s
    } else {
        return;
    };

    let section = Section::<()>::new()
        .with_screen_position((section.screen_position_x, section.screen_position_y))
        // .with_bounds((section.bounds_x, section.bounds_y))
        .with_text(vec![Text::new(text)
            .with_font_id(FontId(section.font_id as _))
            .with_color([
                section.color_r,
                section.color_g,
                section.color_b,
                section.color_a,
            ])
            .with_scale(PxScale {
                x: section.scale_x,
                y: section.scale_y,
            })]);

    let mut brush = if let Some(b) = BRUSH.as_ref() {
        b.write().unwrap()
    } else {
        return;
    };

    brush.brush.queue(section);
}

#[no_mangle]
pub unsafe extern "C" fn br_get_vertices(count: *mut u32) -> *const BrushVertex {
    let brush = if let Some(b) = BRUSH.as_ref() {
        b.read().unwrap()
    } else {
        return std::ptr::null();
    };

    if let Some(c) = count.as_mut() {
        *c = brush.vertices.len() as u32;
    }

    brush.vertices.as_ptr()
}

#[derive(Debug, Copy, Clone)]
#[repr(C)]
pub struct BrushVertex {
    pub min_x: f32,
    pub min_y: f32,

    pub max_x: f32,
    pub max_y: f32,

    pub uv_min_x: f32,
    pub uv_min_y: f32,

    pub uv_max_x: f32,
    pub uv_max_y: f32,
    pub color_r: f32,
    pub color_g: f32,
    pub color_b: f32,
    pub color_a: f32,
}

#[inline]
fn to_vertex(
    glyph_brush::GlyphVertex {
        mut tex_coords,
        pixel_coords,
        bounds,
        extra,
    }: glyph_brush::GlyphVertex,
) -> BrushVertex {
    let gl_bounds = bounds;

    use glyph_brush::ab_glyph::Rect;

    let mut gl_rect = Rect {
        min: point(pixel_coords.min.x as f32, pixel_coords.min.y as f32),
        max: point(pixel_coords.max.x as f32, pixel_coords.max.y as f32),
    };

    if gl_rect.max.x > gl_bounds.max.x {
        let old_width = gl_rect.width();
        gl_rect.max.x = gl_bounds.max.x;
        tex_coords.max.x = tex_coords.min.x + tex_coords.width() * gl_rect.width() / old_width;
    }

    if gl_rect.min.x < gl_bounds.min.x {
        let old_width = gl_rect.width();
        gl_rect.min.x = gl_bounds.min.x;
        tex_coords.min.x = tex_coords.max.x - tex_coords.width() * gl_rect.width() / old_width;
    }

    if gl_rect.max.y > gl_bounds.max.y {
        let old_height = gl_rect.height();
        gl_rect.max.y = gl_bounds.max.y;
        tex_coords.max.y = tex_coords.min.y + tex_coords.height() * gl_rect.height() / old_height;
    }

    if gl_rect.min.y < gl_bounds.min.y {
        let old_height = gl_rect.height();
        gl_rect.min.y = gl_bounds.min.y;
        tex_coords.min.y = tex_coords.max.y - tex_coords.height() * gl_rect.height() / old_height;
    }

    BrushVertex {
        min_x: gl_rect.min.x,
        min_y: gl_rect.min.y,
        max_x: gl_rect.max.x,
        max_y: gl_rect.max.y,
        uv_min_x: tex_coords.min.x,
        uv_min_y: tex_coords.min.y,
        uv_max_x: tex_coords.max.x,
        uv_max_y: tex_coords.max.y,
        color_r: extra.color[0],
        color_g: extra.color[1],
        color_b: extra.color[2],
        color_a: extra.color[3],
    }
}
