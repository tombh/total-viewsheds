// Ensure that the first point from the point of view is always visible
#define MAX_ANGLE -2000.0

// Normalisation constant: due to repeated exposure of closer points and
// infrequent exposure of points further away. See main loop for more explanation.
#define TAN_ONE_RAD 0.0174533

// So that some points are not visible simply by virtue of the earth's spherical
// shape.
#define EARTH_RADIUS_SQUARED 12742000.0


kernel void viewshed(
  // Every single DEM point's elevation
  global const short *elevations,
  // Every single DEM point's distance from the sector-orthogonal axis
  global const float *distances,
  // Deltas used to build a band. Deltas are always the same!
  global const int *band_deltas,
  // Array for final TVS values. Usually 1/8th the size of DEM.
  global float *cumulative_surfaces,
  // Array for keeping track of visible chunks called Ring Sectors.
  global int *ring_data,
  // Most GPUs can't handle the volume of data in 1 go, so we need to
  // send and therefore keep track of batched data.
  const int batch_start
) {

  int tvs_id, band_direction, debug, delta;
  float elevation_delta, distance_delta, angle;
  bool opening, closing, is_currently_visible, is_previously_visible;

  float max_angle = MAX_ANGLE;

  // Keep track of the amount of the earth visible from this particular band
  float band_surface = 0.0;

  // For each visible continuous region we find, we'll note it as a single ring
  // sector. We need to know how many ring sectors we find in order to save that
  // data to file later. Start at 1 to reserve later writing of the number of ring
  // sectors found.
  int ring_id = 1;

  // A globally unique identifier for the band we're currently calculating.
  // Note that for each computable DEM point, there'll be 2 bands: front and back.
  int idx = batch_start + get_global_id(0);

  // Translate an idx to a TVS ID
  if (idx < (TOTAL_BANDS / 2)) {
    tvs_id = idx;
    band_direction = 1;
  } else {
    tvs_id = idx - (TOTAL_BANDS / 2);
    band_direction = -1;
  }

  // Translate a TVS ID to a DEM ID.
  int pov_id = (((tvs_id / TVS_WIDTH) + MAX_LOS_AS_POINTS) * DEM_WIDTH)
               + ((tvs_id % TVS_WIDTH) + MAX_LOS_AS_POINTS);

  // The PoV is involved in every calculation, so do now and save for later.
  float pov_elevation = elevations[pov_id] + OBSERVER_HEIGHT;
  float pov_distance = distances[pov_id];

  // This thread needs its own unshared space in global memory.
  int ring_data_start = idx * RESERVED_RINGS;

  // We already know that the first point from the PoV is always visible and
  // is therefore the opening of a visible region.
  ring_data[ring_data_start + ring_id] = pov_id;
  is_previously_visible = true;

  // The DEM ID will change as we loop, but the PoV won't. For now we need the PoV
  // ID to start the reconstruction of a unique band from the band delta template.
  int dem_id = pov_id;

  // The kernel's kernel. The most critical code of all.
  // Passing the band size as a compiler argument allows for optimisation through
  // loop unrolling.
  for(int i = 0; i < BAND_DELTAS_SIZE; i++) {

    // Deltas are simply the numerical difference between DEM IDs in a band of
    // sight. DEM IDs are certainly different for every point in a DEM, but
    // conveniently the *differences* between DEM IDs are identical. With the only
    // caveat that back-facing bands have opposite magnitudes. It should be stressed
    // that this feature of band data is a huge benefit to the space-requirements
    // (and thus speed) of the algorithm.
    // TODO: Explore:
    //         * storing the band deltas in local memory.
    //         * compressing repeating delta values -- is simple addition faster than
    //           memory accesses?
    delta = band_deltas[i];

    // Derive the new DEM ID. Note that band_direction can only be +/- 1 depending
    // on whether this is a front- or back-facing band.
    dem_id += band_direction * delta;

    // Pull the actual data needed to make a visibility calculation from global memory.
    elevation_delta = elevations[dem_id] - pov_elevation;
    distance_delta = fabs(distances[dem_id] - pov_distance);

    // TODO: Is there a better way to do this? Perhaps by ensuring this
    //       never happens in the first place?
    if (distance_delta == INFINITY) distance_delta = 0.0;
    if (elevation_delta == INFINITY) elevation_delta = 0.0;

    // The actual visibility calculation.
    // Note the adjustment for curvature of the earth. It is merely a crude
    // approximation using the spherical earth model.
    // TODO: Account for refraction.
    // TODO: Is there a performance gain to be had from only checking for an
    //       increase in elevation as a trigger for the full angle calculation?
    angle = (elevation_delta / distance_delta) - (distance_delta / EARTH_RADIUS_SQUARED);

    //                            5              |-
    //                        4 .-`-. 6          |-
    //            1       3  .-`     `-.  7      |-
    //   o    0 .-`-. 2   .-`           `-.      |- Elevation deltas
    //   /\  .-`     `-.-`                 `-.8  |-
    //    |                                      |-
    //    |---|---|---|---|---|---|---|---|---|
    //    |       Distance deltas
    //    |
    //   PoV  --------> direction of band point iterations
    //
    // Notice how only points 1, 4 and 5 increase the angle between the viewer and
    // the land and therefore can be considered visible.
    is_currently_visible = angle > max_angle;

    // Here we consider the *previous* visibility to decide whether this is the
    // beginning or ending of a visible region.
    opening = is_currently_visible && !is_previously_visible;
    closing = is_previously_visible && !is_currently_visible;

    if (is_currently_visible) {
      // This normalises surfaces due to repeated visibility over sectors.
      // For example points near the PoV may be counted 180 times giving
      // an unrealistic aggregation of total surfaces. This trig operation
      // (0.017 is tan(1 degree) where 1 degree is the size of each sector)
      // considers adjacent points as 1/57th, points that are about 57 cells
      // away as 1 and points futher away as increasingly over 1.
      // Bear in my mind that due to the parallel nature of the band of sight,
      // distant points may not even be swept for a given PoV, therefore it
      // is an approximation to over-compansenate the surface area of very
      // distant points.
      //
      // Ultimately the reasoning for such normalisation is not so much to
      // measure true surface areas, but to ensure as much as possible that all
      // PoVs are treated consistently so that overlapping TVS raster map tiles
      // do not have visual artefacts.
      //
      // TODO: Can this be refactored into a single calculation at the closing
      //       of a ring sector?
      band_surface += (distance_delta * TAN_ONE_RAD);
    }

    // Store the position on the DEM where the visible region starts.
    if (opening) {
      ring_id++;
      ring_data[ring_data_start + ring_id] = dem_id;
    }

    // Store the position on the DEM where the visible region ends.
    if (closing) {
      ring_id++;
      ring_data[ring_data_start + ring_id] = dem_id;
    }

    // Prepare for the next iteration.
    is_previously_visible = is_currently_visible;
    max_angle = fmax(angle, max_angle);
  }

  // Close any ring sectors prematurely cut off by a restricted line of sight.
  if (is_currently_visible && !closing) {
    ring_id++;
    ring_data[ring_data_start + ring_id] = dem_id;
  }

  // Make a note at the start of the ring sector data of how many rings we found.
  ring_data[ring_data_start] = ring_id;

  // Accumulate surfaces for a given DEM ID. Note that this is thread safe, because
  // front/back bands are kept in separate array positions. And that actual
  // cumulation additions are made on data from a *previous* run where a completely
  // different sector angle was calculated.
  cumulative_surfaces[idx] += band_surface;
}
