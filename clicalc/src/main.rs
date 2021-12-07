/* calc
 * A command line math expression parser with
 * support for variables and functions.
 */
use std::collections::HashMap;
use std::vec::Vec;

macro_rules! hashmap {
    ($( $key: expr => $val: expr ),*) => {{
         let mut map = ::std::collections::HashMap::new();
         $( map.insert($key, $val); )*
         map
    }}
}

enum Variable {
    Value(f64),
    Function(Function)
}

struct Function {
    params: Vec<String>,
    body: String
}

pub struct CalcVars {
    vars: HashMap<String, Variable>   
}

// type CalcRes = Result<f64, String>;

/* Gets a character 
*/
fn get_char(string: &str, index: usize) -> char {
    string.as_bytes()[index] as char
}

/* Tests whether the given string is a valid name for a
   variable or function.
*/
fn valid_identifier(name: &str) -> bool {
    // The name must not start with a number
    if get_char(name, 0).is_numeric() {
        return false;
    }

    // ...And must only consist of alphanumeric chars and underscores
    let mut valid = true;
    for c in name.chars() {
        if !c.is_alphanumeric() && c != '_' {
            valid = false;
            break;
        }
    }
    valid
}

/* Searches a segment of an expression for the lowest 
   precendence operator, and returns its character and index
*/
fn find_lowest_prec_op_in_segment(
    expression: &str, begin: usize, end: usize
) -> Option<(char, usize)> {
    const NO_PREC : i32 = std::f32::INFINITY as i32;
    let op_precedences = hashmap![
        '+' => 0, '-' => 1, '*' => 2, '/' => 3, '^' => 4
    ];

    let mut op : char = 's';
    let mut index : usize = 0;

    let mut precedence = NO_PREC; 
    let mut bracket_level = 0;

    // a - operator at the beginning should not be counted, as
    // it is a negation as opposed to a minus operator 
    let mut i = if get_char(expression, begin) == '-'
         { begin+1 } else { begin };

    while i < end {
        let c = expression.chars().nth(i).unwrap();
        if      c == '(' { bracket_level += 1; }
        else if c == ')' { bracket_level -= 1; }
        
        // If the char is an operator of lower precedence than the current
        // one and not bracketed, replace the current one
        if bracket_level == 0 
        && op_precedences.contains_key(&c) 
        && op_precedences.get(&c) < Some(&precedence) {
            op = c;
            index = i;
            precedence = *op_precedences.get(&c).unwrap();

            // If the next char is '-', treat it as 
            // part of the next number and skip it 
            if get_char(expression, i+1) == '-' { 
                i += 1;
            }
        }
        i += 1;
    }

    if precedence == NO_PREC {
        return None;
    }
    Some((op, index))
} 

/* takes an expression that could be a function, and splits
   it into a name, as well as parameters if there are any.
   "my_func(x, y, z)" will yield ("my_func", Some({"x", "y", "z"}))
*/
fn unwrap_lhs(lhs: &str) -> (String, Option<Vec<String>>) {
    let mut name = lhs;
    let mut parameters = None;
    
    if lhs.contains("(") {
        // The expression is a function
        let open = lhs.find("(").unwrap();
        let close = lhs.find(")").unwrap();
        name = &lhs[..open];
        
        // Split the inside of the bracket into a vec
        let param_str = &lhs[open+1..close];
        let split = param_str.split(",");
        parameters = Some(split
            .map(|x| x.to_string())
            .collect::<Vec<String>>());
    }
    
    (name.to_string(), parameters)
}

impl Function {
    /* Computes a function body using a a vector of arguments
    */
    pub fn call(&self, inputs: Vec<String>) -> Result<f64, String> {
        if inputs.len() != self.params.len() {
            return Err("Supplied arguments do not match parameters".to_string());
        }

        let mut param_vars = CalcVars::new();

        // Match each of the arguments
        for i in 0..inputs.len() {
            let value = inputs[i].parse::<f64>().unwrap();
            let key = &self.params[i];
            param_vars.vars.insert(key.to_string(), Variable::Value(value));
        }

        param_vars.parse_expression_segment(
            &self.body, 0, self.body.chars().count())
    }
}
              
