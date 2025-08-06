//! GPU pipeline for running the kernel with Vulkan.
use std::num::NonZeroU64;
use wgpu::util::DeviceExt as _;

use color_eyre::Result;

#[expect(clippy::too_many_lines, reason = "")]
/// Compute a single sector.
// TODO: Don't recreate the entire pipeline for every sector.
pub fn run(
    constants: total_viewsheds_kernel::kernel::Constants,
    elevations: &[f32],
    distances: &[f32],
    band_deltas: &[i32],
    reserved_rings: usize,
) -> Result<Vec<f32>> {
    // We first initialize an wgpu `Instance`, which contains any "global" state wgpu needs.
    //
    // This is what loads the vulkan/dx12/metal/opengl libraries.
    let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor::default());

    // We then create an `Adapter` which represents a physical gpu in the system. It allows
    // us to query information about it and create a `Device` from it.
    //
    // This function is asynchronous in WebGPU, so request_adapter returns a future. On native/webgl
    // the future resolves immediately, so we can block on it without harm.
    let adapter =
        pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions::default()))?;

    // Print out some basic information about the adapter.
    tracing::info!("Running on Adapter: {:#?}", adapter.get_info());

    // Check to see if the adapter supports compute shaders. While WebGPU guarantees support for
    // compute shaders, wgpu supports a wider range of devices through the use of "downlevel" devices.
    let downlevel_capabilities = adapter.get_downlevel_capabilities();
    if !downlevel_capabilities
        .flags
        .contains(wgpu::DownlevelFlags::COMPUTE_SHADERS)
    {
        color_eyre::eyre::bail!("Adapter does not support compute shaders");
    }

    // We then create a `Device` and a `Queue` from the `Adapter`.
    //
    // The `Device` is used to create and manage GPU resources.
    // The `Queue` is a queue used to submit work for the GPU to process.
    let required_limits = wgpu::Limits {
        max_storage_buffers_per_shader_stage: 5, // or whatever you need
        ..wgpu::Limits::default()
    };
    let (device, queue) = pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor {
        label: None,
        required_features: wgpu::Features::empty(),
        required_limits,
        memory_hints: wgpu::MemoryHints::MemoryUsage,
        trace: wgpu::Trace::Off,
    }))?;

    // TODO: embed this if we ever make a proper binary release.
    let module = device.create_shader_module(wgpu::include_spirv!(
        "../../shader/total_viewsheds_kernel.spv"
    ));

    // A buffer for the constants data.
    //
    // `create_buffer_init` is a utility provided by `wgpu::util::DeviceExt` which simplifies creating
    // a buffer with some initial data.
    let input_constants_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: None,
        contents: bytemuck::bytes_of(&constants),
        usage: wgpu::BufferUsages::UNIFORM | wgpu::BufferUsages::COPY_DST,
    });

    // All the elevation data.
    let input_elevations_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: None,
        contents: bytemuck::cast_slice(elevations),
        usage: wgpu::BufferUsages::STORAGE,
    });

    // All the distance data.
    let input_distances_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: None,
        contents: bytemuck::cast_slice(distances),
        usage: wgpu::BufferUsages::STORAGE,
    });

    // All the deltas data.
    let input_deltas_buffer = device.create_buffer_init(&wgpu::util::BufferInitDescriptor {
        label: None,
        contents: bytemuck::cast_slice(band_deltas),
        usage: wgpu::BufferUsages::STORAGE,
    });

    let output_surfaces_size =
        u64::from(constants.total_bands.div_euclid(2)) * u64::try_from(std::mem::size_of::<f32>())?;
    let output_surfaces_buffer = device.create_buffer(&wgpu::BufferDescriptor {
        label: None,
        size: output_surfaces_size,
        usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_SRC,
        mapped_at_creation: false,
    });
    // Finally we create a buffer which can be read by the CPU. This buffer is how we will read
    // the data. We need to use a separate buffer because we need to have a usage of `MAP_READ`,
    // and that usage can only be used with `COPY_DST`.
    let download_surfaces_buffer = device.create_buffer(&wgpu::BufferDescriptor {
        label: None,
        size: output_surfaces_size,
        usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
        mapped_at_creation: false,
    });

    let output_rings_size =
        u64::try_from(reserved_rings)? * u64::try_from(std::mem::size_of::<u32>())?;
    let output_rings_buffer = device.create_buffer(&wgpu::BufferDescriptor {
        label: None,
        size: output_rings_size,
        usage: wgpu::BufferUsages::STORAGE | wgpu::BufferUsages::COPY_SRC,
        mapped_at_creation: false,
    });
    // Finally we create a buffer which can be read by the CPU. This buffer is how we will read
    // the data. We need to use a separate buffer because we need to have a usage of `MAP_READ`,
    // and that usage can only be used with `COPY_DST`.
    let download_rings_buffer = device.create_buffer(&wgpu::BufferDescriptor {
        label: None,
        size: output_rings_size,
        usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
        mapped_at_creation: false,
    });

    let constants_size =
        u64::try_from(std::mem::size_of::<total_viewsheds_kernel::kernel::Constants>())?;
    // A bind group layout describes the types of resources that a bind group can contain. Think
    // of this like a C-style header declaration, ensuring both the pipeline and bind group agree
    // on the types of resources.
    let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
        label: None,
        entries: &[
            // Constants
            wgpu::BindGroupLayoutEntry {
                binding: 0,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Uniform,
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(constants_size),
                    has_dynamic_offset: false,
                },
                count: None,
            },
            // Elevations
            wgpu::BindGroupLayoutEntry {
                binding: 1,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: true },
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(4),
                    has_dynamic_offset: false,
                },
                count: None,
            },
            // Distances
            wgpu::BindGroupLayoutEntry {
                binding: 2,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: true },
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(4),
                    has_dynamic_offset: false,
                },
                count: None,
            },
            // Deltas
            wgpu::BindGroupLayoutEntry {
                binding: 3,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: true },
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(4),
                    has_dynamic_offset: false,
                },
                count: None,
            },
            // Output: surface data
            wgpu::BindGroupLayoutEntry {
                binding: 4,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: false },
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(4),
                    has_dynamic_offset: false,
                },
                count: None,
            },
            // Output: ring data
            wgpu::BindGroupLayoutEntry {
                binding: 5,
                visibility: wgpu::ShaderStages::COMPUTE,
                ty: wgpu::BindingType::Buffer {
                    ty: wgpu::BufferBindingType::Storage { read_only: false },
                    // This is the size of a single element in the buffer.
                    min_binding_size: NonZeroU64::new(4),
                    has_dynamic_offset: false,
                },
                count: None,
            },
        ],
    });

    // The bind group contains the actual resources to bind to the pipeline.
    //
    // Even when the buffers are individually dropped, wgpu will keep the bind group and buffers
    // alive until the bind group itself is dropped.
    let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
        label: None,
        layout: &bind_group_layout,
        entries: &[
            wgpu::BindGroupEntry {
                binding: 0,
                resource: input_constants_buffer.as_entire_binding(),
            },
            wgpu::BindGroupEntry {
                binding: 1,
                resource: input_elevations_buffer.as_entire_binding(),
            },
            wgpu::BindGroupEntry {
                binding: 2,
                resource: input_distances_buffer.as_entire_binding(),
            },
            wgpu::BindGroupEntry {
                binding: 3,
                resource: input_deltas_buffer.as_entire_binding(),
            },
            wgpu::BindGroupEntry {
                binding: 4,
                resource: output_surfaces_buffer.as_entire_binding(),
            },
            wgpu::BindGroupEntry {
                binding: 5,
                resource: output_rings_buffer.as_entire_binding(),
            },
        ],
    });

    // The pipeline layout describes the bind groups that a pipeline expects
    let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
        label: None,
        bind_group_layouts: &[&bind_group_layout],
        push_constant_ranges: &[],
    });

    // The pipeline is the ready-to-go program state for the GPU. It contains the shader modules,
    // the interfaces (bind group layouts) and the shader entry point.
    let pipeline = device.create_compute_pipeline(&wgpu::ComputePipelineDescriptor {
        label: None,
        layout: Some(&pipeline_layout),
        module: &module,
        entry_point: Some("main"),
        compilation_options: wgpu::PipelineCompilationOptions::default(),
        cache: None,
    });

    // The command encoder allows us to record commands that we will later submit to the GPU.
    let mut encoder =
        device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });

    // A compute pass is a single series of compute operations. While we are recording a compute
    // pass, we cannot record to the encoder.
    let mut compute_pass = encoder.begin_compute_pass(&wgpu::ComputePassDescriptor {
        label: None,
        timestamp_writes: None,
    });

    // Set the pipeline that we want to use
    compute_pass.set_pipeline(&pipeline);
    // Set the bind group that we want to use
    compute_pass.set_bind_group(0, &bind_group, &[]);

    // Now we dispatch a series of workgroups. Each workgroup is a 3D grid of individual programs.
    //
    // We defined the workgroup size in the shader as 64x1x1. So in order to process all of our
    // inputs, we ceiling divide the number of inputs by 64. If the user passes 32 inputs, we will
    // dispatch 1 workgroups. If the user passes 65 inputs, we will dispatch 2 workgroups, etc.
    let workgroup_count = (constants.total_bands * 2).div_ceil(64);
    compute_pass.dispatch_workgroups(workgroup_count, 1, 1);

    // Now we drop the compute pass, giving us access to the encoder again.
    drop(compute_pass);

    // We add a copy operation to the encoder. This will copy the data from the output buffer on the
    // GPU to the download buffer on the CPU.
    encoder.copy_buffer_to_buffer(
        &output_surfaces_buffer,
        0,
        &download_surfaces_buffer,
        0,
        output_surfaces_size,
    );

    encoder.copy_buffer_to_buffer(
        &output_rings_buffer,
        0,
        &download_rings_buffer,
        0,
        output_rings_size,
    );

    // We finish the encoder, giving us a fully recorded command buffer.
    let command_buffer = encoder.finish();

    // At this point nothing has actually been executed on the gpu. We have recorded a series of
    // commands that we want to execute, but they haven't been sent to the gpu yet.
    //
    // Submitting to the queue sends the command buffer to the gpu. The gpu will then execute the
    // commands in the command buffer in order.
    queue.submit([command_buffer]);

    // We now map the download buffer so we can read it. Mapping tells wgpu that we want to read/write
    // to the buffer directly by the CPU and it should not permit any more GPU operations on the buffer.
    //
    // Mapping requires that the GPU be finished using the buffer before it resolves, so mapping has a callback
    // to tell you when the mapping is complete.
    let buffer_slice = download_surfaces_buffer.slice(..);
    buffer_slice.map_async(wgpu::MapMode::Read, |_| {
        // In this case we know exactly when the mapping will be finished,
        // so we don't need to do anything in the callback.
    });

    // Wait for the GPU to finish working on the submitted work. This doesn't work on WebGPU, so we would need
    // to rely on the callback to know when the buffer is mapped.
    device.poll(wgpu::PollType::Wait)?;

    // We can now read the data from the buffer.
    let data = buffer_slice.get_mapped_range();
    // Convert the data back to a slice of f32.
    let result: &[f32] = bytemuck::cast_slice(&data);

    Ok(result.to_vec())
}
