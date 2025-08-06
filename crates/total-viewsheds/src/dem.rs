//! DEM: Digital Elevation Model
//! <https://en.wikipedia.org/wiki/Digital_elevation_model>
//!
//! A 1D array of 2D data. Each item represents the height of a point above sea level. It doesn't
//! contain coordinates itself. But they can be derived from the position of the item in the array
//! and the known location of the DEM origins itself.

use color_eyre::Result;

/// `DEM`
pub struct DEM {
    /// Trigonomic data about the points when you rotate the DEM through certain angles.
    pub axes: crate::axes::Axes,
    /// The band deltas for the current sector.
    pub band_deltas: Vec<i32>,
    /// All the elevation data.
    pub elevations: Vec<f32>,
    /// The width of the DEM.
    pub width: u32,
    /// The width of the computable sub-grid within the DEM. Consider a point on the edge of the
    /// DEM, whilst we cannot calculate its viewshed, it is required to calculate a truly coputable
    /// point further inside the DEM.
    pub tvs_width: u32,
    /// The total number of points in the DEM.
    pub size: u32,
    /// The size of each point in meters.
    pub scale: f32,
    /// The maximum distance in metres to search for visible points.
    pub max_line_of_sight: u32,
    /// The maximum distance in terms of points to search.
    pub max_los_as_points: u32,
    /// The total number of points that can have full viewsheds calculated for them.
    pub computable_points_count: u32,
    /// The size of a "band of sight". This is generally the number of points that fit into the max
    /// line of sight. But it could be more, not to increase the distance, but to improve
    /// interpolation.
    pub band_size: u32,
}

impl DEM {
    /// `Instantiate`
    pub fn new(width: u32, scale: f32, max_line_of_sight: u32) -> Result<Self> {
        let size = width * width;
        #[expect(
            clippy::cast_possible_truncation,
            clippy::as_conversions,
            clippy::cast_sign_loss,
            clippy::cast_precision_loss,
            reason = "This shouldn't be a problem in most sane cases"
        )]
        let max_los_as_points = (max_line_of_sight as f32 / scale) as u32;
        let max_possible_los_as_points = width.div_euclid(2);

        if max_los_as_points > max_possible_los_as_points {
            color_eyre::eyre::bail!(
                "The maximum line of sight ({max_line_of_sight}m) is longer than the maximum \
                distance that any point can completely see ({}m).",
                f64::from(max_possible_los_as_points) * f64::from(scale)
            );
        }

        let mut dem = Self {
            axes: crate::axes::Axes::default(),
            band_deltas: Vec::default(),
            elevations: Vec::default(),
            width,
            tvs_width: 0,
            size,
            scale,
            max_line_of_sight,
            max_los_as_points,
            computable_points_count: 0,
            // Add 1 just to be sure that we always compute points within the line of sight, and no
            // less.
            band_size: max_los_as_points + 1,
        };
        tracing::debug!("band_size: {}", dem.band_size);
        dem.count_computable_points();
        dem.tvs_width = dem.computable_points_count.isqrt();
        Ok(dem)
    }

    /// Count the number of points in the DEM that can have their viewsheds fully calculated.
    fn count_computable_points(&mut self) {
        self.computable_points_count = 0;
        for point in 0..self.size {
            if self.is_point_computable(point) {
                self.computable_points_count += 1;
            }
        }
    }

    /// Depending on the requested max line of sight, only certain points in the middle of the DEM
    /// can truly have their total visible surfaces calculated. This is because points on the edge
    /// do not have access to further elevation data.
    #[expect(
        clippy::as_conversions,
        clippy::cast_precision_loss,
        reason = "
          We're just calculating _what's_ computable, not doing the calculations themselves.
        "
    )]
    fn is_point_computable(&self, dem_id: u32) -> bool {
        let max_line_of_sight_f32 = self.max_line_of_sight as f32;
        let x = (dem_id.rem_euclid(self.width)) as f32 * self.scale;
        let y = (dem_id.div_euclid(self.width)) as f32 * self.scale;
        let lower = max_line_of_sight_f32;
        #[expect(clippy::suboptimal_flops, reason = "Readability is more important")]
        let upper = ((self.width - 1) as f32 * self.scale) - max_line_of_sight_f32;
        x >= lower && x <= upper && y >= lower && y <= upper
    }

    /// Convert an original DEM ID to the coordinate system of the computable points sub-DEM.
    #[expect(dead_code, reason = "We'll use it in the next commit")]
    pub fn pov_id_to_tvs_id(&self, pov_id: u64) -> u64 {
        let max_los_as_points_u64 = u64::from(self.max_los_as_points);
        let width_u64 = u64::from(self.width);
        let x = pov_id.rem_euclid(width_u64) - max_los_as_points_u64;
        let y = pov_id.div_euclid(width_u64) - max_los_as_points_u64;
        (y * u64::from(self.tvs_width)) + x
    }

    /// Convert a computable sub-DEM ID to its original DEM ID.
    #[cfg(test)]
    pub const fn tvs_id_to_pov_id(&self, tvs_id: u32) -> u32 {
        let x = tvs_id.rem_euclid(self.tvs_width) + self.max_los_as_points;
        let y = tvs_id.div_euclid(self.tvs_width) + self.max_los_as_points;
        (y * self.width) + x
    }

    /// Do the calculations needed to create bands for a new angle.
    pub fn calculate_axes(&mut self, angle: f32) -> Result<()> {
        self.axes = crate::axes::Axes::new(self.width, angle)?;
        self.axes.compute();
        Ok(())
    }
}
