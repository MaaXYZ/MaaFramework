use crate::define::*;
use crate::error::{MaaError, Result};
use crate::ffi;
use std::ffi::{CStr, CString};

pub struct StringBuffer {
    handle: MaaStringBufferHandle,
    own: bool,
}

impl StringBuffer {
    pub fn new() -> Self {
        Self {
            handle: ffi::maa_string_buffer_create(),
            own: true,
        }
    }

    pub fn from_handle(handle: MaaStringBufferHandle) -> Self {
        Self { handle, own: false }
    }

    pub fn handle(&self) -> MaaStringBufferHandle {
        self.handle
    }

    pub fn get(&self) -> Result<String> {
        let ptr = ffi::maa_string_buffer_get(self.handle);
        if ptr.is_null() {
            return Ok(String::new());
        }
        let cstr = unsafe { CStr::from_ptr(ptr) };
        Ok(cstr.to_string_lossy().into_owned())
    }

    pub fn set(&self, value: &str) -> Result<bool> {
        let cstr = CString::new(value).map_err(|e| MaaError::BufferError(e.to_string()))?;
        Ok(ffi::maa_string_buffer_set(self.handle, cstr.as_ptr()) != 0)
    }

    pub fn size(&self) -> u64 {
        ffi::maa_string_buffer_size(self.handle)
    }
}

impl Default for StringBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for StringBuffer {
    fn drop(&mut self) {
        if self.own && !self.handle.is_null() {
            ffi::maa_string_buffer_destroy(self.handle);
        }
    }
}

pub struct ImageBuffer {
    handle: MaaImageBufferHandle,
    own: bool,
}

impl ImageBuffer {
    pub fn new() -> Self {
        Self {
            handle: ffi::maa_image_buffer_create(),
            own: true,
        }
    }

    pub fn from_handle(handle: MaaImageBufferHandle) -> Self {
        Self { handle, own: false }
    }

    pub fn handle(&self) -> MaaImageBufferHandle {
        self.handle
    }

    pub fn get_raw_data(&self) -> *mut u8 {
        ffi::maa_image_buffer_get_raw_data(self.handle)
    }

    pub fn width(&self) -> i32 {
        ffi::maa_image_buffer_width(self.handle)
    }

    pub fn height(&self) -> i32 {
        ffi::maa_image_buffer_height(self.handle)
    }

    pub fn channels(&self) -> i32 {
        ffi::maa_image_buffer_channels(self.handle)
    }

    pub fn image_type(&self) -> i32 {
        ffi::maa_image_buffer_type(self.handle)
    }

    pub fn set_raw_data(&self, data: &[u8], width: i32, height: i32, img_type: i32) -> bool {
        ffi::maa_image_buffer_set_raw_data(self.handle, data.as_ptr(), width, height, img_type) != 0
    }

    pub fn to_vec(&self) -> Vec<u8> {
        let size = (self.width() * self.height() * self.channels()) as usize;
        if size == 0 {
            return Vec::new();
        }
        let ptr = self.get_raw_data();
        if ptr.is_null() {
            return Vec::new();
        }
        let slice = unsafe { std::slice::from_raw_parts(ptr, size) };
        slice.to_vec()
    }
}

impl Default for ImageBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for ImageBuffer {
    fn drop(&mut self) {
        if self.own && !self.handle.is_null() {
            ffi::maa_image_buffer_destroy(self.handle);
        }
    }
}

pub struct RectBuffer {
    handle: MaaRectHandle,
}

impl RectBuffer {
    pub fn new() -> Self {
        Self {
            handle: ffi::maa_rect_create(),
        }
    }

    pub fn handle(&self) -> MaaRectHandle {
        self.handle
    }

    pub fn get(&self) -> Rect {
        Rect {
            x: ffi::maa_rect_get_x(self.handle),
            y: ffi::maa_rect_get_y(self.handle),
            width: ffi::maa_rect_get_w(self.handle),
            height: ffi::maa_rect_get_h(self.handle),
        }
    }

    pub fn set(&self, rect: &Rect) -> bool {
        ffi::maa_rect_set(self.handle, rect.x, rect.y, rect.width, rect.height) != 0
    }
}

impl Default for RectBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for RectBuffer {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            ffi::maa_rect_destroy(self.handle);
        }
    }
}

pub struct StringListBuffer {
    handle: MaaStringListBufferHandle,
}

impl StringListBuffer {
    pub fn new() -> Self {
        Self {
            handle: ffi::maa_string_list_buffer_create(),
        }
    }

    pub fn handle(&self) -> MaaStringListBufferHandle {
        self.handle
    }

    pub fn size(&self) -> u64 {
        ffi::maa_string_list_buffer_size(self.handle)
    }

    pub fn get(&self) -> Result<Vec<String>> {
        let size = self.size();
        let mut result = Vec::with_capacity(size as usize);
        for i in 0..size {
            let str_handle = ffi::maa_string_list_buffer_at(self.handle, i);
            let str_buf = StringBuffer::from_handle(str_handle);
            result.push(str_buf.get()?);
        }
        Ok(result)
    }

    pub fn set(&self, values: &[&str]) -> Result<bool> {
        for value in values {
            let str_buf = StringBuffer::new();
            str_buf.set(value)?;
            if ffi::maa_string_list_buffer_append(self.handle, str_buf.handle()) == 0 {
                return Ok(false);
            }
        }
        Ok(true)
    }
}

impl Default for StringListBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for StringListBuffer {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            ffi::maa_string_list_buffer_destroy(self.handle);
        }
    }
}

pub struct ImageListBuffer {
    handle: MaaImageListBufferHandle,
}

impl ImageListBuffer {
    pub fn new() -> Self {
        Self {
            handle: ffi::maa_image_list_buffer_create(),
        }
    }

    pub fn handle(&self) -> MaaImageListBufferHandle {
        self.handle
    }

    pub fn size(&self) -> u64 {
        ffi::maa_image_list_buffer_size(self.handle)
    }

    pub fn get(&self) -> Vec<ImageBuffer> {
        let size = self.size();
        let mut result = Vec::with_capacity(size as usize);
        for i in 0..size {
            let img_handle = ffi::maa_image_list_buffer_at(self.handle, i);
            result.push(ImageBuffer::from_handle(img_handle));
        }
        result
    }
}

impl Default for ImageListBuffer {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for ImageListBuffer {
    fn drop(&mut self) {
        if !self.handle.is_null() {
            ffi::maa_image_list_buffer_destroy(self.handle);
        }
    }
}

