//! The main entrypoint for running computations.

use color_eyre::Result;

/// Handles all the computations.
pub struct Compute<'compute> {
    /// The Digital Elevation Model that we're computing.
    dem: &'compute mut crate::dem::DEM,
    /// The constants for each kernel computation.
    pub constants: crate::kernel::Constants,
    /// The amount of reserved memory for ring data.
    reserved_rings: usize,
}

impl<'compute> Compute<'compute> {
    /// Instantiate.
    pub fn new(dem: &'compute mut crate::dem::DEM, rings_per_km: f32) -> Result<Self> {
        let total_bands = dem.computable_points_count * 2;
        let rings_per_band = Self::ring_count_per_band(rings_per_km, dem.max_line_of_sight);
        let constants = crate::kernel::Constants {
            total_bands,
            max_los_as_points: dem.max_los_as_points,
            dem_width: dem.width,
            tvs_width: dem.tvs_width,
            observer_height: 1.8,
            reserved_rings: u32::try_from(rings_per_band)?,
        };

        let total_reserved_rings = usize::try_from(total_bands)? * rings_per_band;

        Ok(Self {
            dem,
            constants,
            reserved_rings: total_reserved_rings,
        })
    }

    #[expect(
        clippy::as_conversions,
        clippy::cast_precision_loss,
        clippy::cast_possible_truncation,
        clippy::cast_sign_loss,
        reason = "Accuracy isn't needed, we're just calculating a value to help find minimum RAM usage."
    )]
    /// Calculate the expected number of rings per band of sight.
    fn ring_count_per_band(rings_per_km: f32, max_line_of_sight: u32) -> usize {
        let meters_per_km = 1000.0;
        let band_length_in_km = (max_line_of_sight as f32) / meters_per_km;
        (band_length_in_km * rings_per_km) as usize
    }

    /// Do all computations.
    pub fn run(&mut self) -> Result<(Vec<f32>, Vec<Vec<u32>>)> {
        let mut cumulative_surfaces = vec![0.0; usize::try_from(self.dem.computable_points_count)?];
        let mut all_ring_data = Vec::new();
        for angle in 0..180 {
            let mut sector_ring_data = vec![0; self.reserved_rings];
            self.compute_sector(angle, &mut cumulative_surfaces, &mut sector_ring_data)?;
            all_ring_data.push(sector_ring_data.clone());
        }

        Ok((cumulative_surfaces, all_ring_data))
    }

    /// Compute a single sector.
    fn compute_sector(
        &mut self,
        angle: u8,
        cumulative_surfaces: &mut [f32],
        ring_data: &mut [u32],
    ) -> Result<()> {
        tracing::debug!("Calculating bands for sector: {angle}°");
        self.dem.calculate_axes(f32::from(angle))?;

        tracing::debug!("Calculating band deltas for {angle}°");
        self.dem.compile_band_data()?;

        tracing::debug!("Running kernel for {angle}°");
        for kernel_id in 0..self.constants.total_bands {
            crate::kernel::kernel(
                kernel_id,
                &self.constants,
                &self.dem.elevations,
                &self.dem.axes.distances,
                &self.dem.compile_band_data()?,
                cumulative_surfaces,
                ring_data,
            );
        }

        Ok(())
    }
}

#[expect(clippy::unreadable_literal, reason = "It's just for the tests")]
#[cfg(test)]
mod test {
    use super::*;

    fn make_dem() -> crate::dem::DEM {
        crate::dem::DEM::new(9, 1.0, 0.001, 3).unwrap()
    }

    fn compute_tvs(dem: &mut crate::dem::DEM, elevations: &[i16]) -> (Vec<f32>, Vec<Vec<u32>>) {
        dem.elevations = elevations.iter().map(|&x| f32::from(x)).collect();
        let mut compute = Compute::new(dem, 5000.0).unwrap();
        compute.run().unwrap()
    }

    fn create_tvs(elevations: &[i16]) -> (Vec<f32>, Vec<Vec<u32>>) {
        let mut dem = make_dem();
        compute_tvs(&mut dem, elevations)
    }

    fn create_viewshed(elevations: &[i16], pov_id: u32) -> Vec<String> {
        let mut dem = make_dem();
        let (_, ring_data) = compute_tvs(&mut dem, elevations);
        crate::output::ascii::OutputASCII::convert(
            &dem,
            pov_id,
            &ring_data,
            crate::compute::Compute::ring_count_per_band(5000.0, 3),
        )
        .unwrap()
    }

