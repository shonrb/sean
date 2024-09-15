let path = "input.txt"

let read_lines p =
    let channel = open_in p in
    let rec read acc = 
        try 
            let line = input_line channel in
            read @@ line :: acc
        with End_of_file ->
            close_in channel;
            List.rev acc in
    read []

let digit_value = function 
    | "one"   | "1" -> 1
    | "two"   | "2" -> 2
    | "three" | "3" -> 3
    | "four"  | "4" -> 4
    | "five"  | "5" -> 5
    | "six"   | "6" -> 6 
    | "seven" | "7" -> 7
    | "eight" | "8" -> 8
    | "nine"  | "9" -> 9
    | _             -> raise (Failure "not a digit")

let get_value pattern line = 
    let get () = digit_value @@ Str.matched_string line in
    let back  = String.length line - 1 in
    ignore @@ Str.search_forward pattern line 0;
    let first = get () in
    ignore @@ Str.search_backward pattern line back;
    let last = get () in
    first * 10 + last

let calibration pattern =
    let lines = read_lines path in
    let values = List.map (get_value pattern) lines in
    List.fold_left (+) 0 values

let match1 = Str.regexp "[0-9]"
let match2 = Str.regexp 
    "one\\|two\\|three\\|four\\|five\\|six\\|seven\\|eight\\|nine\\|[0-9]"

let () = 
    Printf.printf "%d\n" @@ calibration match1;
    Printf.printf "%d\n" @@ calibration match2

