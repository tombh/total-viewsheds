//! Output data as ASCII. Most likely useful for tests.

#![cfg(test)]
#![expect(clippy::indexing_slicing, reason = "This code is mostly for tests")]

use color_eyre::eyre::{ContextCompat as _, Result};

/// `OutputASCII`
pub struct OutputASCII<'output> {
    /// The DEM used to compute the final data.
    dem: &'output crate::dem::DEM,
    /// Data about the visibility regions of each computed band of sight.
    sector_ring_data: std::slice::Iter<'output, u32>,
    /// Amount of reserved ring data space.
    reserved_ring_size: usize,
    /// The position of the observer for the viewshed we want to reconstruct.
    pov_id: u32,
    /// The reconstructed viewshed.
    viewshed: Vec<String>,
}

impl<'output> OutputASCII<'output> {
    /// Extract and convert a single viewshed from all the ring data for all possible viewsheds.
    pub fn convert(
        dem: &'output crate::dem::DEM,
        pov_id: u32,
        all_ring_data: &'output Vec<Vec<u32>>,
        reserved_ring_size: usize,
    ) -> Result<Vec<String>> {
        let mut outputter = Self {
            dem,
            sector_ring_data: std::slice::Iter::default(),
            reserved_ring_size,
            pov_id,
            viewshed: vec![".".to_owned(); usize::try_from(dem.size)?],
        };

        for sector_ring_data in all_ring_data {
            outputter.sector_ring_data = sector_ring_data.iter();
            outputter.parse_sector()?;
        }

        outputter.viewshed[usize::try_from(pov_id)?] = "o".to_owned();

        Ok(outputter
            .viewshed
            .chunks(usize::try_from(dem.width)?)
            .map(|chunk| chunk.join(" "))
            .collect())
    }

    fn read_next_value(&mut self) -> Result<u32> {
        let value = *self
            .sector_ring_data
            .next()
            .context("Couldn't get next ring in ring data")?;
        Ok(value)
    }

    fn parse_sector(&mut self) -> Result<()> {
        for _ in 0..self.dem.computable_points_count * 2 {
            // We divide by 2 because every ring must have both an opening and a closing.
            let no_of_ring_values = self.read_next_value()?.div_euclid(2);

            // Assume that every DEM point has an opening at the PoV.
            let pov_id = self.read_next_value()?;

            for index in 0..no_of_ring_values {
                let opening = if index == 0 {
                    pov_id
                } else {
                    self.read_next_value()?
                };
                let closing = self.read_next_value()?;

                if pov_id == self.pov_id {
                    self.populate_viewshed(usize::try_from(opening)?, usize::try_from(closing)?);
                }
            }

            let skip = self.reserved_ring_size - ((usize::try_from(no_of_ring_values)?) * 2) - 2;
            self.sector_ring_data.nth(skip);
        }

        Ok(())
    }

    fn populate_viewshed(&mut self, opening: usize, closing: usize) {
        if self.viewshed[opening] == "." {
            self.viewshed[opening] = "+".to_owned();
        } else {
            self.viewshed[opening] = "±".to_owned();
        }

        if self.viewshed[closing] == "." {
            self.viewshed[closing] = "-".to_owned();
        } else {
            self.viewshed[closing] = "±".to_owned();
        }

        if opening == closing {
            self.viewshed[opening] = "±".to_owned();
        }
    }
}
