//! Calculate DEM properties based on a new set of axes.
//!
//! It's easy to imagine the following DEM points based around an conventional x/y
//! horizontally/vertically aligned axis. But let's overlay another axis (ab/cd) rotated at an
//! angle, namely the so-called "sector angle". Here the sector angle is 45 degrees:
//!
//!       y
//!       ^
//!       |
//!     a |                 d
//!       * . . . . . . . *
//!       . * . . . . . * .
//!       . . * . . . * . .
//!       . . . * . * . . .
//!       . . . . * . . . .
//!       . . . * . * . . .
//!       . . * . . . * . .
//!       . * . . . . . * .
//!       * . . . . . . . * ----> x
//!     c                   b
//!
//! Roughly speaking, the band of sight always runs parallel to the 'ab' axis and we use the 'cd'
//! axis as a datum to measure relative distances inside the band of sight.
//!
//! TODO:
//!   * Now that we use band delta shapes, can we get away with just computing for the points in
//!     one band?
//!   * This code is surprisingly fast, even without GPU parallelisation. However, it's a
//!     perfect candidate for parallelisation.

use color_eyre::Result;

/// The number of steps we sweep through to reach 180°.
// TODO: make configurable?
pub const SECTOR_STEPS: u16 = 180;

/// Initial angular shift in sector alignment. This avoids DEM point aligments.
/// Eg; The first sector without shift looks from A to B, but with shift looks from A to somehwere
/// between B and C:
///
/// A. . . . .B
///  . . . . .C
///  . . . . .
///  . . . . .
///  . . . . .
///  
pub const SECTOR_SHIFT: f32 = 0.001;

/// `Axes`
#[derive(Default)]
pub struct Axes {
    /// The width of the DEM.
    width: u32,
    /// The angle for which we are calculating the axes.
    pub angle: f32,
    /// The distance of each DEM point from the base of the sector angle. Although exactly where
    /// the base line lies is not that important, as long it has the correct angle, then positive
    /// and negative distances relative to it are fine too..
    pub distances: Vec<f32>,
    /// DEM IDs ordered by their distance from the sector axis.
    pub sector_ordered: Vec<u32>,
    /// This orders the DEM points by their distances from an axis perpendicular to the sector axis.
    /// But it doesn't contain the DEM IDs themselves, you give it a DEM ID and it tells you what
    /// position it is in the ordered list.
    pub sight_ordered_map: Vec<usize>,
}

impl Axes {
    /// Instantiate.
    pub fn new(width: u32, angle: f32) -> Result<Self> {
        let total_points = width.pow(2);
        let sight_ordered_map: Vec<usize> = (0..usize::try_from(total_points)?).collect();
        let sector_ordered: Vec<u32> = (0..total_points).collect();
        Ok(Self {
            width,
            angle: angle + SECTOR_SHIFT,
            distances: Vec::new(),
            sight_ordered_map,
            sector_ordered,
        })
    }

    /// Do the main computations.
    pub fn compute(&mut self) {
        tracing::debug!("Calculating axes for {}°", self.angle);

        let distances = self.calculate_distances(self.angle);
        let sight_distances = self.order_by_distance(&distances);
        self.convert_distances_to_f32(&distances);

        #[expect(
            clippy::as_conversions,
            clippy::indexing_slicing,
            reason = "
              We're swapping exactly the same range of numbers, they just have different types
            "
        )]
        for (index, dem_id) in sight_distances.iter().enumerate() {
            let dem_id_usize = *dem_id as usize;
            self.sight_ordered_map[dem_id_usize] = index;
        }

        let sector_distances = self.calculate_distances(self.angle + 90.0);
        self.sector_ordered = self.order_by_distance(&sector_distances);
    }

    /// Calculate distances of elevantion samples from a line like `ab`:
    ///
    ///                   -----b
    ///            ------/
    ///      -----/+-----+\q1
    /// a---/ p1\  | DEM | \
    ///          \ |     |  \
    ///           \+-----+   \
    ///            \          \q2
    ///           p2\
    ///
    /// The distances we're calculating are `p1p2`, `q1q2`, etc. Except that the points are inside
    /// the DEM.
    ///
    /// [0, 0] is at the top-left of the DEM.
    ///
    /// The equation for the shortest distance between a point `[x, y]` and a line
    /// with angle `θ` that passes exactly through the origin at `[0, 0]` is:
    ///   `x * sinθ − y * cosθ`
    ///
    /// Note that for certain angles either the sight-based line or the sector-based line can pass
    /// inside the DEM.
    fn calculate_distances(&self, angle: f32) -> Vec<f64> {
        let angle_f64 = f64::from(angle);
        let mut distances = Vec::<f64>::new();
        let sine_of_angle = angle_f64.to_radians().sin();
        let cosine_of_angle = angle_f64.to_radians().cos();

        let range = (-(i64::from(self.width) - 1)..=0).rev();
        for y in range {
            for x in 0..self.width {
                let left = f64::from(x) * sine_of_angle;
                #[expect(
                    clippy::as_conversions,
                    clippy::cast_precision_loss,
                    reason = "We only ever use the i64 up to the negative size of u32"
                )]
                let right = y as f64 * cosine_of_angle;
                let distance = left - right;
                distances.push(distance);
            }
        }

        distances
    }

    /// Order by distance, but don't reorder the original data. Instead return the new indexes of
    /// the data if it were ordered.
    fn order_by_distance(&self, distances: &[f64]) -> Vec<u32> {
        let mut ordered: Vec<u32> = (0..self.width.pow(2)).collect();
        #[expect(
            clippy::indexing_slicing,
            clippy::as_conversions,
            reason = "We're sorting 2 vectors of the same size so out of bounds is impossible"
        )]
        ordered.sort_by(|&i, &j| {
            let left = distances[i as usize];
            let right = distances[j as usize];
            left.partial_cmp(&right)
                .unwrap_or(std::cmp::Ordering::Equal)
        });

        ordered
    }

    #[expect(
        clippy::as_conversions,
        clippy::cast_possible_truncation,
        reason = "
          We do as much of the cacheable pre-computations using `f64`. But do the most intensive
          calculations using `f32` for efficieny.
        "
    )]
    /// Convert high precisions distance calculations to `f32` for use on GPUs.
    fn convert_distances_to_f32(&mut self, distances: &[f64]) {
        self.distances = distances.iter().map(|&x| x as f32).collect();
    }
}

