//! Defines all the CLI arguments.

/// `Config`
#[derive(clap::Parser, Debug)]
pub struct Config {
    /// The input DEM file. Currently only `.hgt` files are supported.
    #[arg(long, value_name = "Path to the DEM file")]
    pub input: std::path::PathBuf,
    /// The maximum distance in metres to search for visible points. For a TVS calculation to be
    /// truly correct, it must have access to all the DEM data around it that may possibly be
    /// visible to it. However, the further the distances searched the exponentially greater the
    /// computations required. Note that the largest currently known line of sight in the world
    /// is 538km. Defaults to one third of the DEM width.
    #[arg(long, value_name = "The maximum expected line of sight in meters")]
    pub max_line_of_sight: Option<u32>,
    /// Size of each DEM point
    /// The maximum number of visible rings expected per km of band of sight. This is the number
    /// of times land may appear and disappear for an observer looking out into the distance. The
    /// value is used to decide how much memory is reserved for collecting ring data. So if it is
    /// too low then the program may panic. If it is too high then performance is lost due to
    /// unused RAM.
    #[arg(long, value_name = "Expected rings per km", default_value_t = 5.0)]
    pub rings_per_km: f32,
    /// The height of the observer in meters.
    #[arg(
        long,
        value_name = "Height of observer in meters",
        default_value = "1.65"
    )]
    pub observer_height: f32,
    /// Initial angular shift in sector alignment. This avoids DEM point aligments.
    /// Eg; The first sector without shift looks from A to B, but with shift
    /// looksfrom A to somehwere between B and C.
    ///
    /// A.  .  .  .  .B
    ///  .  .  .  .  .C
    ///  .  .  .  .  .
    ///  .  .  .  .  .
    ///  .  .  .  .  .
    #[arg(
        long,
        verbatim_doc_comment,
        value_name = "Degrees of offset for each sector",
        default_value = "0.001"
    )]
    pub sector_shift: f32,
    /// Where to run the kernel calculations.
    #[arg(
        long,
        value_enum,
        value_name = "The method of running the kernel",
        default_value_t = ComputeType::Vulkan
    )]
    pub compute: ComputeType,
    /// Path to save the heatmap of the total viewshed surfaces.
    #[arg(long, value_name = "Directory to save output to", default_value = "./")]
    pub output_dir: std::path::PathBuf,
}

#[derive(clap::ValueEnum, Clone, Debug)]
pub enum ComputeType {
    /// Conventional CPU computations. The slowest method.
    CPU,
    /// A SPIRV shader run on the GPU via Vulkan.
    Vulkan,
    /// TBC
    Cuda,
}
