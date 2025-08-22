

typedef struct {
    unsigned int total_bands;
    unsigned int max_los_as_points;
    unsigned int dem_width;
    unsigned int tvs_width;
    float observer_height;
    unsigned int reserved_rings;
} calculation_constants;

#define EARTH_RADIUS_SQUARED 12742000.0

extern "C" __global__ void angle_kernel(
    // Constants for the calculations.
    calculation_constants constants,
    // Every single DEM point's elevation.
    const float* __restrict__ elevations,
    // Every single DEM point's distance from the sector-orthogonal axis.
    const float* __restrict__ distances,
    // Deltas used to build a band. Deltas are always the same for both front and back bands.
    //
    // Deltas are simply the numerical difference between DEM IDs in a band of
    // sight. DEM IDs are certainly different for every point in a DEM, but
    // conveniently the _differences_ between DEM IDs are identical. With the only
    // caveat that back-facing bands have opposite magnitudes. It should be stressed
    // that this feature of band data is a huge benefit to the space-requirements
    // (and thus speed) of the algorithm.
    // TODO: Explore:
    //         * storing the band deltas in local memory.
    //         * compressing repeating delta values -- is simple addition faster than
    //           memory accesses?
    const unsigned int* __restrict__ delta_pos,
    const unsigned int* __restrict__ delta_neg,
    float* result
) {
    int half_total_bands = constants.total_bands/2;

    int tvs_id = ((blockDim.x*blockIdx.x)+threadIdx.x) % half_total_bands;

    bool forward = tvs_id < half_total_bands;

    int pov_x = (tvs_id % constants.tvs_width) + constants.max_los_as_points;
    int pov_y = (tvs_id / constants.tvs_width) + constants.max_los_as_points;

    // get the dem id for our pov which is where we start our calculation
    int pov_id = (pov_x * constants.dem_width) + pov_y;

    // calculate he height
    float pov_elevation = elevations[pov_id] + constants.observer_height;
    float pov_distance = distances[pov_id];

    int delta = forward ? delta_pos[threadIdx.y]
                        : -delta_neg[threadIdx.y];

    int dem_id = pov_id + delta;

    float elevation_delta = elevations[dem_id] - pov_elevation;
    float distance_delta = fabs(distances[dem_id] - pov_distance);

    int absolute_pov_idx = blockDim.x * blockIdx.x * blockDim.y;
    int index = blockDim.x * blockIdx.x * blockDim.y
        + threadIdx.y * blockDim.x + threadIdx.x;

    result[index] = (elevation_delta/distance_delta);
}
