use std::ops::*;

pub type Vector2 = Vector<2>;
pub type Vector3 = Vector<3>;

#[derive(Clone, Copy)]
pub struct Vector<const D: usize> {
    pub v: [f64; D]
}

impl<const D: usize> Vector<D> {
    pub fn new(v: [f64; D]) -> Self {
        Self { v: v }
    }

    pub fn sum(self) -> f64 {
        self.v.iter().sum()
    }

    pub fn dot(self, other: Self) -> f64 {
        (self * other).sum()
    }

    pub fn magnitude2(self) -> f64 {
        self.dot(self)
    }

    pub fn magnitude(self) -> f64 {
        self.magnitude2().sqrt()
    }

    pub fn normal(self) -> Self {
        let m = self.magnitude();
        if m == 0.0 {
            self
        } else {
            self / m
        }
    }

    pub fn clamp(self, min: Self, max: Self) -> Self {
        let mut x = [0.0; D];
        for i in 0 .. D {
            x[i] = self[i].clamp(min[i], max[i]);
        }
        Self::new(x)
    }
}

impl Vector<2> {
    pub fn wedge(self, other: Self) -> f64 {
        self[0]*other[1] - self[1]*other[0]
    }
}

impl Vector<3> {
    pub fn cross(self, other: Self) -> Self {
        Self::new([
            self[1]*other[2] - self[2]*other[1],
            self[2]*other[0] - self[0]*other[2],
            self[0]*other[1] - self[1]*other[0]
        ])
    }
}

impl<const D: usize> IndexMut<usize> for Vector<D> {
    fn index_mut(&mut self, i: usize) -> &mut f64 {
        &mut self.v[i]
    }
}

impl<const D: usize> Index<usize> for Vector<D> {
    type Output = f64;
    
    fn index(&self, i: usize) -> &f64 {
        &self.v[i]
    }
}


macro_rules! define_operator {
($tname: tt, $fname: tt, $op: tt) => {
    impl<const D: usize> $tname <Vector<D>> for Vector<D> {
        type Output = Self;
        fn $fname (self, other: Self) -> Self {
            let mut x: [f64; D] = [0.0; D];
            for i in 0 .. D {
                x[i] = self[i] $op other[i];
            }
            Self::new(x)
        }
    }

    impl<const D: usize> $tname <f64> for Vector<D> {
        type Output = Self;
        fn $fname (self, other: f64) -> Self {
            let mut x: [f64; D] = [0.0; D];
            for i in 0 .. D {
                x[i] = self[i] $op other;
            }
            Self::new(x)
        }
    }
}}

macro_rules! define_op_assign {
($tname: tt, $fname: tt, $op: tt) => {
    impl<const D: usize> $tname for Vector<D> {
        fn $fname (&mut self, other: Self) {
            *self = *self $op other;
        }
    }

    impl<const D: usize> $tname <f64> for Vector<D> {
        fn $fname (&mut self, other: f64) {
            *self = *self $op other;
        }
    }
}}

define_operator!{Add, add, +}
define_operator!{Sub, sub, -}
define_operator!{Mul, mul, *}
define_operator!{Div, div, /}

define_op_assign!{AddAssign, add_assign, +}
define_op_assign!{SubAssign, sub_assign, -}
define_op_assign!{MulAssign, mul_assign, *}
define_op_assign!{DivAssign, div_assign, /}