impl CalcVars {
    pub fn new() -> CalcVars {
        CalcVars { vars: HashMap::new() }
    }

    /* Parses a mathematical expression
    */
    fn parse_expression_segment(
        &self, expression: &str, mut begin: usize, mut end: usize
    ) -> Result<f64, String> {
        if get_char(expression, begin) == '(' 
        && get_char(expression, end-1) == ')' {
            begin += 1;
            end -= 1;
        }
    
        let result = find_lowest_prec_op_in_segment(expression, begin, end);
        if result.is_none() {
            // No operators in segment means that the segment is an operand
            let substring = &expression[begin..end].to_string();
            let c = get_char(&substring.to_string(), 0);
            
            // If the first character is a number, treat the operand as one
            if c.is_numeric() {
                let result = substring.parse::<f64>();
                return match result {
                    Err(_) => Err("Invalid value".to_string()),
                    Ok(val) => Ok(val)
                };
            }

            // Otherwise treat it as an identifier
            let (name, params) = unwrap_lhs(&substring);

            if !self.vars.contains_key(&name) {
                return Err("Variable does not exist".to_string());
            }

            let contents = self.vars.get(&name).unwrap();
            return match contents {
                Variable::Value(v)    => Ok(*v),
                Variable::Function(f) => f.call(params.unwrap())
            };
        }
    
        let (op, index) = result.unwrap();

        // If an operator is found, compute its result by parsing either side
        let lhs_res = self.parse_expression_segment(expression, begin, index);
        let rhs_res = self.parse_expression_segment(expression, index+1, end);
        if lhs_res.is_err() || rhs_res.is_err() {
            return Err("Invalid value".to_string());
        }

        let lhs = lhs_res.ok().unwrap();
        let rhs = rhs_res.ok().unwrap();

        return Ok(match op {
            '+' => rhs + lhs,
            '-' => rhs - lhs,
            '*' => rhs * lhs,
            '/' => rhs / lhs,
            '^' => rhs.powf(lhs),
            _ => 0.0 // ??????
        });
    }
    
    /* Parses a raw line of input from the user, and returns
       a response string
    */
    pub fn parse_expression(&mut self, expression: &str) -> String {
        // Hack: remove all whitespace. 
        let exp = &expression.replace(" ", "");
        let eq_index = exp.find("=");

        if eq_index == None {
            // No equals operator, so just return the result of the expression
            let res = self.parse_expression_segment(exp, 0, exp.len());

            return match res {
                Ok(val) => val.to_string(),
                Err(msg) => msg
            };
        }

        let index = eq_index.unwrap();
        let lhs = &exp[0..index];
        let rhs = &exp[index+1..];
        
        // Parse the lhs. params will be None if it is just a variable name
        let (identifier, params) = unwrap_lhs(lhs);

        // No params. Parse the rhs and assign it to a variable
        if params == None {
            let res = self.parse_expression_segment(&rhs, 0, rhs.len());
            return match res {
                Ok(v) => {
                    self.vars.insert(lhs.to_string(), Variable::Value(v));
                    format!("{} {} {}", lhs, "=", v)
                },
                Err(s) => s
            };
        }

        // Params are present, so assign it as a function definition
        let func = Function { params: params.unwrap(), body: rhs.to_string() };
        self.vars.insert(identifier.to_string(), Variable::Function(func));
        format!("{} {} {}", lhs, "=", rhs)
    }
}

use rustyline::Editor;

fn main() {
    // `()` can be used when no completer is required
    let mut rl = Editor::<()>::new();
    let mut vars = CalcVars::new();
    loop {
        let readline = rl.readline(">>> ");
        match readline {
            Ok(line) => {
                rl.add_history_entry(line.as_str());
                println!("{}", vars.parse_expression(&line));
            },
            Err(err) => {
                println!("Error: {:?}", err);
                break;
            }
        }
    }
}