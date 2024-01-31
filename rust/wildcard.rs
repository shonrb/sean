/* wildcard.rs
 * A wildcard pattern matcher
 */
fn match_wild_seg(
    text: &Vec<char>, patt: &Vec<char>, 
    ti:   usize,      pi:   usize
) -> bool {
    // If the end of the pattern is reached,
    // there is a match if the end of the text is reached too.
    if pi == patt.len() {
        return ti == text.len();
    } 
    
    // The text index may be out of range here
    let tc = text.get(ti);
    let pc = patt[pi];

    // '*' matches an arbitrary number of any character
    if pc == '*' {
        for i in ti..text.len() {
            if match_wild_seg(text, patt, i, pi+1) {
                return true
            }
        }
        return match_wild_seg(text, patt, text.len(), pi+1);
    } else if tc.is_some() 
    &&        pc != '?' 
    &&        pc != *tc.unwrap() {
        return false;
    }

    // This segment is a match, is either the same character or '?'
    // '?' matches one instance of any character
    match_wild_seg(text, patt, ti+1, pi+1)
}

fn match_wild(text_str: &str, patt_str: &str) -> bool {
    let text: Vec<char> = text_str.chars().collect();
    let patt: Vec<char> = patt_str.chars().collect();
    match_wild_seg(&text, &patt, 0, 0)
}

fn main() {
    let args: Vec<String> = std::env::args().collect();

    if args.len() != 3 {
        panic!("Invalid args");
    }

    let text = &args[1];
    let patt = &args[2];
    println!("{}", match_wild(text, patt));
}
