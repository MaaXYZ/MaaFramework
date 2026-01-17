//! Safe buffer types for FFI data exchange.
//!
//! These buffers provide RAII-based memory management for data passed between
//! Rust and the C API. All buffers are automatically deallocated when dropped.
//!
//! # Buffer Types
//!
//! - [`MaaStringBuffer`] - UTF-8 string data
//! - [`MaaImageBuffer`] - Image data (supports PNG/JPEG encoding)
//! - [`MaaStringListBuffer`] - List of strings
//! - [`MaaImageListBuffer`] - List of images
//! - [`MaaRectBuffer`] - Rectangle coordinates

use std::ffi::{CStr, CString};
use std::ptr::NonNull;

use crate::{sys, MaaError, MaaResult};

/// A string buffer for UTF-8 text data.
///
/// Used for passing strings between Rust and the C API.
/// Automatically freed when dropped.
pub struct MaaStringBuffer {
    handle: NonNull<sys::MaaStringBuffer>,
}

impl MaaStringBuffer {
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaStringBufferCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    pub fn set(&mut self, content: &str) -> MaaResult<()> {
        let c_str = CString::new(content)?;
        let ret = unsafe { sys::MaaStringBufferSet(self.handle.as_ptr(), c_str.as_ptr()) };
        crate::common::check_bool(ret)
    }

    pub fn set_ex(&mut self, content: &str) -> MaaResult<()> {
        let c_str = CString::new(content)?;
        let ret = unsafe {
            sys::MaaStringBufferSetEx(self.handle.as_ptr(), c_str.as_ptr(), content.len() as u64)
        };
        crate::common::check_bool(ret)
    }

    pub fn as_str(&self) -> &str {
        unsafe {
            let ptr = sys::MaaStringBufferGet(self.handle.as_ptr());
            if ptr.is_null() {
                ""
            } else {
                CStr::from_ptr(ptr).to_str().unwrap_or("")
            }
        }
    }

    pub fn raw(&self) -> *mut sys::MaaStringBuffer {
        self.handle.as_ptr()
    }
}

impl Drop for MaaStringBuffer {
    fn drop(&mut self) {
        unsafe { sys::MaaStringBufferDestroy(self.handle.as_ptr()) }
    }
}

impl ToString for MaaStringBuffer {
    fn to_string(&self) -> String {
        self.as_str().to_string()
    }
}

impl From<&str> for MaaStringBuffer {
    fn from(s: &str) -> Self {
        let mut buf = Self::new().expect("Failed to create MaaStringBuffer");
        buf.set(s).expect("Failed to set string buffer content");
        buf
    }
}

/// Image buffer for storing and manipulating image data.
///
/// `MaaImageBuffer` wraps the C API's image buffer, providing safe access to
/// image data including dimensions, pixel format, and encoded bytes.
///
/// # Example
/// ```ignore
/// let buffer = MaaImageBuffer::new()?;
/// let width = buffer.width();
/// let height = buffer.height();
/// if let Some(bytes) = buffer.to_vec() {
///     // Process PNG/JPEG encoded bytes
/// }
/// ```
pub struct MaaImageBuffer {
    handle: NonNull<sys::MaaImageBuffer>,
}

impl MaaImageBuffer {
    /// Create a new empty image buffer.
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaImageBufferCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    /// Check if the buffer is empty (contains no image data).
    pub fn is_empty(&self) -> bool {
        unsafe { sys::MaaImageBufferIsEmpty(self.handle.as_ptr()) != 0 }
    }

