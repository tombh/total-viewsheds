//! Defines all the CLI arguments.

/// `Config`
#[derive(clap::Parser)]
pub struct Config {
    /// The maximum number of visible rings expected per km of band of sight. This is the number
    /// of times land may appear and disappear for an observer looking out into the distance. The
    /// value is used to decide how much memory is reserved for collecting ring data. So if it is
    /// too low then the program may panic. If it is too high then performance is lost due to
    /// unused RAM.
    #[arg(long, default_value_t = 3.3333333)]
    pub rings_per_km: f32,
}
