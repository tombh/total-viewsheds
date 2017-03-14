
kernel void viewshed(
  global const float *elevations,
  global const float *distances,
  global const int *bands,
  const int max_los_as_points,
  const int dem_width,
  const int tvs_width,
  const int computable_points_count,
  const int reserved_rings,
  global float *cumulative_surfaces,
  global int *sector_rings
) {

  int dem_id, shifted_id, pov_scaled, band_point;
  float elevation_delta, distance_delta, angle;
  bool above, opening, closing;

  int idx = get_global_id(0);
  bool visible = true;
  float max_angle = -2000;
  int band_start = idx * BAND_SIZE;
  int band_next = band_start + BAND_SIZE;
  int pov_id = bands[band_start];
  float pov_elevation = elevations[pov_id];
  float pov_distance = distances[pov_id];
  float band_surface = 0;
  int sector_rings_start = idx * reserved_rings; // Keep an eye on this
  int ring_id = 1;
  sector_rings[sector_rings_start + 1] = pov_id; // PoV is always visible

  // TODO: There could well be a performance gain by running this in another
  // dimension and using an inclusive max scan on the angles.
  for(int i = 1; i < BAND_SIZE; i++) {
    band_point = band_start + i;
    dem_id = bands[band_point];
    elevation_delta = pov_elevation - elevations[dem_id];
    distance_delta = fabs(pov_distance - distances[dem_id]);

    // Note the adjustment for curvature of the earth. It is merely a crude
    // approximation using the spherical earth model.
    // TODO: Account for refraction.
    // TODO: Is there a performance gain to be had from only checking for an
    // increase in elevation as a trigger for the full angle calculation?
    angle = (elevation_delta / distance_delta) - (distance_delta / 12742000);

    above = angle > max_angle;
    max_angle = max(angle, max_angle);

    opening = above && (!visible);
    closing = (!above) && visible;
    visible = above;

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
      band_surface = band_surface + (distance_delta * 0.0174533);
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

  // Shift DEM-based coordinates into TVS coordinates. Recall that TVS values can only be calculated
  // for values away from the edge of the DEM -- thus why the TVS grid is smaller.
  // TODO: Imply the TVS grid ordering in the ordering of the bands sent here. This can be done once
  // during precompute.
  shifted_id =
    (((pov_id / dem_width) - max_los_as_points) * tvs_width)
    + ((pov_id % dem_width) - max_los_as_points);
  // Group all back bands at the end.
  if (idx % 2) {
    shifted_id = shifted_id + computable_points_count;
  }

  cumulative_surfaces[shifted_id] = cumulative_surfaces[shifted_id] + band_surface;
}
