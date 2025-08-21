

use color_eyre::eyre::{ContextCompat as _, Result};

use std::sync::Arc;
use cudarc::driver::{CudaContext, CudaFunction, CudaSlice, CudaStream, DriverError, LaunchConfig};
use cudarc::driver::PushKernelArg;
use cudarc::nvrtc;
use total_viewsheds_kernel::kernel::Constants;
use crate::dem::DEM;

pub struct CudaKernel {
    ctx: Arc<CudaContext>,
    stream: Arc<CudaStream>,

    angle_kernel: CudaFunction,
    max_kernel: CudaFunction,
    angles: CudaSlice<f32>
}



impl CudaKernel {
    pub fn new(bands: usize, deltas: usize) -> Result<Self> {
        let ctx = CudaContext::new(0)?;

        let angle_kernel: CudaFunction;
        let max_kernel: CudaFunction;

        {
            let kernel = nvrtc::compile_ptx(include_str!("angles.cu"))?;
            let module = ctx.load_module(kernel)?;
            angle_kernel = module.load_function("angle_kernel")?;
        }

        {
            let kernel = nvrtc::compile_ptx(include_str!("max.cu"))?;
            let module = ctx.load_module(kernel)?;
            max_kernel = module.load_function("max_kernel")?;
        }

        // JIT the kernel

        let stream = ctx.default_stream();

        let angles = stream.alloc_zeros::<f32>(bands*deltas)?;

        Ok(Self {
            ctx,
            stream,
            angle_kernel,
            max_kernel,
            angles,
        })
    }

    fn calculate_angles(&mut self, constants: &Constants, dem: &DEM, sums: &[i32], differences: &[i32]) -> Result<()> {
        let mut launcher = self.stream.launch_builder(&self.angle_kernel);

        let elevations = self.stream.memcpy_stod(&dem.elevations)?;
        let distances = self.stream.memcpy_stod(&dem.axes.distances)?;
        let deltas = self.stream.memcpy_stod(sums)?;
        let differences = self.stream.memcpy_stod(differences)?;

        // launcher.arg(constants);
        launcher.arg(&elevations);
        launcher.arg(&distances);
        launcher.arg(&deltas);
        launcher.arg(&differences);
        launcher.arg(&mut self.angles);

        let num_grids = constants.total_bands/2;

        let launch_config = LaunchConfig{
            block_dim: (10, 100, 1),
            grid_dim: (num_grids/10, 4, 1),
            shared_mem_bytes: 0,
        };

        unsafe {
            launcher.launch(launch_config)
        }?;

        Ok(())
    }


    pub fn line_of_sight(&mut self, constants: &Constants, dem: &DEM, sums: &[i32], differences: &[i32]) -> Result<Vec<i32>> {
        self.calculate_angles(constants, dem, sums, differences)?;

        let result = self.stream.alloc_zeros::<i32>(constants.total_bands as usize)?;
        let mut launcher = self.stream.launch_builder(&self.max_kernel);
        // launcher.arg(constants);
        launcher.arg(&self.angles);
        launcher.arg(&result);

        let cfg = LaunchConfig{
            block_dim: (1, 1, 1),
            grid_dim: (constants.total_bands, 1, 1),
            shared_mem_bytes: 0,
        };

        unsafe {
            launcher.launch(cfg)
        }?;

        let res = self.stream.memcpy_dtov(&result)?;
        println!("{:?}", &res[1..10]);

        Ok(res)
    }
}