#[expect(
    clippy::unreadable_literal,
    clippy::as_conversions,
    clippy::indexing_slicing,
    reason = "It's just for the tests"
)]
#[cfg(test)]
mod test {
    use super::*;

    fn calculate_axes_at_angle(angle: f32) -> Axes {
        let mut axes = Axes::new(5, angle).unwrap();
        axes.compute();
        axes
    }

    fn extract_from_mapped(map: &[usize]) -> Vec<u64> {
        let mut unmapped = vec![0; map.len()];
        for (index, id) in map.iter().enumerate() {
            unmapped[*id] = index as u64;
        }
        unmapped
    }

    #[test]
    fn at_0_degrees() {
        // To prevent point alignments 0 degrees still gets a 'shift angle'
        // applied.
        let axes = calculate_axes_at_angle(0.0);

        #[rustfmt::skip]
        assert_eq!(
            axes.distances,
            [
                0.0, 1.7453292e-5, 3.4906585e-5, 5.235988e-5, 6.981317e-5,
                1.0, 1.0000174,    1.0000349,    1.0000523,   1.0000699,
                2.0, 2.0000174,    2.0000348,    2.0000525,   2.0000699,
                3.0, 3.0000174,    3.0000348,    3.0000525,   3.0000699,
                4.0, 4.0000176,    4.000035,     4.0000525,   4.0000696
            ]
        );

        // Ordering of elevation samples from an axis a bit like
        //
        // ........~~~~~~~~```
        //     +-----+
        //     |     |
        //     |     |
        //     +-----+
        //
        #[rustfmt::skip]
        assert_eq!(
            extract_from_mapped(&axes.sight_ordered_map),
            [
                0,  1,  2,  3,  4,
                5,  6,  7,  8,  9,
                10, 11, 12, 13, 14,
                15, 16, 17, 18, 19,
                20, 21, 22, 23, 24
            ]
        );

        // Ordering of elevation samples from an axis a bit like:
        //
        //  |
        //  |  +-----+
        //   | |     |
        //   | |     |
        //    |+-----+
        //    |
        //
        #[rustfmt::skip]
        assert_eq!(
            axes.sector_ordered,
            [
                20, 15, 10, 5, 0,
                21, 16, 11, 6, 1,
                22, 17, 12, 7, 2,
                23, 18, 13, 8, 3,
                24, 19, 14, 9, 4,
            ]
        );
    }

    #[test]
    fn at_135_degrees() {
        let axes = calculate_axes_at_angle(135.0);

        #[rustfmt::skip]
        assert_eq!(
            axes.distances,
            [
                0.0,        0.7070944,     1.4141887,     2.121283,      2.8283775,
                -0.7071192, -2.4857438e-5, 0.7070695,     1.4141638,     2.1212583,
                -1.4142385, -0.7071441,    -4.9714876e-5, 0.70704466,    1.414139,
                -2.1213577, -1.4142632,    -0.70716894,   -7.4572315e-5, 0.7070198,
                -2.828477,  -2.1213825,    -1.4142882,    -0.7071938,    -9.942975e-5
            ]
        );

        // Ordering of elevation samples from an axis a bit like:
        //
        //        +-----+
        //        |     |
        //  `~.   |     |
        //     `~.+-----+
        //        `~.
        //           `~.
        //              `~.
        //
        #[rustfmt::skip]
        assert_eq!(
            extract_from_mapped(&axes.sight_ordered_map),
            [
                20, 21, 15, 22, 16,
                10, 23, 17, 11, 5,
                24, 18, 12, 6,  0,
                19, 13, 7,  1,  14,
                8,  2,  9,  3,  4
            ]
        );

        // Ordering of elevation samples from an axis a bit like:
        //
        //   +-----+
        //   |     |       .~`
        //   |     |    .~`
        //   +-----+ .~`
        //        .~`
        //
        #[rustfmt::skip]
        assert_eq!(
            axes.sector_ordered,
            [
                24, 19, 23, 14, 18,
                22, 9,  13, 17, 21,
                4,  8,  12, 16, 20,
                3,  7,  11, 15, 2,
                6,  10, 1,  5,  0
            ]
        );
    }
}
