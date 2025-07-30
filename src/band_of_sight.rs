impl crate::dem::DEM {
    /// These deltas apply to *all* possible bands in this sector.
    pub fn compile_band_data(&self) -> std::vec::Vec<i64> {
        let mut dem_ids_to_compute = Vec::new();
        let mut distance_ids = Vec::new();

        let band_deltas_size = self.band_size - 1;
        let mut band_deltas = vec![0; band_deltas_size as usize];

        // Increasing the band size is effectively like interpolating. The wider the band
        // the more points have the ability to block or count towards visibility for any
        // given line of sight. Adding a couple more points ensures that the band isn't
        // so thin that the line of sight 'slips' between DEM points, effectively
        // interpolating nothing.
        let band_samples = (self.band_size * 2) + 2;

        // Find the very middle of the DEM. This is arbitrary as we ultimately only care about
        // deltas.
        let pov_id = self.size / 2;

        // We want the PoV to be in the middle of the band, both horizontally and
        // vertically.
        let band_edge = pov_id - (band_samples / 2);

        // Fill the band with the points before the PoV. This stage just gives *which* points are
        // in the band of sight, but not in the correct order.
        for i in 0..band_samples {
            let dem_id = self.axes.sector_ordered[(band_edge + i) as usize];
            dem_ids_to_compute.push(dem_id);
            // Make a note of the distance IDs (how far each point is from a line orthogonal to
            // the sector ordering).
            let distance_id = self.axes.sight_ordered_map[dem_id as usize];
            distance_ids.push(distance_id);
        }

        // These IDs map the distance IDs to the DEM IDs. Let's say that the DEM IDs we need to
        // compute are:
        //   `31, 22, 46`
        // Then we look up those IDs on the sight-ordered map and we get another set of IDs.
        // Note that there is no direct intuition for what these IDs are, they just represent how
        // the DEM IDs are ordered based on another set of axes. So let's just say they are:
        //   `83, 91, 57`
        // Now because this second set is connected to the first set, any operation that we do on
        // the second can be equally applied to the first. So the smallest ID in the second set
        // is `57` which is in the third position, etc. And so the map to order the second set from
        // smallest to largest is:
        //   `2, 0, 1`
        // That map can then be used to order the first set. That is what we're doing here.
        let mut distances_to_sector_ids_map: Vec<usize> = (0..band_samples as usize).collect();
        distances_to_sector_ids_map.sort_by(|&i, &j| {
            let left = distance_ids[i];
            let right = distance_ids[j];
            left.partial_cmp(&right).unwrap()
        });

        // Fill the actual band of sight with the deltas that trace a path from the PoV
        // to the final point in the band.
        let middle_of_whole_band = (band_samples / 2) as usize;
        let end_of_band = (self.band_size * 2) as usize;
        for (i, band_id) in (middle_of_whole_band..end_of_band).enumerate() {
            let current = dem_ids_to_compute[distances_to_sector_ids_map[band_id]] as i64;
            let next = dem_ids_to_compute[distances_to_sector_ids_map[band_id + 1]] as i64;
            band_deltas[i] = current - next;
        }

        band_deltas
    }
}

// This is a little map to help orient yourself when looking at the test results:
//
// 0  1  2  3  4  5  6  7  8
// 9  10 11 12 13 14 15 16 17
// 18 19 20 21 22 23 24 25 26
// 27 28 29 30 31 32 33 34 35
// 36 37 38 39 40 41 42 43 44
// 45 46 47 48 49 50 51 52 53
// 54 55 56 57 58 59 60 61 62
// 63 64 65 66 67 68 69 70 71
// 72 73 74 75 76 77 78 79 80
#[cfg(test)]
mod test {
    /// Reconstruct bands from deltas.
    fn reconstruct_bands(angle: f64) -> Vec<Vec<u64>> {
        let mut dem = crate::dem::DEM::new(9, 1.0, 0.001, 3);
        assert_eq!(dem.computable_points_count, 9);
        dem.calculate_axes(angle);

        let mut both_bands = Vec::new();
        let band_deltas = dem.compile_band_data();

        for point in 0..dem.computable_points_count {
            let mut front_band = Vec::new();
            let mut back_band = Vec::new();
            let mut front_dem_id = dem.tvs_id_to_pov_id(point);
            let mut back_dem_id = front_dem_id;
            front_band.push(front_dem_id);
            back_band.push(back_dem_id);
            for delta in &band_deltas {
                front_dem_id = (front_dem_id as i64 + delta) as u64;
                back_dem_id = (back_dem_id as i64 - delta) as u64;
                front_band.push(front_dem_id);
                back_band.push(back_dem_id);
            }

            // The end of the back band should be the same as the start as the front band. So
            // it makes sense to push the back band before the front one.
            back_band.reverse();
            both_bands.push(back_band.clone());
            both_bands.push(front_band.clone());
        }

        both_bands
    }

