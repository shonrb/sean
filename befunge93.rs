/* befunge93.rs
 * A befunge-93 interpreter.
 */
use std::env;
use std::fs::File;
use std::io;
use io::{BufRead, BufReader};
use std::time::{SystemTime, UNIX_EPOCH};

enum Direction {
    None, Up, Down, Left, Right
}

struct Interpreter {
    memory:      Vec<char>,
    stack:       Vec<u8>,
    rng_state:   u32,

    pc_x:        usize,
    pc_y:        usize,
    direction:   Direction,
    
    running:     bool,
    string_mode: bool,
    bridging:    bool,
}

fn parse_char(c: char) -> u8 {
    if !('0'..='9').contains(&c) {
        panic!("Non numerical char passed");
    }
    c as u8 - '0' as u8
}

fn read_char(prompt: &str) -> char {
    /* Reads a single character from stdin
     */
    print!("{}", prompt);

    let mut line = String::new();
    std::io::stdin()
        .read_line(&mut line)
        .expect("Failed to read line");

    // Fail if there is no character given
    return line.chars()
        .nth(0)
        .expect("No Input Given");
}

impl Interpreter {
    const WIDTH:  usize = 80;
    const HEIGHT: usize = 25;

    pub fn new(lines: Vec<String>, rng_seed: u32) -> Self {
        // Program memory buffer
        let mut buffer: Vec<char> = Vec::new();
        
        // Add the contents of each line to the buffer
        for (i, line) in lines.iter().enumerate() {
            buffer.append(&mut line.chars().collect());
            
            // Fail if the line is >80 chars long
            let len = line.len();
            if len > Self::WIDTH {
                panic!("LINE {} WAS TOO LONG", i);
            }
            
            // Add padding to make the line 80 chars long
            let     padding_len = Self::WIDTH - len;
            let mut padding     = vec![' '; padding_len];
            buffer.append(&mut padding);
        }
        
        // Add padding to reach 25 lines
        let     num_lines         = lines.len();
        let     horiz_padding_len = (Self::HEIGHT - num_lines) * Self::WIDTH;
        let mut horiz_padding     = vec![' '; horiz_padding_len];
        buffer.append(&mut horiz_padding);

        // Fail if there is more than 25 lines
        if num_lines > Self::HEIGHT {
            panic!("FILE CONTAINS TOO MANY LINES");
        }

        Self {
            memory:      buffer,
            stack:       Vec::new(),
            rng_state:   rng_seed,
            pc_x:        0,
            pc_y:        0,
            direction:   Direction::None,
            running:     true,
            string_mode: false,
            bridging:    false
        }
    }

    fn mem_index(x: usize, y: usize) -> usize {
        /* Translates coordinates x, y into an offset into memory.
         * Values outside the dimensions of the program wrap around.
         */
        assert!(x < Self::WIDTH, "x out of range");
        assert!(y < Self::HEIGHT, "y out of range");
        x + y * Self::WIDTH
    }

    fn mem_read(&self, x: usize, y: usize) -> char {
        let index = Interpreter::mem_index(x, y);
        self.memory[index]
    }

    fn mem_write(&mut self, x: usize, y: usize, value: char) {
        let index = Interpreter::mem_index(x, y);
        self.memory[index] = value;
    }

    fn pop_stack(&mut self) -> u8 {
        if let Some(val) = self.stack.pop() {
            return val;
        }
        // If the stack is empty, popping from the stack returns 0
        0
    }

    fn pop2_stack(&mut self) -> (u8, u8) {
        let a = self.pop_stack();
        let b = self.pop_stack();
        (a, b)
    }

    fn do_op(&mut self, op: fn(u8, u8) -> u8) {
        /* Helper function for implementing the basic arithmetic
         * operators + - * / %. Pops two values and pushes the result
         * of op() when applied to them.
         */
        let (a, b) = self.pop2_stack();
        let res = op(a, b);
        self.stack.push(res);
    }

    fn conditional_move(&mut self, zero: Direction, nonzero: Direction) {
        /* Helper function for implementing | and _
         * Pops a value, begin moving in direction 
         * 'zero' if it is zero, otherwise move in 'nonzero'.
         */
        self.direction = 
            if self.pop_stack() == 0 { zero }
            else                     { nonzero };
    }

    fn next_random(&mut self) -> u32 {
        let product = self.rng_state as u64 * 48271;
        let x = ((product & 0x7fffffff) + (product >> 31)) as u32;

        self.rng_state = (x & 0x7fffffff) + (x >> 31);
        self.rng_state
    }