    /// Clear the buffer, releasing any image data.
    pub fn clear(&mut self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaImageBufferClear(self.handle.as_ptr()) };
        crate::common::check_bool(ret)
    }

    /// Get the raw pointer to the underlying C buffer.
    pub fn raw(&self) -> *mut sys::MaaImageBuffer {
        self.handle.as_ptr()
    }

    /// Get the image width in pixels.
    pub fn width(&self) -> i32 {
        unsafe { sys::MaaImageBufferWidth(self.handle.as_ptr()) }
    }

    /// Get the image height in pixels.
    pub fn height(&self) -> i32 {
        unsafe { sys::MaaImageBufferHeight(self.handle.as_ptr()) }
    }

    /// Get the number of color channels (e.g., 3 for RGB, 4 for RGBA).
    pub fn channels(&self) -> i32 {
        unsafe { sys::MaaImageBufferChannels(self.handle.as_ptr()) }
    }

    /// Get the OpenCV image type constant.
    pub fn image_type(&self) -> i32 {
        unsafe { sys::MaaImageBufferType(self.handle.as_ptr()) }
    }

    /// Get the encoded image data as a byte vector (PNG format).
    ///
    /// Returns `None` if the buffer is empty or encoding fails.
    pub fn to_vec(&self) -> Option<Vec<u8>> {
        unsafe {
            let ptr = sys::MaaImageBufferGetEncoded(self.handle.as_ptr());
            let size = sys::MaaImageBufferGetEncodedSize(self.handle.as_ptr());
            if !ptr.is_null() && size > 0 {
                let slice = std::slice::from_raw_parts(ptr, size as usize);
                Some(slice.to_vec())
            } else {
                None
            }
        }
    }

    /// Get the raw image data (BGR format, row-major).
    ///
    /// Returns the raw pixel data as used by OpenCV. The format is typically
    /// BGR with 3 channels, stored row by row.
    pub fn raw_data(&self) -> Option<&[u8]> {
        unsafe {
            let ptr = sys::MaaImageBufferGetRawData(self.handle.as_ptr());
            if ptr.is_null() {
                return None;
            }
            let w = self.width() as usize;
            let h = self.height() as usize;
            let c = self.channels() as usize;
            if w == 0 || h == 0 || c == 0 {
                return None;
            }
            Some(std::slice::from_raw_parts(ptr as *const u8, w * h * c))
        }
    }

    /// Set the image from raw BGR data.
    ///
    /// # Arguments
    /// * `data` - Raw pixel data in BGR format
    /// * `width` - Image width
    /// * `height` - Image height
    /// * `img_type` - OpenCV image type (e.g., CV_8UC3 = 16)
    pub fn set_raw_data(
        &mut self,
        data: &[u8],
        width: i32,
        height: i32,
        img_type: i32,
    ) -> MaaResult<()> {
        let ret = unsafe {
            sys::MaaImageBufferSetRawData(
                self.handle.as_ptr(),
                data.as_ptr() as *mut std::ffi::c_void,
                width,
                height,
                img_type,
            )
        };
        crate::common::check_bool(ret)
    }

    /// Set the image from encoded data (PNG/JPEG).
    ///
    /// # Arguments
    /// * `data` - Encoded image bytes (PNG or JPEG format)
    pub fn set_encoded(&mut self, data: &[u8]) -> MaaResult<()> {
        let ret = unsafe {
            sys::MaaImageBufferSetEncoded(
                self.handle.as_ptr(),
                data.as_ptr() as *mut u8,
                data.len() as u64,
            )
        };
        crate::common::check_bool(ret)
    }

    /// Convert to `image` crate's `DynamicImage`.
    ///
    /// This method requires the `image` feature to be enabled.
    ///
    /// # Example
    /// ```ignore
    /// let buffer = controller.cached_image()?;
    /// let img = buffer.to_dynamic_image()?;
    /// img.save("screenshot.png")?;
    /// ```
    #[cfg(feature = "image")]
    pub fn to_dynamic_image(&self) -> MaaResult<image::DynamicImage> {
        let encoded = self.to_vec().ok_or(MaaError::ImageConversionError)?;
        image::load_from_memory(&encoded).map_err(|_| MaaError::ImageConversionError)
    }

    /// Create an image buffer from `image` crate's `DynamicImage`.
    ///
    /// This method requires the `image` feature to be enabled.
    ///
    /// # Example
    /// ```ignore
    /// let img = image::open("template.png")?;
    /// let buffer = MaaImageBuffer::from_dynamic_image(&img)?;
    /// ```
    #[cfg(feature = "image")]
    pub fn from_dynamic_image(img: &image::DynamicImage) -> MaaResult<Self> {
        use std::io::Cursor;

        let mut bytes = Vec::new();
        img.write_to(&mut Cursor::new(&mut bytes), image::ImageFormat::Png)
            .map_err(|_| MaaError::ImageConversionError)?;

        let mut buffer = Self::new()?;
        buffer.set_encoded(&bytes)?;
        Ok(buffer)
    }

    /// Create an image buffer from an RGB image.
    ///
    /// This method requires the `image` feature to be enabled.
    #[cfg(feature = "image")]
    pub fn from_rgb_image(img: &image::RgbImage) -> MaaResult<Self> {
        Self::from_dynamic_image(&image::DynamicImage::ImageRgb8(img.clone()))
    }

    /// Create an image buffer from an RGBA image.
    ///
    /// This method requires the `image` feature to be enabled.
    #[cfg(feature = "image")]
    pub fn from_rgba_image(img: &image::RgbaImage) -> MaaResult<Self> {
        Self::from_dynamic_image(&image::DynamicImage::ImageRgba8(img.clone()))
    }
}

