mod axes;
mod dem;

fn main() {
    let shift_angle = 0.001;
    let mut axes = crate::axes::Axes::new(20000, 0.0, shift_angle);
    axes.compute();
}
