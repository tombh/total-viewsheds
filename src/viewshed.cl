
kernel void viewshed(
  global const float *elevations,
  global const float *distances,
  global const int *bands,
  const int max_los_as_points,
  const float observer_height,
  const int dem_width,
  const int tvs_width,
  const int computable_points_count,
  const int reserved_rings,
  global float *cumulative_surfaces,
  global int *sector_rings
) {

  int dem_id, shifted_id, pov_scaled, band_point;
  float elevation_delta, distance_delta, angle;
  bool above, opening, closing, visible;

  int idx = get_global_id(0);
  float max_angle = -2000.0;
  float tan_one_rad = 0.0174533;
  float earth_radius_squared = 12742000.0;
  int band_start = idx * BAND_SIZE;
  int band_next = band_start + BAND_SIZE;
  int pov_id = bands[band_start];
  float pov_elevation = elevations[pov_id] + observer_height;
  float pov_distance = distances[pov_id];
  float band_surface = 0.0;
  int sector_rings_start = idx * reserved_rings;
  int ring_id = 1;
  sector_rings[sector_rings_start + 1] = pov_id; // PoV is always visible

  // TODO: There could well be a performance gain by running this in another
  // dimension, removing all branches and using an inclusive max scan on the angles.
  for(int i = 1; i < BAND_SIZE; i++) {
    dem_id = bands[band_start + i];
    elevation_delta = elevations[dem_id] - pov_elevation;
    distance_delta = fabs(distances[dem_id] - pov_distance);

    // TODO: Is there a better way to do this? Perhaps by ensuring this
    // never happens in the first place?
    if (distance_delta == INFINITY) distance_delta = 0.0;
    if (elevation_delta == INFINITY) elevation_delta = 0.0;

    // Note the adjustment for curvature of the earth. It is merely a crude
    // approximation using the spherical earth model.
    // TODO: Account for refraction.
    // TODO: Is there a performance gain to be had from only checking for an
    // increase in elevation as a trigger for the full angle calculation?
    angle = (elevation_delta / distance_delta) - (distance_delta / earth_radius_squared);

    above = angle > max_angle;
    opening = above && (!visible);
    closing = (!above) && visible;
    visible = above;
    max_angle = fmax(angle, max_angle);

    if (visible) {
      // This normalises surfaces due to repeated visibility over sectors.
      // For example points near the PoV may be counted 180 times giving
      // an unrealistic aggregation of total surfaces. This trig operation
      // (0.017 is tan(1 degree) where 1 degree is the size of each sector)
      // considers adjacent points as 1/57th, points that are about 57 cells
      // away as 1 and points futher away as increasingly over 1.
      // Bear in my mind that due to the parallel nature of the Band of Sight,
      // distant points may not even be swept for a given PoV, therefore it
      // is an approximation to overcompansenate the surface area of very
      // distant points.
      // Ultimately the reasoning for such normalisation is not so much to
      // measure true surface areas, but to ensure as much as possible that all
      // PoVs are treated consistently so that overlapping raster map tiles
      // do not have visual artefacts.
      // TODO: Can this be refactored into a single calculation at the closing
      // of a ring sector?
      band_surface = band_surface + (distance_delta * tan_one_rad);
    }

    if (opening) {
      ring_id++;
      sector_rings[sector_rings_start + ring_id] = dem_id;
    }

    if (closing) {
      ring_id++;
      sector_rings[sector_rings_start + ring_id] = dem_id;
    }
  }

  // Close any ring sectors prematurely cut off by a restricted line of sight.
  if (visible && !closing) {
    ring_id++;
    sector_rings[sector_rings_start + ring_id] = dem_id;
  }

  // Make a note of how many rings we found
  sector_rings[sector_rings_start] = ring_id;

  cumulative_surfaces[idx] = cumulative_surfaces[idx] + band_surface;
}