impl Drop for MaaImageBuffer {
    fn drop(&mut self) {
        unsafe { sys::MaaImageBufferDestroy(self.handle.as_ptr()) }
    }
}

/// A list buffer for storing multiple images.
///
/// Used when APIs return multiple images (e.g., recognition draws).
pub struct MaaImageListBuffer {
    handle: NonNull<sys::MaaImageListBuffer>,
}

impl MaaImageListBuffer {
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaImageListBufferCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    pub fn raw(&self) -> *mut sys::MaaImageListBuffer {
        self.handle.as_ptr()
    }

    pub fn len(&self) -> usize {
        unsafe { sys::MaaImageListBufferSize(self.handle.as_ptr()) as usize }
    }

    pub fn is_empty(&self) -> bool {
        unsafe { sys::MaaImageListBufferIsEmpty(self.handle.as_ptr()) != 0 }
    }

    /// Get image at index. Returns None if index out of bounds.
    /// Note: The returned buffer is a view into this list, do not destroy it separately.
    pub fn at(&self, index: usize) -> Option<MaaImageBufferRef> {
        unsafe {
            let ptr = sys::MaaImageListBufferAt(self.handle.as_ptr(), index as u64);
            if ptr.is_null() {
                None
            } else {
                Some(MaaImageBufferRef {
                    ptr: ptr as *mut sys::MaaImageBuffer,
                })
            }
        }
    }

    pub fn append(&self, image: &MaaImageBuffer) -> MaaResult<()> {
        let ret = unsafe { sys::MaaImageListBufferAppend(self.handle.as_ptr(), image.raw()) };
        crate::common::check_bool(ret)
    }

    pub fn remove(&self, index: usize) -> MaaResult<()> {
        let ret = unsafe { sys::MaaImageListBufferRemove(self.handle.as_ptr(), index as u64) };
        crate::common::check_bool(ret)
    }

    pub fn clear(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaImageListBufferClear(self.handle.as_ptr()) };
        crate::common::check_bool(ret)
    }

    /// Get all images as a vector of encoded byte vectors (PNG format).
    pub fn to_vec_of_vec(&self) -> Vec<Vec<u8>> {
        let mut result = Vec::with_capacity(self.len());
        for i in 0..self.len() {
            if let Some(img) = self.at(i) {
                if let Some(data) = img.to_vec() {
                    result.push(data);
                }
            }
        }
        result
    }
}

impl Drop for MaaImageListBuffer {
    fn drop(&mut self) {
        unsafe { sys::MaaImageListBufferDestroy(self.handle.as_ptr()) }
    }
}

/// A borrowed reference to an image buffer within an image list.
/// This does not own the buffer and should not be destroyed.
pub struct MaaImageBufferRef {
    ptr: *mut sys::MaaImageBuffer,
}

impl MaaImageBufferRef {
    pub fn width(&self) -> i32 {
        unsafe { sys::MaaImageBufferWidth(self.ptr) }
    }

    pub fn height(&self) -> i32 {
        unsafe { sys::MaaImageBufferHeight(self.ptr) }
    }

    pub fn channels(&self) -> i32 {
        unsafe { sys::MaaImageBufferChannels(self.ptr) }
    }

    pub fn to_vec(&self) -> Option<Vec<u8>> {
        unsafe {
            let ptr = sys::MaaImageBufferGetEncoded(self.ptr);
            let size = sys::MaaImageBufferGetEncodedSize(self.ptr);
            if !ptr.is_null() && size > 0 {
                let slice = std::slice::from_raw_parts(ptr, size as usize);
                Some(slice.to_vec())
            } else {
                None
            }
        }
    }
}

