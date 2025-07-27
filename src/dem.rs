pub struct DEM {
    pub width: u32,
    pub size: u64,
    pub scale: f64,
}

impl DEM {
    pub fn new(width: u32, scale: f64, shift_angle: f64) -> Self {
        let size = (width * width) as u64;
        Self { width, size, scale }
    }

    pub fn pre_calculate(&mut self) {}
}
