pub struct DEM {
    pub axes: crate::axes::Axes,
    pub width: u32,
    pub tvs_width: u32,
    pub size: u64,
    pub scale: f64,
    pub shift_angle: f64,
    pub max_line_of_sight: u32,
    pub max_los_as_points: u32,
    pub computable_points_count: u64,
    pub band_size: u64,
}

impl DEM {
    pub fn new(width: u32, scale: f64, shift_angle: f64, max_line_of_sight: u32) -> Self {
        let size = (width * width) as u64;

        let mut dem = Self {
            axes: crate::axes::Axes::default(),
            width,
            tvs_width: 0,
            size,
            scale,
            shift_angle,
            max_line_of_sight,
            max_los_as_points: (max_line_of_sight as f64 / scale) as u32,
            computable_points_count: 0,
            band_size: ((max_line_of_sight as f64 / scale) + 1.0) as u64,
        };
        dem.count_computable_points();
        dem.tvs_width = (dem.computable_points_count as f64).sqrt() as u32;
        dem
    }

    fn count_computable_points(&mut self) {
        for point in 0..self.size {
            if self.is_point_computable(point) {
                self.computable_points_count += 1;
            }
        }
    }

    // Depending on the requested max line of sight, only certain points in the middle of the DEM
    // can truly have their total visible surfaces calculated. This is because points on the edge
    // do not have access to elevation data that may or not be visible.
    fn is_point_computable(&self, dem_id: u64) -> bool {
        let x = (dem_id % self.width as u64) as f64 * self.scale;
        let y = (dem_id.div_euclid(self.width as u64)) as f64 * self.scale;
        let lower_x = self.max_line_of_sight as f64;
        let upper_x = ((self.width - 1) as f64 * self.scale) - self.max_line_of_sight as f64;
        let lower_y = self.max_line_of_sight as f64;
        let upper_y = ((self.width - 1) as f64 * self.scale) - self.max_line_of_sight as f64;
        x >= lower_x && x <= upper_x && y >= lower_y && y <= upper_y
    }

    pub fn pov_id_to_tvs_id(&self, pov_id: u64) -> u64 {
        let x = (pov_id % self.width as u64) - self.max_los_as_points as u64;
        let y = pov_id.div_euclid(self.width as u64) - self.max_los_as_points as u64;
        (y * self.tvs_width as u64) + x
    }

    pub fn tvs_id_to_pov_id(&self, tvs_id: u64) -> u64 {
        let x = (tvs_id % self.tvs_width as u64) + self.max_los_as_points as u64;
        let y = tvs_id.div_euclid(self.tvs_width as u64) + self.max_los_as_points as u64;
        (y * self.width as u64) + x
    }

    pub fn calculate_axes(&mut self, angle: f64) {
        self.axes = crate::axes::Axes::new(self.width, angle, self.shift_angle);
        self.axes.compute();
    }
}
