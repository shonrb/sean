pub mod vector;
pub mod field;
pub mod solver;

use solver::*;
use vector::Vector3;
use field::DiscreteField;

use sfml::graphics::{
    RenderTarget, 
    RenderWindow, 
    Vertex, 
    PrimitiveType, 
    RenderStates,
    Color
};

use sfml::window::{
    Event, 
    Style, 
    ContextSettings,
    Key,
    mouse::Button
};

const WINDOW_WIDTH:  usize      = 900;
const WINDOW_HEIGHT: usize      = 900;
const FLUID_WIDTH:   usize      = 100;
const FLUID_HEIGHT:  usize      = 100;
const TIME_STEP:     f64        = 1.5;
const VISCOSITY:     f64        = 0.15;
const SCALE:         (f32, f32) = (
    WINDOW_WIDTH  as f32 / FLUID_WIDTH  as f32,
    WINDOW_HEIGHT as f32 / FLUID_HEIGHT as f32,
);

type SFVec2 = sfml::system::Vector2f;

enum RenderType {
    Velocity,
    Pressure,
    Density
}

struct Main {
    window:      RenderWindow,
    solver:      FluidSolver<FLUID_WIDTH, FLUID_HEIGHT>,
    render:      RenderType,
    mouse:       (i32, i32),
    last_mouse:  (i32, i32),
    left_click:  bool,
    right_click: bool,
    paused:      bool
}

impl Main {
    pub fn init() -> Self {
        let mut window = RenderWindow::new(
            (WINDOW_WIDTH as u32, WINDOW_HEIGHT as u32), 
            "Stable Fluids",
            Style::CLOSE,
            &ContextSettings::default()
        );
        window.set_vertical_sync_enabled(true);

        let solver = FluidSolver
            ::<FLUID_WIDTH, FLUID_HEIGHT>
            ::new(TIME_STEP, VISCOSITY);
        
        Self {
            window:      window,
            solver:      solver,
            render:      RenderType::Density,
            mouse:       (0, 0),
            last_mouse:  (0, 0),
            left_click:  false,
            right_click: false,
            paused:      false
        }
    }

    pub fn run(&mut self) {
        while self.window.is_open() {
            self.handle_events();
            if !self.paused {
                if self.left_click {
                    self.paint();
                }
                self.solver.update();
                self.render();
                self.window.display();
            }
        }
    }

    fn handle_events(&mut self) {
        while let Some(event) = self.window.poll_event() {
            match event {
                Event::Closed => self.window.close(),
                Event::MouseButtonPressed  { button: Button::Left, .. } 
                    => self.left_click = true,
                Event::MouseButtonReleased { button: Button::Left, .. } 
                    => self.left_click = false,
                Event::MouseButtonPressed  { button: Button::Right, .. } 
                    => self.right_click = true,
                Event::MouseButtonReleased { button: Button::Right, .. } 
                    => self.right_click = false,
                Event::MouseMoved { x, y } => {
                    self.last_mouse = self.mouse;
                    self.mouse = (x, y);
                },
                Event::KeyPressed { code, .. } => {
                    match code {
                        Key::Space => self.paused = !self.paused,
                        Key::Num1 => self.render = RenderType::Density,
                        Key::Num2 => self.render = RenderType::Velocity,
                        Key::Num3 => self.render = RenderType::Pressure,
                        _ => {}
                    }
                },
                _ => {}
            }
        }
    }

    fn paint(&mut self) {
        const TIME_MULTIPLIER: f64 = 0.01;
        const MOUSE_FORCE_MULTIPLIER: f64 = 5.0;
        const BRUSH_RADIUS: i32 = 1;
        let (mx, my)  = Self::screen_to_world(self.mouse);
        let (lx, ly)  = Self::screen_to_world(self.last_mouse);
        let force_x   = (mx - lx) * MOUSE_FORCE_MULTIPLIER;
        let force_y   = (my - ly) * MOUSE_FORCE_MULTIPLIER;
        let from_x    = mx as i32 - BRUSH_RADIUS;
        let from_y    = my as i32 - BRUSH_RADIUS;
        let to_x      = mx as i32 + BRUSH_RADIUS;
        let to_y      = my as i32 + BRUSH_RADIUS;
        let t         = self.solver.time * TIME_MULTIPLIER;
        let (r, g, b) = Self::colour_gradient(t);
        for x in from_x ..= to_x {
            for y in from_y ..= to_y {
                let xx = x as usize;
                let yy = y as usize;
                self.solver.insert_force(xx, yy, force_x, force_y);
                self.solver.insert_density(xx, yy, r, g, b);
            }
        }
    }