/// A list buffer for storing multiple strings.
///
/// Used for APIs that return or accept string lists (e.g., node lists).
pub struct MaaStringListBuffer {
    handle: NonNull<sys::MaaStringListBuffer>,
}

impl MaaStringListBuffer {
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaStringListBufferCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self { handle: ptr })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    pub fn raw(&self) -> *mut sys::MaaStringListBuffer {
        self.handle.as_ptr()
    }

    pub fn to_vec(&self) -> Vec<String> {
        unsafe {
            let size = sys::MaaStringListBufferSize(self.handle.as_ptr());
            let mut vec = Vec::with_capacity(size as usize);
            for i in 0..size {
                let ptr = sys::MaaStringListBufferAt(self.handle.as_ptr(), i);
                if !ptr.is_null() {
                    let c_str = sys::MaaStringBufferGet(ptr);
                    if !c_str.is_null() {
                        vec.push(CStr::from_ptr(c_str).to_string_lossy().into_owned());
                    }
                }
            }
            vec
        }
    }

    pub fn len(&self) -> usize {
        unsafe { sys::MaaStringListBufferSize(self.handle.as_ptr()) as usize }
    }

    pub fn is_empty(&self) -> bool {
        unsafe { sys::MaaStringListBufferIsEmpty(self.handle.as_ptr()) != 0 }
    }

    pub fn append(&self, s: &str) -> MaaResult<()> {
        let mut str_buf = MaaStringBuffer::new()?;
        str_buf.set(s)?;
        let ret = unsafe { sys::MaaStringListBufferAppend(self.handle.as_ptr(), str_buf.raw()) };
        if ret != 0 {
            Ok(())
        } else {
            Err(crate::MaaError::FrameworkError(0))
        }
    }

    pub fn remove(&self, index: usize) -> MaaResult<()> {
        let ret = unsafe { sys::MaaStringListBufferRemove(self.handle.as_ptr(), index as u64) };
        crate::common::check_bool(ret)
    }

    pub fn clear(&self) -> MaaResult<()> {
        let ret = unsafe { sys::MaaStringListBufferClear(self.handle.as_ptr()) };
        crate::common::check_bool(ret)
    }
}

impl Drop for MaaStringListBuffer {
    fn drop(&mut self) {
        unsafe { sys::MaaStringListBufferDestroy(self.handle.as_ptr()) }
    }
}

/// Rect buffer for coordinate passing between Rust and C API.
pub struct MaaRectBuffer {
    handle: NonNull<sys::MaaRect>,
    own: bool,
}

impl MaaRectBuffer {
    /// Create a new rect buffer.
    pub fn new() -> MaaResult<Self> {
        let handle = unsafe { sys::MaaRectCreate() };
        if let Some(ptr) = NonNull::new(handle) {
            Ok(Self {
                handle: ptr,
                own: true,
            })
        } else {
            Err(MaaError::NullPointer)
        }
    }

    /// Create from an existing handle (does not take ownership).
    pub fn from_handle(handle: *mut sys::MaaRect) -> Option<Self> {
        NonNull::new(handle).map(|ptr| Self {
            handle: ptr,
            own: false,
        })
    }

    pub fn raw(&self) -> *mut sys::MaaRect {
        self.handle.as_ptr()
    }

    /// Get the rect values.
    pub fn get(&self) -> crate::common::Rect {
        unsafe {
            crate::common::Rect {
                x: sys::MaaRectGetX(self.handle.as_ptr()),
                y: sys::MaaRectGetY(self.handle.as_ptr()),
                width: sys::MaaRectGetW(self.handle.as_ptr()),
                height: sys::MaaRectGetH(self.handle.as_ptr()),
            }
        }
    }

    /// Set the rect values.
    pub fn set(&mut self, rect: &crate::common::Rect) -> MaaResult<()> {
        let ret = unsafe {
            sys::MaaRectSet(
                self.handle.as_ptr(),
                rect.x,
                rect.y,
                rect.width,
                rect.height,
            )
        };
        crate::common::check_bool(ret)
    }
}

impl Drop for MaaRectBuffer {
    fn drop(&mut self) {
        if self.own {
            unsafe { sys::MaaRectDestroy(self.handle.as_ptr()) }
        }
    }
}
