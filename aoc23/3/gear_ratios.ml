let max a b = if a > b then a else b

let min a b = if a < b then a else b

let rec range a b =
    if   a < b 
    then a :: range (a+1) b
    else []

let is_symbol = function
    | '!' .. '-' 
    | '/' 
    | ':' .. '@' 
    | '[' .. '`' 
    | '{' .. '~' -> true
    | _          -> false

let schematic =
    let channel = open_in "input.txt" in
    let rec read acc =
        try
            let line = input_line channel in
            read @@ line :: acc
        with End_of_file ->
            close_in channel;
            List.rev acc 
    in
    read []

let height = List.length schematic

let width = String.length @@ List.hd schematic

let locate_all pattern =
    let re = Str.regexp pattern in
    let rec locate index y str =
        try 
            let start  = Str.search_forward re str index in
            let finish = Str.match_end () in
            (start, finish, y) :: locate finish y str
        with Not_found -> []
    in
    let by_row = List.mapi (locate 0) schematic in
    List.fold_left (@) [] by_row

let extract_number (start, finish, row) =
    let row = List.nth schematic row
    let str = String.sub row start @@ finish - start in
    int_of_string str

let part_locations = 
    let neighbours (start, finish, row) =
        let (x1, x2) = (max (start-1) 0, min (finish+1) width)  in
        let (y1, y2) = (max (row-1)   0, min (row+2)    height) in
        List.fold_left (fun yacc y ->
            yacc @ List.fold_left (fun xacc x ->
                let row = List.nth schematic y in
                let c = String.get row x in
                c :: xacc
            ) [] @@ range x1 x2
        ) [] @@ range y1 y2
    in 

    let is_part_number location = 
        let n = neighbours location in
        let s = List.filter is_symbol n in
        List.length s > 0
    in

    let nums = locate_all "[0-9]+" in
    List.filter is_part_number nums

let sum_of_parts = 
    List.fold_left (+) 0 @@ List.map extract_number part_locations

let sum_of_gear_ratios = 
    let gear_ratio (column, _, row) =
        let adj = List.filter (fun (part_col_start, part_col_end, part_row) ->
            column >= (part_col_start-1) && column <  (part_col_end+1) &&
            row    >= (part_row-1)       && row    <= (part_row+1)
        ) part_locations in
        match adj with
        | a :: b :: [] -> Option.some @@ extract_number a * extract_number b
        | _            -> Option.none
    in

    let stars = locate_all "*" in
    let ratios = List.filter_map gear_ratio stars in
    List.fold_left (+) 0 ratios

let () =
    Printf.printf "%d\n" sum_of_parts;
    Printf.printf "%d\n" sum_of_gear_ratios

