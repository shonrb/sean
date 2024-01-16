use crate::vector::*;
use std::ops::*;

pub trait Quantity<T=Self> 
: Add<T,   Output=T>
+ Sub<T,   Output=T>
+ Mul<T,   Output=T>
+ Div<T,   Output=T>
+ Add<f64, Output=T>
+ Sub<f64, Output=T>
+ Mul<f64, Output=T>
+ Div<f64, Output=T>
+ Copy
{}

pub struct DiscreteField<T: Copy> {
    data: Vec<T>,
    pub width: usize,
    pub height: usize
}

impl<T> Quantity for T where T
: Add<T,   Output=T>
+ Sub<T,   Output=T>
+ Mul<T,   Output=T>
+ Div<T,   Output=T>
+ Add<f64, Output=T>
+ Sub<f64, Output=T>
+ Mul<f64, Output=T>
+ Div<f64, Output=T>
+ Copy
{}

impl<T: Copy> DiscreteField<T> {
    pub fn new(w: usize, h: usize, default: T) -> Self {
        let cap = w * h;
        let mut d = Vec::<T>::with_capacity(cap);
        for _ in 0 .. cap {
            d.push(default);
        }
        Self { data: d, width: w, height: h }
    }

    pub fn set(&mut self, v: T) {
        for y in 0 .. self.height {
            for x in 0 .. self.width {
                self[x][y] = v;
            }
        }
    }
}

impl<T: Copy> IndexMut<usize> for DiscreteField<T> {
    fn index_mut(&mut self, i: usize) -> &mut [T] {
        &mut self.data[i * self.width .. (i+1) * self.width]
    }
}

impl<T: Copy> Index<usize> for DiscreteField<T> {
    type Output = [T];

    fn index(&self, i: usize) -> &[T] {
        &self.data[i * self.width .. (i+1) * self.width]
    }
}

pub fn bilerp<T: Quantity>(field: &DiscreteField<T>, p: Vector2) -> T {
    let x1 = p[0].floor();
    let y1 = p[1].floor();
    let x2 = x1 + 1.0;
    let y2 = y1 + 1.0;

    let grad_x = x2 - p[0];
    let grad_y = y2 - p[1];

    let q11 = field[x1 as usize][y1 as usize];
    let q12 = field[x1 as usize][y2 as usize];
    let q21 = field[x2 as usize][y1 as usize];
    let q22 = field[x2 as usize][y2 as usize];

    let fx1 = q11 * grad_x + q21 * (1.0 - grad_x); 
    let fx2 = q12 * grad_x + q22 * (1.0 - grad_x); 
    let fxy = fx1 * grad_y + fx2 * (1.0 - grad_y);
    fxy
}



