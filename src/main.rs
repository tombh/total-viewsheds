//! Total Viewshed Calculator

use color_eyre::eyre::Result;

mod axes;
mod band_of_sight;
mod dem;

fn main() -> Result<()> {
    let mut dem = crate::dem::DEM::new(10000, 1.0, 0.001, 100)?;
    dem.calculate_axes(0.0)?;
    dem.compile_band_data()?;
    Ok(())
}
