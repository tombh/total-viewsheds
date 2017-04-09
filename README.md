NB: This project is currently very much work in progress. My aim is to fully document it and make it as easy to use and contribute to as possible.

# Total Viewshed Calculator
Calculates the total visible surface from every point of a given terrain. For example, the viewshed for the summit of a mountain will be large and for a valley it will be small. There are other viewshed calculators, most notably for [Arcgis](http://pro.arcgis.com/en/pro-app/tool-reference/3d-analyst/viewshed.htm<Paste>), but they only calculate single viewsheds for a specific point. The algorithm used here however takes advantage of the computational efficiency gained by calculating *all*, therefore the total, viewsheds for a given region.

![Caltopo example viewshed](https://2.bp.blogspot.com/-wgkILUyFc7g/UZa6igZWQ0I/AAAAAAAAAJc/URPpKFTpVxM/s1600/Screen+shot+2013-05-17+at+3.09.13+PM.png)

This is an example of a single viewshed from the summit of Mt. Diablo in California. It was created by another viewshed tool called [Caltopo](https://caltopo.com).

## Algorithm
This project is based on the work of S. Tabik, A. R. Cervilla, E. Zapata and L.F. Romero from the University of Málaga, in their paper; [Efficient Data Structure and Highly Scalable Algorithm for Total-Viewshed Computation](www.ac.uma.es/~siham/STARS.pdf).

However it deviates significantly by using Band of Sight 'shapes' rather than calculating the coordinates of every single Band. This has massive space improvement and therefore speed improvement. It also reduces the need for a linked list, further improving the simplicity of the algorithm.

## Code style
Styling follows [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html).
It can be automated with [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (included when installing clang). This repo contains a `.clang-format` file defining the rules for Google's style guide (generated with `clang-format -style=google -dump-config > .clang-format`).

## Building
OpenCL, there are many providers.
Gflags requires `cmake`.

Then just run;
`make`

For CUDA you may need something like;
```
CFLAGS=-I/usr/local/cuda-8.0/targets/x86_64-linux/include \
LINKER_FLAGS=/usr/local/cuda-8.0/targets/x86_64-linux/lib/libOpenCL.so \
make
```

## Benchmark
Intel(R) HD Graphics Cherryview (OpenCL)    
portishead-benchmark.bt 168x168   
precompute: ~0.3s   
compute (including ring writing): ~2.87s   

