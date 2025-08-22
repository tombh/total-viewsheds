use std::mem;
use color_eyre::eyre::{ContextCompat as _, Result};

use std::sync::Arc;
use cudarc::driver::{CudaContext, CudaFunction, CudaSlice, CudaStream, DeviceSlice, DriverError, LaunchConfig};
use cudarc::driver::PushKernelArg;
use cudarc::nvrtc;
use cudarc::runtime::result::{get_mem_info};
use cudarc::runtime::result::device::free;
use total_viewsheds_kernel::kernel::Constants;
use crate::compute::Angle;
use crate::dem::DEM;

pub struct CudaKernel {
    ctx: Arc<CudaContext>,
    stream: Arc<CudaStream>,

    angle_kernel: CudaFunction,
    max_kernel: CudaFunction,
}

const MB: usize = 1_000_000;

impl CudaKernel {
    pub fn new() -> Result<Self> {
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

        Ok(Self {
            ctx,
            stream,
            angle_kernel,
            max_kernel,
        })
    }

    fn calculate_angles(
        &self,
        forwards_deltas: &CudaSlice<i32>,
        backward_deltas: &CudaSlice<i32>,
        distances: &CudaSlice<f32>,
        angle_buf: &CudaSlice<f32>,
        n: usize
    ) -> Result<()> {

        tracing::debug!("Calculating angles for {}", n);

        // TODO: this is extremely tuned for Everest, maybe make this a bit more general?
        let launch_cfg = LaunchConfig {
            block_dim: (1, 1000, 1),
            grid_dim: (n as u32, 6, 1),
            shared_mem_bytes: 0,
        };

        //
        assert_eq!(forwards_deltas.len() % (launch_cfg.block_dim.1 * launch_cfg.grid_dim.1) as usize, 0);
        Ok(())
    }

    fn prefix_max() {}

    pub fn line_of_sight(&self, constants: &Constants, angles: &[Angle], elevations: &[f32], cumulative_surfaces: usize) -> Result<Vec<f32>> {
        let all_angles = struct_of_arrays(angles);

        // allocate all the forward deltas/backward deltas, elevations, and the cumulative
        // surfaces result
        let forward_deltas = self.stream.memcpy_stod(&all_angles.forward_deltas)?;
        let backward_deltas = self.stream.memcpy_stod(&all_angles.backward_deltas)?;
        let distances = self.stream.memcpy_stod(&all_angles.distances)?;

        let elevations = self.stream.memcpy_stod(elevations)?;
        let result = self.stream.alloc_zeros::<f32>(cumulative_surfaces)?;

        // Use the above "overhead" to calculate about how much space we have left
        // on the GPU so that we can use the maximum
        let overhead = forward_deltas.num_bytes() + backward_deltas.num_bytes()
            + distances.num_bytes() + elevations.num_bytes() + result.num_bytes();

        tracing::debug!("data overhead: {}MB", overhead / MB);

        // we'll have |deltas| f32s, all future calculations are in bytes so we need size_of
        let line_size = angles[0].forward_deltas.len() * size_of::<f32>();
        tracing::debug!("line size: {}MB", line_size / 1000);

        let (free_bytes, total) = get_mem_info()?;
        tracing::debug!("{}MB free / {}MB total", (free_bytes-overhead)/MB, total/MB);

        // allocate 7/8ths of the bytes left after reserving our overhead
        // for angle calculations
        let bytes_for_angles = ((free_bytes - overhead) * 7) / 8;

        // figure out the maximum number of lines we could possibly have
        let lines_in_buf = bytes_for_angles / (line_size*2);
        tracing::debug!("number of concurrent lines: {}", bytes_for_angles / line_size);

        let angle_buf_size = lines_in_buf *line_size;

        let angle_buf = self.stream.alloc_zeros::<f32>(angle_buf_size / size_of::<f32>())?;
        let prefix_max = self.stream.alloc_zeros::<f32>(angle_buf_size / size_of::<f32>())?;

        tracing::debug!("leftover memory: {}B", bytes_for_angles-(angle_buf.num_bytes()*2));

        let total_lines = crate::axes::SECTOR_STEPS as usize * constants.total_bands as usize;

        let execution_count = total_lines.div_ceil(lines_in_buf);
        tracing::debug!("total_lines: {}", total_lines);


        for i in 0..execution_count {
            let rest = total_lines-(lines_in_buf*i);
            let n = if rest > lines_in_buf {
                lines_in_buf
            } else {
                rest
            };

            self.calculate_angles(
                &forward_deltas,
                &backward_deltas,
                &distances,
                &angle_buf,
                n
            )?
        }

        Ok(self.stream.memcpy_dtov(&result)?)
    }
}


fn struct_of_arrays(angles: &[Angle]) -> Angle {
    let forward = angles
        .iter()
        .map(|a| a.forward_deltas.clone())
        .flatten()
        .collect::<Vec<_>>();

    let backwards = angles
        .iter()
        .map(|a| a.backward_deltas.clone())
        .flatten()
        .collect::<Vec<_>>();

    let distances = angles
        .iter()
        .map(|a| a.distances.clone())
        .flatten()
        .collect::<Vec<_>>();

    Angle {
        forward_deltas: forward,
        backward_deltas: backwards,
        distances,
    }
}



