//! Total Viewshed Calculator

use clap::Parser as _;
use color_eyre::eyre::Result;

mod axes;
mod band_of_sight;
mod compute;
mod config;
mod dem;
mod kernel;
/// Various ways to output data.
mod output {
    pub mod ascii;
}

fn main() -> Result<()> {
    let config = crate::config::Config::parse();
    let mut dem = crate::dem::DEM::new(1000, 1.000, 0.001, 30);
    let mut compute = crate::compute::Compute::new(&mut dem, config.rings_per_km)?;
    compute.run()?;
    Ok(())
}
