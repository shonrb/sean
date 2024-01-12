use crate::vector::*;
use crate::field::*;
use std::mem;

pub struct FluidSolver<const W: usize, const H: usize> {
    pub velocity:  DiscreteField<Vector2>,
    pub density:    DiscreteField<Vector3>,
    pub pressure:  DiscreteField<f64>,
    pub time:      f64,
    velocity_back: DiscreteField<Vector2>,
    density_back:   DiscreteField<Vector3>,
    compression:   DiscreteField<f64>,
    delta_time:    f64,
    viscosity:     f64
}

impl<const W: usize, const H: usize> FluidSolver<W, H> {
    pub fn new(dt: f64, v: f64) -> Self {
        Self { 
            velocity:        DiscreteField::new(W, H, Vector2::new([0.0, 0.0])),
            density:         DiscreteField::new(W, H, Vector3::new([0.0, 0.0, 0.0])),
            pressure:        DiscreteField::new(W, H, 0.0),
            time:            0.0,
            velocity_back:   DiscreteField::new(W, H, Vector2::new([0.0, 0.0])),
            density_back:    DiscreteField::new(W, H, Vector3::new([0.0, 0.0, 0.0])),
            compression:     DiscreteField::new(W, H, 0.0),
            delta_time:      dt,
            viscosity:       v,
        }
    }

    pub fn insert_force(&mut self, x: usize, y: usize, fx: f64, fy: f64) {
        if x >= 1 && x <= W-2 && y >= 1 && y <= H-2 {
            self.velocity_back[x][y] = Vector2::new([fx, fy]);
        }
    }

    pub fn insert_density(&mut self, x: usize, y: usize, r: f64, g: f64, b: f64) {
        if x >= 1 && x <= W-2 && y >= 1 && y <= H-2 {
            self.density_back[x][y] = Vector3::new([r, g, b]);
        }
    }

    pub fn update(&mut self) {
        self.add_external();
        self.swap_fields();
        Self::diffuse(&mut self.density,  &self.density_back,  self.viscosity, self.delta_time);
        Self::diffuse(&mut self.velocity, &self.velocity_back, self.viscosity, self.delta_time);
        self.project();
        self.swap_fields();
        Self::advect(&self.velocity_back, &self.density_back, &mut self.density, self.delta_time);
        Self::advect(&self.velocity_back, &self.velocity_back, &mut self.velocity, self.delta_time);
        self.project();
        self.velocity_back.set(Vector2::new([0.0, 0.0]));
        self.density_back.set(Vector3::new([0.0, 0.0, 0.0]));
        self.time += self.delta_time;
    }

    fn add_external(&mut self) {
        for i in 1 .. W-1 {
            for j in 1 .. H-1 {
                let v  = self.velocity[i][j];
                let dv = self.velocity_back[i][j];
                self.velocity[i][j] = v + dv;
                let d  = self.density[i][j];
                let dd = self.density_back[i][j];
                self.density[i][j] = d + dd;
            }
        }
    }

    fn swap_fields(&mut self) {
        mem::swap(&mut self.velocity, &mut self.velocity_back);
        mem::swap(&mut self.density, &mut self.density_back);
    }

    fn set_bounds(&mut self) {
        // TODO: Add boundary conditions for solid objects / screen edges
    }

    fn diffuse<T: Quantity>(
        f: &mut DiscreteField<T>, fb: &DiscreteField<T>, mul: f64, dt: f64
    ) {
        let a = mul * dt; 
        let g = 1.0 + 4.0 * a;
        relax(f, fb, a, 1.0, g, 10);
    }

    fn advect<T: Quantity>(
        velocity: &DiscreteField<Vector2>, 
        target_prev: &DiscreteField<T>,
        target: &mut DiscreteField<T>,
        dt: f64
    ) {
        let min = Vector2::new([1.0, 1.0]);
        let max = Vector2::new([(W-2) as f64, (H-2) as f64]);

        for i in 1 .. W-1 {
            for j in 1 .. H-1 {
                let position = Vector2::new([i as f64, j as f64]);
                let velocity = velocity[i][j];
                let previous = position - velocity * dt;
                let bounded  = previous.clamp(min, max);
                let advected = bilerp(&target_prev, bounded);
                target[i][j] = advected;
            }
        }
    }

    fn project(&mut self) {
        // Correct the velocity field to be divergence free. 
        self.solve_pressure();
        self.subtract_pressure_gradient();
    }

    fn solve_pressure(&mut self) {
        // Get the pressure field which results in a 0 divergence
        // result to the equation.
        for i in 1 .. W-1 {
            for j in 1 .. H-1 {
                let dx = self.velocity[i+1][j][0] - self.velocity[i-1][j][0];
                let dy = self.velocity[i][j+1][1] - self.velocity[i][j-1][1];
                let dv = (dx + dy) * 0.5;
                self.compression[i][j] = dv;
                self.pressure[i][j] = 0.0;
            }
        }
        
        relax(&mut self.pressure, &self.compression, 1.0, -1.0, 4.0, 20);
    }

    fn subtract_pressure_gradient(&mut self) {
        for i in 1 .. W-1 {
            for j in 1 .. H-1 {
                let p1 = Vector2::new([
                    self.pressure[i+1][j],
                    self.pressure[i][j+1],
                ]);
                let p2 = Vector2::new([
                    self.pressure[i-1][j],
                    self.pressure[i][j-1],
                ]);
                let gradient = (p1 - p2) * 0.5;
                let velocity = self.velocity[i][j];
                self.velocity[i][j] = velocity - gradient;
            }
        }
    }
}

