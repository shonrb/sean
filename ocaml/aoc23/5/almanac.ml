open Str

(* HELPER *)

let (<>) = Core.Fn.compose

let foldl1 f (x :: xs) = List.fold_left f x xs

let fst (x, _) = x

let split_on delimiter source =
    let pattern = regexp delimiter in
    let rec next i =
        try
            let ending = search_forward pattern source i in
            let seg    = String.sub source i @@ ending - i in
            let rest   = next @@ match_end () in
            if i == ending then rest else seg :: rest
        with Not_found ->
            let rest = String.length source - i in
            String.sub source i rest :: []
    in
    next 0

(* PARSE *)

let parse_int_list = List.map int_of_string <> split_on " "

let parse_mapping str =
    let _ :: mappings = split_on "\n" @@ Core.String.strip str in
    let all = List.map (fun mapping ->
        let dest :: src :: len :: [] = parse_int_list mapping in
        (src, dest, len)
    ) mappings in
    let comp_range (a, _, _) (b, _, _) = compare a b in
    List.sort comp_range all

let (seeds, mappings) = 
    let contents = Core.In_channel.read_all "input.txt" in
    let seeds_str :: rest = split_on "\n\n" contents in
    let seeds = seeds_str |> split_on ": " |> List.tl |> List.hd |> parse_int_list in
    let mappings = List.map parse_mapping rest in
    (seeds, mappings)

(* PART 1 *)

let rec convert mapping value = match mapping with
    | (src_start, dest_start, len) :: xs ->
        let src_end = src_start + len - 1 in
        if   value >= src_start && value <= src_end
        then dest_start + value - src_start
        else convert xs value
    | [] -> value

let seed_to_loc = mappings |> List.map convert |> List.rev |> foldl1 (<>)

let minimum_loc = seeds |> List.map seed_to_loc |> foldl1 min

(* PART 2 *)

let split_range (start, length) on = 
    let finish = start + length - 1 in
    if   start < on && on < finish 
    then [(start, on - start); (on, finish - on)]
    else [(start, length)]

let split_range_on_range range on_start on_length =
    let on_finish = on_start + on_length - 1 in
    let x :: xs   = split_range range on_finish in 
    let y         = split_range x on_start in
    y @ xs

let rec split_range_on_mappings maps ((start, len) as range) =
    match maps with
    | (src_start, _, src_len) :: xs -> 
        let finish       = start     + len     - 1 in
        let src_finish   = src_start + src_len - 1 in
        if src_finish < start then
            split_range_on_mappings xs range
        else if finish < src_start then
            [range]
        else
            let split = split_range_on_range range src_start src_len in
            let last :: rest = List.rev split in
            rest @ split_range_on_mappings xs last 
    | [] -> [range]

let convert_range maps (start, len) = (convert maps start, len)

let convert_ranges maps =
    List.map (convert_range maps)
    <> foldl1 (@)
    <> List.map (split_range_on_mappings maps)

let seed_ranges_to_locs = 
    mappings |> List.map convert_ranges |> List.rev |> foldl1 (<>)

let rec ints_to_ranges = function 
    | x1 :: x2 :: xs -> (x1, x2) :: ints_to_ranges xs
    | _              -> []

let minimum_loc_with_ranges =
    seeds 
    |> ints_to_ranges |> seed_ranges_to_locs 
    |> List.map fst   |> foldl1 min

let () =
    Printf.printf "%d\n" minimum_loc;
    Printf.printf "%d\n" minimum_loc_with_ranges