    fn render(&mut self) {
        self.window.clear(Color::BLACK);

        match self.render {
            RenderType::Density  => self.render_density(),
            RenderType::Velocity => self.render_velocity(),
            RenderType::Pressure => self.render_pressure(),
        }
    }
    
    fn render_velocity(&mut self) {
        const VECTOR_LENGTH_MULTIPLIER: f64 = 12.5;

        for x in 0 .. FLUID_WIDTH {
            for y in 0 .. FLUID_HEIGHT {                
                let v = self.solver.velocity[x][y];
                let (x1, y1) = Self::world_to_screen((
                    x as f64 + 0.5, 
                    y as f64 + 0.5
                ));

                let (vx, vy) = Self::world_to_screen((v[0], v[1]));
                let x2 = x1 + vx * VECTOR_LENGTH_MULTIPLIER;
                let y2 = y1 + vy * VECTOR_LENGTH_MULTIPLIER;

                let n = v.normal();
                let col = Color::rgb(
                    ((n[0] + 1.0) / 2.0 * 255.0) as u8, 
                    ((n[1] + 1.0) / 2.0 * 255.0) as u8, 
                    0
                );

                let vertices = [
                    Vertex::with_pos_color(SFVec2::new(x1 as f32, y1 as f32), col),
                    Vertex::with_pos_color(SFVec2::new(x2 as f32, y2 as f32), col)
                ];

                self.window.draw_primitives(
                    &vertices, 
                    PrimitiveType::LINES,
                    &RenderStates::DEFAULT
                );
            }
        }
    }

    fn render_pressure(&mut self) {
        const MIN: f64 = -0.5;
        const MAX: f64 = 0.5;
        self.draw_field(|m| &m.solver.pressure, |p: f64| -> Color {
            Color::rgb(
                ((1.0 - p.max(0.0) / MAX) * 255.0) as u8,
                0u8,
                ((1.0 - p.min(0.0) / MIN) * 255.0) as u8,
            )
        });
    }

    fn render_density(&mut self) {
        self.draw_field(|m| &m.solver.density , |d: Vector3| -> Color {
            Main::make_colour(d[0], d[1], d[2]) 
        });
    }

    fn draw_field<
        T: Copy, 
        GetField: Fn(&Self) -> &DiscreteField<T>, 
        ToColour: Fn(T) -> Color
    >(&mut self, field: GetField, colour: ToColour) {
        let f = field(self);
        for x in 0 .. FLUID_WIDTH-1 {
            for y in 0 .. FLUID_HEIGHT-1 {                
                let p00 = colour(f[x][y]);
                let p10 = colour(f[x+1][y]);
                let p01 = colour(f[x][y+1]);
                let p11 = colour(f[x+1][y+1]);
                let (px, py) = Self::world_to_screen((x as f64, y as f64));
                let (dx, dy) = Self::world_to_screen((1.0, 1.0));
                let v00 = SFVec2::new(px        as f32, py as f32);
                let v10 = SFVec2::new((px + dx) as f32, py as f32);
                let v01 = SFVec2::new(px        as f32, (py + dy) as f32);
                let v11 = SFVec2::new((px + dx) as f32, (py + dy) as f32);
                let vertices = [
                    Vertex::with_pos_color(v00, p00),
                    Vertex::with_pos_color(v10, p10),
                    Vertex::with_pos_color(v11, p11),
                    Vertex::with_pos_color(v00, p00),
                    Vertex::with_pos_color(v11, p11),
                    Vertex::with_pos_color(v01, p01),
                ];
                self.window.draw_primitives(
                    &vertices, 
                    PrimitiveType::TRIANGLES,
                    &RenderStates::DEFAULT
                );
            }
        }
    }

    fn make_colour(r: f64, g: f64, b: f64) -> Color {
        Color::rgb(
            (r * 255.0) as u8,
            (g * 255.0) as u8,
            (b * 255.0) as u8
        )
    }
    
    fn colour_gradient(t: f64) -> (f64, f64, f64) {
        fn f(x: f64, y: f64) -> f64 { (1.0 + (x + y).sin()) * 0.5 }
        ( f(t, 1.0), f(t, 2.0), f(t, 3.0) ) 
    }

    fn screen_to_world<T: Into<f64> + Sized>(xy: (T, T)) -> (f64, f64) {
        (xy.0.into() / SCALE.0 as f64, xy.1.into() / SCALE.1 as f64)
    }
    
    fn world_to_screen<T: Into<f64> + Sized>(xy: (T, T)) -> (f64, f64) {
        (xy.0.into() * SCALE.0 as f64, xy.1.into() * SCALE.1 as f64)
    }
}

pub fn main() {
    Main::init().run();
}