    #[rustfmt::skip]
    fn single_peak_dem() -> Vec<i16> {
        vec![
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 1, 1, 1, 1, 1, 1, 0,
            0, 1, 3, 3, 3, 3, 3, 1, 0,
            0, 1, 3, 6, 6, 6, 3, 1, 0,
            0, 1, 3, 6, 9, 6, 3, 1, 0,
            0, 1, 3, 6, 6, 6, 3, 1, 0,
            0, 1, 3, 3, 3, 3, 3, 1, 0,
            0, 1, 1, 1, 1, 1, 1, 1, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0
        ]
    }

    #[rustfmt::skip]
    fn double_peak_dem() -> Vec<i16> {
        vec![
            0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 1, 1, 1, 1, 1, 1, 1, 0,
            0, 1, 3, 3, 3, 3, 3, 3, 4,
            0, 1, 3, 4, 4, 4, 4, 4, 3,
            0, 1, 3, 4, 6, 4, 4, 4, 3,
            0, 1, 3, 4, 4, 4, 5, 5, 3,
            0, 1, 3, 4, 4, 5, 9, 5, 3,
            0, 1, 1, 4, 4, 5, 5, 5, 3,
            0, 0, 4, 1, 3, 3, 3, 3, 3
        ]
    }

    #[test]
    fn single_peak_totals() {
        let (cumulative_surfaces, _) = create_tvs(&single_peak_dem());
        #[rustfmt::skip]
        assert_eq!(
            cumulative_surfaces,
            [
                28.951092, 18.20732, 28.951097,
                18.207321, 35.32013, 18.207323,
                28.951097, 18.207317, 28.951092
            ]
        );
    }

    #[test]
    fn double_peak_totals() {
        let (cumulative_surfaces, _) = create_tvs(&double_peak_dem());
        #[rustfmt::skip]
        assert_eq!(
            cumulative_surfaces,
            [
                30.305563, 27.532042, 27.445095,
                27.366535, 35.86692, 24.969402,
                27.101336, 24.08531, 22.368183
            ]
        );
    }

    // Here we use a simple ASCII representation of the opening and closing of ring
    // sectors:
    //   A '.' can be either inside or outisde a ring sector.
    //   A '+' is the opening and a '-' is the closing.
    //   A '±' represents both an opening and closing (from different sectors).
    //
    // Eg; Given this ASCII art profile of 2 mountain peaks, where the observer is
    // 'X':
    //
    //                     .*`*.
    //                  .*`  |  `*.
    //       .      X.*`     |     `*.
    //    .*`|`*. .*`        |        `*.
    // .*`   |   `  |        |           `*.
    //       |      |        |
    // there would be 2 ring sectors, both opening at the same point but looking
    // in different directions:
    //       |      |        |
    // ......-......+........-..............
    //
    // Or to use 0s and 1s to show the surfaces seen by the observer:
    //
    // 0000001111111111111111100000000000000
    mod viewsheds {
        #[test]
        fn summit() {
            let viewshed = super::create_viewshed(&super::single_peak_dem(), 40);
            #[rustfmt::skip]
            assert_eq!(
                viewshed,
                [
                    ". . . . . . . . .",
                    ". ± ± ± ± ± ± ± .",
                    ". ± ± ± . ± ± ± .",
                    ". ± ± . . . ± ± .",
                    ". ± . . o . . ± .",
                    ". ± ± . . . ± ± .",
                    ". ± ± ± . ± ± ± .",
                    ". ± ± ± ± ± ± ± .",
                    ". . . . . . . . ."]
            );
        }

        #[test]
        fn off_summit() {
            let viewshed = super::create_viewshed(&super::single_peak_dem(), 30);
            #[rustfmt::skip]
            assert_eq!(
                viewshed,
                [
                    "± ± ± ± ± ± ± . .",
                    "± ± ± . ± ± ± . .",
                    "± ± . . ± ± ± . .",
                    "± . . o . . ± . .",
                    "± ± ± . . ± ± . .",
                    "± ± ± . ± ± . . .",
                    "± ± ± ± ± . . . .",
                    ". . . . . . . . .",
                    ". . . . . . . . ."
                ]
            );
        }

        #[test]
        fn double_peak() {
            let viewshed = super::create_viewshed(&super::double_peak_dem(), 30);
            #[rustfmt::skip]
            assert_eq!(
                viewshed,
                [
                    "± ± ± ± ± ± ± . .",
                    "± ± ± . ± ± ± . .",
                    "± ± . . ± ± ± . .",
                    "± . . o . . ± . .",
                    "± ± ± . . ± ± . .",
                    "± ± ± . ± ± . . .",
                    "± ± ± ± ± . ± . .",
                    ". . . . . . . . .",
                    ". . . . . . . . ."
                ]
            );
        }
    }
}
