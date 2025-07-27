//! Calculate DEM properties based on a new set of axes.
//!
//! It's easy to imagine the following DEM points based around an
//! intuitive x/y horizontally/vertically aligned axis. But let's
//! overlay another axis (ab/cd) rotated at an angle, namely the
//! so-called "sector angle". Here the sector angle is 45 degrees:
//!
//!       y
//!       ^
//!       |
//!     a |                         d
//!       *  .  .  .  .  .  .  .  *
//!       .  *  .  .  .  .  .  *  .
//!       .  .  *  .  .  .  *  .  .
//!       .  .  .  *  .  *  .  .  .
//!       .  .  .  .  *  .  .  .  .
//!       .  .  .  *  .  *  .  .  .
//!       .  .  *  .  .  .  *  .  .
//!       .  *  .  .  .  .  .  *  .
//!       *  .  .  .  .  .  .  .  * -----> x
//!     c                           b
//!
//! Roughly speaking, the band of sight always runs parallel to the 'ab'
//! axis and we use the 'cd' axis as a datum to measure relative distances inside the
//! band of sight.
//!
//! TODO: This code is surprisingly fast, even without GPU parallelisation. However,
//! it's a perfect candidate for parallelisation.

pub struct Axes {
    width: u32,
    angle: f64,
    distances: Vec<f64>,
    sight_ordered: Vec<u64>,
    sector_ordered: Vec<u64>,
}

impl Axes {
    pub fn new(width: u32, angle: f64, shift_angle: f64) -> Self {
        let total_points = u64::from(width).pow(2);
        let sight_ordered: Vec<u64> = (0..total_points).collect();
        let sector_ordered = sight_ordered.clone();
        Self {
            width,
            angle: angle + shift_angle,
            distances: Vec::new(),
            sight_ordered,
            sector_ordered,
        }
    }

    pub fn compute(&mut self) {
        self.distances = self.calculate_distances(self.angle);
        self.sight_ordered = self.order_by_distance(&self.distances);

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
    fn calculate_distances(&mut self, angle: f64) -> Vec<f64> {
        let mut distances = Vec::<f64>::new();
        let sine_of_angle = angle.to_radians().sin();
        let cosine_of_angle = angle.to_radians().cos();

        let range = (-(self.width as i32 - 1)..=0).rev();
        for y in range {
            for x in 0..self.width {
                let left = (x as f64) * sine_of_angle;
                let right = (y as f64) * cosine_of_angle;
                let distance = left - right;
                distances.push(distance);
            }
        }

        distances
    }

    fn order_by_distance(&self, distances: &[f64]) -> Vec<u64> {
        let mut ordered: Vec<u64> = (0..self.width.pow(2) as u64).collect();
        ordered.sort_by(|&i, &j| {
            let left = distances[i as usize];
            let right = distances[j as usize];
            left.partial_cmp(&right).unwrap()
        });

        ordered
    }
}

#[cfg(test)]
mod test {
    use super::*;

    fn calculate_axes_at_angle(angle: f64) -> Axes {
        let shift_angle = 0.001;
        let mut axes = Axes::new(5, angle, shift_angle);
        axes.compute();
        axes
    }

    #[test]
    fn at_0_degrees() {
        // Zero degrees still gets a 'shift angle' applied to
        // prevent point alignments.
        let axes = calculate_axes_at_angle(0.0);

        #[rustfmt::skip]
        assert_eq!(
            axes.distances,
            [
                0.0, 1.7453292519057202e-5, 3.4906585038114403e-5, 5.23598775571716e-5, 6.981317007622881e-5,
                0.9999999998476913, 1.0000174531402104, 1.0000349064327294, 1.0000523597252484, 1.0000698130177674,
                1.9999999996953826, 2.0000174529879016, 2.000034906280421, 2.0000523595729396, 2.000069812865459,
                2.999999999543074, 3.000017452835593, 3.000034906128112, 3.000052359420631, 3.00006981271315,
                3.999999999390765, 4.000017452683284, 4.000034905975803, 4.000052359268322, 4.000069812560842
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
            axes.sight_ordered,
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
               0.0, 0.7070944397373546, 1.4141888794747093, 2.121283319212064, 2.8283777589494186,
               -0.7071191224203434, -2.468268298871923e-5, 0.7070697570543659, 1.4141641967917207, 2.1212586365290753,
               -1.4142382448406867, -0.7071438051033321, -4.936536597743846e-5, 0.7070450743713772, 1.4141395141087318,
               -2.12135736726103, -1.4142629275236756, -0.7071684877863209, -7.404804896626871e-5, 0.7070203916883884,
               -2.8284764896813734, -2.121382049944019, -1.4142876102066642, -0.7071931704693095, -9.873073195487692e-5 
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
            axes.sight_ordered,
            [
                20, 21, 15, 22, 16,
                10, 23, 17, 11, 5,
                24, 18, 12, 6,  0,
                19, 13, 7,  1, 14,
                8,  2,  9,  3, 4
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