    #[test]
    fn sector_at_0_degrees() {
        let bands = reconstruct_bands(0.0);
        #[rustfmt::skip]
        assert_eq!(
            bands,
            [
                [57, 48, 39, 30], [30, 21, 12, 3],
                [58, 49, 40, 31], [31, 22, 13, 4],
                [59, 50, 41, 32], [32, 23, 14, 5],
                [66, 57, 48, 39], [39, 30, 21, 12],
                [67, 58, 49, 40], [40, 31, 22, 13],
                [68, 59, 50, 41], [41, 32, 23, 14],
                [75, 66, 57, 48], [48, 39, 30, 21],
                [76, 67, 58, 49], [49, 40, 31, 22],
                [77, 68, 59, 50], [50, 41, 32, 23]
            ]
        );
    }

    #[test]
    fn sector_at_15_degrees() {
        let bands = reconstruct_bands(15.0);
        #[rustfmt::skip]
        assert_eq!(
            bands,
            [
                [49, 40, 39, 30], [30, 21, 20, 11],
                [50, 41, 40, 31], [31, 22, 21, 12],
                [51, 42, 41, 32], [32, 23, 22, 13],
                [58, 49, 48, 39], [39, 30, 29, 20],
                [59, 50, 49, 40], [40, 31, 30, 21],
                [60, 51, 50, 41], [41, 32, 31, 22],
                [67, 58, 57, 48], [48, 39, 38, 29],
                [68, 59, 58, 49], [49, 40, 39, 30],
                [69, 60, 59, 50], [50, 41, 40, 31]
            ]
        );
    }

    #[test]
    fn sector_at_90_degrees() {
        let bands = reconstruct_bands(90.0);
        #[rustfmt::skip]
        assert_eq!(
            bands,
            [
                [33, 32, 31, 30], [30, 29, 28, 27],
                [34, 33, 32, 31], [31, 30, 29, 28],
                [35, 34, 33, 32], [32, 31, 30, 29],
                [42, 41, 40, 39], [39, 38, 37, 36],
                [43, 42, 41, 40], [40, 39, 38, 37],
                [44, 43, 42, 41], [41, 40, 39, 38],
                [51, 50, 49, 48], [48, 47, 46, 45],
                [52, 51, 50, 49], [49, 48, 47, 46],
                [53, 52, 51, 50], [50, 49, 48, 47]
            ]
        );
    }

    #[test]
    fn sector_at_135_degrees() {
        let bands = reconstruct_bands(135.0);
        #[rustfmt::skip]
        assert_eq!(
            bands,
            [
                [6,  14, 22, 30], [30, 38, 46, 54],
                [7,  15, 23, 31], [31, 39, 47, 55],
                [8,  16, 24, 32], [32, 40, 48, 56],
                [15, 23, 31, 39], [39, 47, 55, 63],
                [16, 24, 32, 40], [40, 48, 56, 64],
                [17, 25, 33, 41], [41, 49, 57, 65],
                [24, 32, 40, 48], [48, 56, 64, 72],
                [25, 33, 41, 49], [49, 57, 65, 73],
                [26, 34, 42, 50], [50, 58, 66, 74]
            ]
        );
    }
}