    pub fn do_instruction(&mut self, instr: char) {
        match instr {
            // 0-9 	Push this number on the stack 
            '0'..='9' => self.stack.push(parse_char(instr)),

            // Arithmetic operations. Pop a & b, apply an 
            // operator then push the result onto the stack.
            '+' => self.do_op(|a, b| a + b ),
            '-' => self.do_op(|a, b| a - b ),
            '*' => self.do_op(|a, b| a * b ),
            '/' => self.do_op(|a, b| a / b ),
            '%' => self.do_op(|a, b| a % b ),

            // Logical NOT: Pop a value. If the value is zero, 
            // push 1; otherwise, push zero. 
            '!' => {
                let val = self.pop_stack();
                let res = if val == 0 { 0 } else { 1 };
                self.stack.push(res);
            },

            // Greater than: Pop a and b, 
            // then push 1 if b>a, otherwise zero. 
            '`' => {
                let (a, b) = self.pop2_stack();
                let res    = if b > a { 1 } else { 0 };
                self.stack.push(res);
            },
            
            // Movement operations
            '>' => self.direction = Direction::Right,
            '<' => self.direction = Direction::Left, 
            '^' => self.direction = Direction::Up,   
            'v' => self.direction = Direction::Down, 
            // Movement in a random cardinal direction 
            '?' => {
                let val = self.next_random() % 4;
                self.direction = match val {
                    0 => Direction::Right,
                    1 => Direction::Left,
                    2 => Direction::Up,
                    3 => Direction::Down,
                    _ => panic!("Unreachable")
                }
            },

            // Pop a value, move right if value=0, left otherwise 
            '_' => self.conditional_move(Direction::Right, Direction::Left),
            // Pop a value; move down if value=0, up otherwise 
            '|' => self.conditional_move(Direction::Down, Direction::Up),

            // Start string mode: push each character's 
            // ASCII value all the way up to the next "
            '\"' => self.string_mode = true,

            // Duplicate value on top of the stack 
            ':' => {
                let v = self.pop_stack();
                self.stack.push(v);
                self.stack.push(v);
            },

            // Swap two values on top of the stack 
            '\\' => {
                let (a, b) = self.pop2_stack();
                self.stack.push(b);
                self.stack.push(a);
            },

            // Operations on a value popped from the stack
            '$' => { let _ = self.pop_stack(); },          // Discard
            '.' => print!("{} ", self.pop_stack()),        // Output
            ',' => print!("{}", self.pop_stack() as char), // Output as char

            // Bridge: Skip next cell 
            '#' => self.bridging = true,

            // Put: Pop y, x, and v, then write v to [x, y]
            'p' => {
                let (y, x) = self.pop2_stack();
                let v      = self.pop_stack();
                self.mem_write(x as usize, y as usize, v as char);
            }
            // Get: Pop y and x, read [x, y] to the stack
            'g' => {
                let (y, x) = self.pop2_stack();
                let v = self.mem_read(x as usize, y as usize);
                self.stack.push(v as u8);
            },
            // Read number from input to the stack
            '&' => {
                let c = read_char("Enter a number: ");
                self.stack.push(parse_char(c));
            }

            // Read character from input to the stack
            '~' => {
                let c = read_char("Enter a character: ");
                self.stack.push(c as u8);
            }

            '@' => self.running = false, // End program
            ' ' => (),                   // No-op. Does nothing
            _   => panic!("Syntax error: {}", instr)
        }
    }

    pub fn run(&mut self) {
        while self.running {
            // Read the memory at PC
            let instr = self.mem_read(self.pc_x, self.pc_y);
            
            if self.string_mode {
                // String mode means characters read are pushed to the
                // stack rather than interpreted, until a closing (") is read.
                if instr == '\"' {
                    self.string_mode = false;
                } else {
                    let val = instr as u8;
                    self.stack.push(val);
                }
            } else if self.bridging {
                // Bridging mode means one instruction is ignored
                self.bridging = false;
            } else {
                // Otherwise do the instruction normally
                self.do_instruction(instr);
            }

            // Move the pc in the current direction
            match self.direction {
                Direction::Up    => self.pc_y = (self.pc_y - 1).min(Self::HEIGHT-1),
                Direction::Left  => self.pc_x = (self.pc_x - 1).min(Self::WIDTH-1),
                Direction::Down  => self.pc_y = (self.pc_y + 1) % Self::HEIGHT,
                Direction::Right => self.pc_x = (self.pc_x + 1) % Self::WIDTH,
                _ => {}
            }
        }
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        println!("Usage: {} [filename]", args[0]);
        return;
    }

    // Open the file.
    let filename = &args[1];
    let file     = File::open(filename).expect("Unable to open file");
    let reader   = BufReader::new(file);

    // Collect the lines in the file
    let lines: Vec<String> = reader.lines()
        .filter_map(Result::ok)
        .collect();

    // Get the time as a seed for the prng
    let time = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("System time failure")
        .as_millis() as u32;

    let mut interpreter = Interpreter::new(lines, time);
    interpreter.run();
}
