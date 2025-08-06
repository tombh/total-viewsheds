//! Total Viewshed Calculator

use clap::Parser as _;
use color_eyre::eyre::Result;
use tracing_subscriber::{layer::SubscriberExt as _, util::SubscriberInitExt as _, Layer as _};

mod axes;
mod band_of_sight;
mod compute;
mod config;
mod dem;
mod gpu;
/// Various ways to output data.
mod output {
    pub mod ascii;
    pub mod png;
}

fn main() -> Result<()> {
    color_eyre::install()?;
    setup_logging()?;

    let mut config = crate::config::Config::parse();

    tracing::info!("Loading DEM data from: {}", config.input.display());
    let tile = srtm_reader::Tile::from_file(config.input.clone()).map_err(|error| {
        color_eyre::eyre::eyre!("Couldn't load {}: {error:?}", config.input.display())
    })?;
    let width = u32::try_from(tile.data.len().isqrt())?;
    let scale = get_scale(&tile);

    #[expect(
        clippy::as_conversions,
        clippy::cast_sign_loss,
        clippy::cast_possible_truncation,
        reason = "Sign loss and truncation aren't relevant"
    )]
    let max_line_of_sight = config
        .max_line_of_sight
        .unwrap_or_else(|| (f64::from(width.div_euclid(3)) * f64::from(scale)) as u32);

    config.max_line_of_sight = Some(max_line_of_sight);

    tracing::info!("Initialising with config: {config:?}");
    tracing::info!(
        "Tile: lat: {}, lon: {}, points: {}",
        tile.latitude,
        tile.longitude,
        tile.data.len(),
    );

    let mut dem = crate::dem::DEM::new(width, scale, max_line_of_sight)?;
    tracing::info!("Converting DEM data to `f32`");
    dem.elevations = tile.data.iter().map(|point| f32::from(*point)).collect();

    tracing::info!("Starting computations");
    let mut compute = crate::compute::Compute::new(
        config.compute,
        Some(config.output_dir),
        &mut dem,
        config.rings_per_km,
    )?;
    compute.run()?;

    Ok(())
}

/// Get the scale of the DEM points in meters.
const fn get_scale(tile: &srtm_reader::Tile) -> f32 {
    match tile.resolution {
        srtm_reader::Resolution::SRTM05 => 5.0,
        srtm_reader::Resolution::SRTM1 => 10.0,
        srtm_reader::Resolution::SRTM3 => 30.0,
    }
}

/// Setup logging.
fn setup_logging() -> Result<()> {
    let filters = tracing_subscriber::EnvFilter::builder()
        .with_default_directive("info".parse()?)
        .from_env_lossy();
    let filter_layer = tracing_subscriber::fmt::layer().with_filter(filters);
    let tracing_setup = tracing_subscriber::registry().with(filter_layer);
    tracing_setup.init();

    Ok(())
}
