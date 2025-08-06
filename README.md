# Total Viewshed Calculator
Calculates the total visible surface from every point of a given terrain. For example, the viewshed for the summit of a mountain will be large and for a valley it will be small. There are other viewshed calculators, most notably for [Arcgis](http://pro.arcgis.com/en/pro-app/tool-reference/3d-analyst/viewshed.htm), but they only calculate single viewsheds for a specific point. The algorithm used here however takes advantage of the computational efficiency gained by calculating *all*, therefore the total, viewsheds for a given region.

![Caltopo example viewshed](https://2.bp.blogspot.com/-wgkILUyFc7g/UZa6igZWQ0I/AAAAAAAAAJc/URPpKFTpVxM/s1600/Screen+shot+2013-05-17+at+3.09.13+PM.png)

This is an example of a single viewshed from the summit of Mt. Diablo in California. It was created by another viewshed tool called [Caltopo](https://caltopo.com).

## Algorithm
This project is based on the work of Siham Tabik, Antonio R. Cervilla, Emilio Zapata, Luis F. Romero in their
paper _Efficient Data Structure and Highly Scalable Algorithm for Total-Viewshed Computation_: https://ieeexplore.ieee.org/document/6837455

However it notably improves on it by using Band of Sight 'shapes' rather than calculating the coordinates of every single Band. This has massive space improvement and therefore speed improvement. It also reduces the need for a linked list, further improving the simplicity of the algorithm.

There's also a new paper by the same author from 2021 that seems to focus more on parallelism https://arxiv.org/pdf/2003.02200.

## Usage

You will need a `.hgt` file. They can be downloaded in zip batches from: https://www.viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org3.htm

Example: `RUST_LOG=debug cargo run -- --input N51W002.hgt`

A heatmap of total viewshed surface areas will be saved to `./heatmap.png`. The path can be changed with `--output-dir path/to/dir`. Note that the same image file is updated for every sector angle, so you can watch the details improve as the computatioin progresses.

```
Usage: total-viewsheds [OPTIONS] --input <Path to the DEM file>

Options:
      --input <Path to the DEM file>
          The input DEM file. Currently only `.hgt` files are supported

      --max-line-of-sight <The maximum expected line of sight in meters>
          The maximum distance in metres to search for visible points. For a TVS calculation to be truly correct, it must have access to all the DEM data around it that may possibly be visible to it. However, the further the distances searched the exponentially greater the computations required. Note that the largest currently known line of sight in the world is 538km. Defaults to one third of the DEM width

      --rings-per-km <Expected rings per km>
          Size of each DEM point The maximum number of visible rings expected per km of band of sight. This is the number of times land may appear and disappear for an observer looking out into the distance. The value is used to decide how much memory is reserved for collecting ring data. So if it is too low then the program may panic. If it is too high then performance is lost due to unused RAM

          [default: 5]

      --observer-height <Height of observer in meters>
          The height of the observer in meters

          [default: 1.65]

      --sector-shift <Degrees of offset for each sector>
          Initial angular shift in sector alignment. This avoids DEM point aligments.
          Eg; The first sector without shift looks from A to B, but with shift
          looksfrom A to somehwere between B and C.

          A.  .  .  .  .B
           .  .  .  .  .C
           .  .  .  .  .
           .  .  .  .  .
           .  .  .  .  .

          [default: 0.001]

      --compute <The method of running the kernel>
          Where to run the kernel calculations

          [default: vulkan]

          Possible values:
          - cpu:    Normally on the CPU
          - vulkan: A SPIRV shader run on the GPU via Vulkan
          - cuda:   TBC

      --output-dir <Directory to save output to>
          Path to save the heatmap of the total viewshed surfaces

          [default: ./]

  -h, --help
          Print help (see a summary with '-h')
```

## Building Vulkan shader
* Install `cargo-gpu`: `cargo install --git https://github.com/rust-gpu/cargo-gpu cargo-gpu`
* Compile: `cargo gpu build --shader-crate crates/shader`

Note that the pre-compiled shader already comes with the source code (at `crates/shader/total_viewsheds_kernel.spv`). So you only need to compile the shader if you're developing it.

## License
The license is the same as that used for the original paper's sample code: https://github.com/SihamTabik/Total_Viewshed

## Notes

### View Finder Panoramas by Jonathon de Ferranti
The holy grail of DEM data. He's done all the work to clean ad void fill the DEM data
http://www.viewfinderpanoramas.org

### Open Topography
https://opentopography.org
Seems to be an effort to bring everything together. But doesn't seem to clean up the data like View Finder does.

### SRTM 1 arcsecond data
Apparently this is the highest resolution and most comprehensive DEM data:
https://e4ftl01.cr.usgs.gov/MEASURES/SRTMGL1.003/2000.02.11/SRTMGL1_page_1.html

Map of coverage (80%) and total size of data (35GB)

User guide: https://lpdaac.usgs.gov/documents/179/SRTM_User_Guide_V3.pdf

### GDAL support for the .hgt format
https://gdal.org/drivers/raster/srtmhgt.html

### Claim of Krygystan 538km LoS and cool 3D graphic
https://calgaryvisioncentre.com/news/2017/6/23/tdgft1bsbdlm8496ov7tn73kr0ci1q

### Misc
* https://crates.io/crates/srtm_reader

## Further Reading
* https://en.wikipedia.org/wiki/Long_distance_observations

## TODO
* [ ] Run the kernel on the GPU with `wgpu` and `rust-gpu`/`rust-cuda`.
* [ ] `.png` output to show total viewshed surface area heatmap.
* [ ] Cache pre-computations to disk.
* [ ] Why is computing 0° so much faster than computing 1° and 45° so much faster than 46°??
* [ ] `--rings-per-km` doesn't seem intuitive and doesn't give an informative error when it fails.

