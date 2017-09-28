NB: This project is currently very much work in progress. My aim is to fully document it and make it as easy to use and contribute to as possible.

# GPU-based Total Viewshed Calculator
Calculates the total visible surface from every point of a given terrain. For example, the viewshed for the summit of a mountain will be large and for a valley it will be small. There are other viewshed calculators, most notably for [Arcgis](http://pro.arcgis.com/en/pro-app/tool-reference/3d-analyst/viewshed.htm<Paste>), but they only calculate single viewsheds for a specific point. The algorithm used here however takes advantage of the computational efficiency gained by calculating *all*, therefore the total, viewsheds for a given region.

![Caltopo example viewshed](https://2.bp.blogspot.com/-wgkILUyFc7g/UZa6igZWQ0I/AAAAAAAAAJc/URPpKFTpVxM/s1600/Screen+shot+2013-05-17+at+3.09.13+PM.png)

This is an example of a single viewshed from the summit of Mt. Diablo in California. It was created by another viewshed tool called [Caltopo](https://caltopo.com).

## Algorithm
This project is based on the work of S. Tabik, A. R. Cervilla, E. Zapata and L.F. Romero from the University of Málaga, in their paper; [Efficient Data Structure and Highly Scalable Algorithm for Total-Viewshed Computation](www.ac.uma.es/~siham/STARS.pdf).

However it deviates significantly by using Band of Sight 'shapes' rather than calculating the coordinates of every single Band. This has massive space improvement and therefore speed improvement. It also reduces the need for a linked list, further improving the simplicity of the algorithm.

## Building

### Requirements
`OpenCL` - there are many providers. Note that OpenCL also supports Intel CPUs.    
`cmake` (only needed by the `gflags` dependency, not for building this project itself).

Then just run;
`make`

For CUDA you may need something like;
```
CFLAGS=-I/usr/local/cuda-8.0/targets/x86_64-linux/include \
LINKER_FLAGS=/usr/local/cuda-8.0/targets/x86_64-linux/lib/libOpenCL.so \
make
```

Make sure everything is working with;    
`make test` or `./etc/bin/tvs -run_benchmarks`

## Usage

Currently your source data needs to be in [the `.bt` format](http://vterrain.org/Implementation/Formats/BT.html). It also needs to have perfectly square dimensions. The dimensions are not automatically detected so you also need to enter them as CLI args.

The first step is do the precomputatiions, they don't need any source data, so only need to be run once if you're computing lots of source data of the same size.

`./etc/bin/tvs -is_precompute -dem_height 1000 -dem_width 1000`

You are then ready to run the main computations;

`./etc/bin/tvs -dem_height 1000 -dem_width 1000 -input_dem_file ./source_data.bt`

If successful you will find a `./etc/output/tvs.png` file. It is a heatmap where each pixel represents the total surface area visible for an observer from that point. Note that this final output is smaller than the source data because it only considers points for which areas smaller than the `-max_line_of_sight` setting totally fit. You will also find a directory at `./etc/output/ring_sectors` with actual indivdiual viewsheds. This is currently very raw binary data, which I won't describe for now. Very soon I hope to output it as [topoJSON](https://github.com/topojson/topojson).

### Current CLI arguments
```
-cl_device (The device to use for computations. Eg; see `clinfo` for
  available devices) type: int32 default: 0
-dem_height (Height of DEM) type: int32 default: 5
-dem_scale (Size of each point in the DEM in meters) type: double
  default: 30
-dem_width (Width of DEM) type: int32 default: 5
-etc_dir (General output path for caches and completed results.)
  type: string default: "./etc"
-gpu_batches (The number of batches of data sent to the GPU for a single
  sector.This is currently only relevant for storing the output of ring
  sectors. For example a 2000x2000 DEM with allocated space for 200ring
  sectors per point needs 6.4GB of GPU RAM. By batching into 2then only
  3.2GB of RAM is needed.) type: int32 default: 1
-input_dem_file (File to read DEM data from. Should be binary stream of 2
  byte unsignedintegers.) type: string default: "dem.bt"
-input_dir (General output path for caches and completed results.)
  type: string default: "input"
-is_precompute (Only do pre-computations) type: bool default: false
-log_file (Log path) type: string default: "tvs.log"
-max_line_of_sight (The maximum distance in metres to search for visible
  points.For a TVS calculation to be truly correct, it must have accessto
  all the DEM data around it that may possibly be visible to it.However,
  the further the distances searched the exponentiallygreater the
  computations required. Note that the largestcurrently known line of sight
  int he world is 538km. Defaults toone third of the DEM width.)
  type: int32 default: -1
-observer_height (Distance the observer is from the ground in metres)
  type: double default: 1.5
-output_dir (General output path for precomputation caches and complete
  results) type: string default: "output"
-reserved_ring_space (The maximum number of visible rings expected per band
  of sight.This is a critical value. If it is too low then memory
  corruptionoccurs. Less critically, if it is too high then performance
  islost due to unused RAM. All I know so far is that 50 is enoughfor a
  15km line of sight...) type: int32 default: 50
-ring_sector_dir (Computed results for ring sector defintions. This is
  actually the basisdata for fully reconstructing individual viewsheds)
  type: string default: "ring_sectors"
-run_benchmarks (Run benchmarks) type: bool default: false
-sector_dir (Cache for precomputed sector calculations (everything that can
  be donewithout DEM heights)) type: string default: "sectors"
-sector_shift (Initial angular shift in sector alignment. This avoids DEM
  point aligments.Eg; The first sector without shift looks from A to B, but
  with shift looksfrom A to somehwere between B and C.

  A.  .  .  .  .B
   .  .  .  .  .C
   .  .  .  .  .
   .  .  .  .  .
   .  .  .  .  .

  NB. The same value MUST be applied to both compute and precompute.)
  type: double default: 0.001
-single_sector (Only precompute/compute a single sector angle.
  Useful for running precomputations in parallel. You could alsopotentially
  run computations in parallel as well, however, thatonly makes sense if
  you have multiple GPU devices.) type: int32 default: -1
-total_sectors (Think of a double-sided lighthouse moving this many times
  to make afull 360 degree sweep. The algorithm looks forward and backward
  ineach sector so if moving 1 degree at a time then 180 sectors areneeded)
  type: int32 default: 180
-tvs_results_file (Total viewshed values for every point in DEM)
  type: string default: "tvs.bt"
```

## Code style
Styling follows [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).
It can be automated with [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (included when installing clang). This repo contains a `.clang-format` file defining the rules for Google's style guide (generated with `clang-format -style=google -dump-config > .clang-format`).

## Benchmark
Intel(R) HD Graphics Cherryview (OpenCL)    
portishead-benchmark.bt 168x168   
precompute: ~0.3s   
compute (including ring writing): ~2.87s   

