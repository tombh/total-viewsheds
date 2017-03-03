
kernel void viewshed(
    global const float *elevations,
    global const float *distances,
    global const int *bands,
    int band_size,
    int max_los_as_points,
    int dem_width,
    int tvs_width,
    int computable_points_count,
    global float *cumulative_surfaces
    ) {

  int dem_id, shifted_id, pov_scaled;
  float elevation_delta;
  float distance_delta;
  bool above, opening, closing;
  float angle;
  int idx = get_global_id(0);
  bool visible = true;
  float max_angle = -2000;
  int band_start = idx * band_size;
  int band_next = band_start + band_size;
  int pov_id = bands[band_start];
  float pov_elevation = elevations[pov_id];
  float pov_distance = distances[pov_id];
  float band_surface = 0;

  for(int band_point = band_start + 1; band_point < band_next; band_point++) {
    dem_id = bands[band_point];
    elevation_delta = pov_elevation - elevations[dem_id];
    distance_delta = fabs(pov_distance - distances[dem_id]);

    // Note the adjustment for curvature of the earth. It is merely a crude
    // approximation using the spherical earth model.
    // TODO: Account for refraction.
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
      // considers adjacent points as 1/60th, points that are about 57 cells
      // away as 1 and points futher away as increasingly over 1.
      // Bear in my mind that due to the parallel nature of the Band of Sight
      // distant points may not even be swept for a given Pov, therefore it
      // is an approximation to overcompansenate the surface area of very
      // distant points.
      // Ultimately the reasoning for such normalisation is not so much to
      // measure true surface areas, but to ensure as much as possible that all
      // PoVs are treated consistently so that overlapping raster map tiles
      // do not have visual artefacts.
      band_surface = band_surface + (distance_delta * 0.0174533);
    }

    if (opening) {
    }

    if (closing) {
    }
  }

  // Shift DEM-based coordinates into TVS coordinates. Recall that TVS values can only be calculated
  // For values away from the edge of the DEM -- thus why the TVS grid is smaller.
  shifted_id =
    (((pov_id / dem_width) - max_los_as_points) * tvs_width)
    + ((pov_id % dem_width) - max_los_as_points);

  // Group all back bands at the end.
  if (idx % 2) {
    shifted_id = shifted_id + computable_points_count;
  }

  cumulative_surfaces[shifted_id] = cumulative_surfaces[shifted_id] + band_surface;
}
