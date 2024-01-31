/* gpumandelbrot
 * A mandelbrot set renderer which runs on the GPU
 */
use glfw::{Action, Context as _, Key, WindowEvent, MouseButton};
use luminance::context::GraphicsContext;
use luminance::pipeline::PipelineState;
use luminance::pipeline::Viewport;
use luminance::render_state::RenderState;
use luminance::tess::Mode;
use luminance_derive::{Semantics, Vertex};
use luminance_glfw::GlfwSurface;
use luminance_windowing::{WindowDim, WindowOpt};
use luminance::shader::Uniform;
use luminance::UniformInterface;
use std::env;

const VERT_SHADER: &str = include_str!("vert.glsl");
const FRAG_SHADER: &str = include_str!("frag.glsl");
const DEFAULT_WIDTH: u32 = 900;
const DEFAULT_HEIGHT: u32 = 600;

#[derive(Copy, Clone, Debug, Semantics)]
pub enum VertexSemantics {
    #[sem(name = "position", repr = "[f32; 2]", wrapper = "VertexPosition")]
    Position
}

#[derive(Clone, Copy, Debug, Vertex)]
#[vertex(sem = "VertexSemantics")]
pub struct Vertex {
    position: VertexPosition,
}

#[derive(Debug, UniformInterface)]
struct ShaderInterface {
    window_height: Uniform<f32>,
    offset_x: Uniform<f32>,
    offset_y: Uniform<f32>,
    scale: Uniform<f32>,
    rgb_offsets: Uniform<[f32; 3]>
}

const SCREEN_COORDS: [Vertex; 6] = [
    Vertex::new(VertexPosition::new([-1.0, -1.0])),
    Vertex::new(VertexPosition::new([ 1.0, -1.0])),
    Vertex::new(VertexPosition::new([ 1.0,  1.0])),
    Vertex::new(VertexPosition::new([ 1.0,  1.0])),
    Vertex::new(VertexPosition::new([-1.0,  1.0])),
    Vertex::new(VertexPosition::new([-1.0, -1.0]))
];

fn run_app(surface: GlfwSurface, rgb_offsets: [f32; 3]) {
    let mut context = surface.context;
    let events = surface.events_rx;
    let back_buffer = context.back_buffer().expect("back buffer");

    let mut offset = (-2.5 as f32, -1.5 as f32);
    let mut scale = 200.0;
    let mut mouse_pressed = false;
    let mut mouse_pos = (0.0, 0.0);

    let mut width = DEFAULT_WIDTH;
    let mut height = DEFAULT_HEIGHT;

    const SCALE_DELTA: f32 = 0.05;

    let triangle = context
        .new_tess()
        .set_vertices(&SCREEN_COORDS[..])
        .set_mode(Mode::Triangle)
        .build()
        .unwrap();

    let mut program = context
        .new_shader_program::<VertexSemantics, (), ShaderInterface>()
        .from_strings(VERT_SHADER, None, None, FRAG_SHADER)
        .unwrap()
        .ignore_warnings();

    'app: loop {
        // handle events
        context.window.glfw.poll_events();
        for (_, event) in glfw::flush_messages(&events) {
            match event {
            WindowEvent::MouseButton(MouseButton::Button1, action, _) => {
                mouse_pressed = action == Action::Press;
            },
            WindowEvent::CursorPos(x, y) => {
                if mouse_pressed {
                    offset.0 -= (x as f32 - mouse_pos.0) / scale;
                    offset.1 -= (y as f32 - mouse_pos.1) / scale;
                }
                mouse_pos = (x as f32, y as f32);
            },
            WindowEvent::Scroll(_, delta) => {
                // Function to convert device coords
                // to world coords
                let dc_to_world = |
                    pos: (f32, f32), offset: (f32, f32), scale: f32
                | -> (f32, f32) {
                    let x = pos.0 / scale + offset.0;
                    let y = pos.1 / scale + offset.1;
                    (x, y)
                };
                // Convert the mouse pos to worldspace before and after the
                // zoom, and correct the difference
                let (bx, by) = dc_to_world(mouse_pos, offset, scale);
                if delta < 0.0 { scale *= 1.0 - SCALE_DELTA; }
                else           { scale *= 1.0 + SCALE_DELTA; }
                let (ax, ay) = dc_to_world(mouse_pos, offset, scale);
                offset.0 += bx - ax;
                offset.1 += by - ay;
            },
            WindowEvent::Size(w, h) => {
                width = w as u32;
                height = h as u32;
            },
            WindowEvent::Close | 
            WindowEvent::Key(Key::Escape, _, _, _) => break 'app,
            _ => (),
            }
        }
        
        // Account for any change in window size
        let pipeline_state = PipelineState::default().set_viewport(
            Viewport::Specific{
                x: 0, y: 0, width, height
            }
        );

        let render = context
            .new_pipeline_gate()
            .pipeline(&back_buffer, &pipeline_state, |_, mut shd_gate| {
                shd_gate.shade(&mut program, |mut iface, uni, mut rdr_gate| {
                    iface.set(&uni.window_height, height as f32);
                    iface.set(&uni.offset_x, offset.0);
                    iface.set(&uni.offset_y, offset.1);
                    iface.set(&uni.scale, scale);
                    iface.set(&uni.rgb_offsets, rgb_offsets);

                    rdr_gate.render(&RenderState::default(), |mut tess_gate| {
                        tess_gate.render(&triangle)
                    })
                })
            },
        ).assume();

        // swap buffer chains
        if render.is_ok() {
            context.window.swap_buffers();
        } else {
            eprintln!("Rendering error");
            break 'app;
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();

    let rgb_offsets = match args.len() {
        4 => {
            let r = args[1].parse::<f32>().ok().unwrap();
            let g = args[2].parse::<f32>().ok().unwrap();
            let b = args[3].parse::<f32>().ok().unwrap();
            [r, g, b]
        },
        _ => [0.0, 0.0, 0.0]
    };

    let opt = WindowOpt::default().set_dim(
        WindowDim::Windowed {
            width: DEFAULT_WIDTH,
            height: DEFAULT_HEIGHT,
        }
    );
    let surface = GlfwSurface::new_gl33("Hello, world!", opt);

    match surface {
        Ok(surface) => {
            run_app(surface, rgb_offsets);
        }

        Err(e) => {
            eprintln!("cannot create graphics surface:\n{}", e);
            return;
        }
    }
}