# Total Viewshed Calculator
Calculates the total visible surface from every point of a given terrain.

## Original Idea
The main algorithm is based on the work of S. Tabik, A. R. Cervilla, E. Zapata and L. F. Romero from
*Arquitectura De Los Computadores at The University Of Malaga* in their paper: "Efficient Data Structure and Highly Scalable Algorithm for Total-Viewshed Computation".

## Code style
Styling follows [Google's C++ style guide](https://google.github.io/styleguide/cppguide.html)
It can be automated with [`clang-format`](https://clang.llvm.org/docs/ClangFormat.html) (included when installing clang). This repo contains a `.clang-format` file defining the rules for Google's style guide (generated with `clang-format -style=google -dump-config > .clang-format`).

#Benchmark
Intel(R) Celeron(R) CPU  N3060  @ 1.60GHz, 1903 MB
portishead-benchmark.bt 168x168
precompute: 2.45s
compute: 21.65s (is_store_ring_sectors)

