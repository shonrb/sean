module CountMap = Map.Make(String)

type count_map = int CountMap.t

type game =
    { id: int
    ; rounds: count_map list 
    }

let split_on s = Str.split @@ Str.regexp s

let parse_round round =
    let counts = CountMap.empty 
        |> CountMap.add "red"   0 
        |> CountMap.add "green" 0 
        |> CountMap.add "blue"  0 in
    let cubes = split_on ", " round in
    List.fold_left (fun acc cube ->
        let (count_str :: colour :: []) = split_on " " cube in
        let count = int_of_string count_str in
        CountMap.add colour count acc
    ) counts cubes

let parse_game game =
    let (tag :: data :: []) = split_on ": " game in
    let (_ :: id_str :: []) = split_on " "  tag  in
    let rounds_str          = split_on "; " data in 
    let id     = int_of_string id_str in
    let rounds = List.map parse_round rounds_str in
    { id = id; rounds = rounds }

let get_rgb round =
    let red   = CountMap.find "red"   round in
    let green = CountMap.find "green" round in
    let blue  = CountMap.find "blue"  round in
    (red, green, blue)

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

(* PART 1 *)

let is_possible game =
    List.fold_left (fun acc round ->
        let (r, g, b) = get_rgb round in
        acc && r <= 12 && g <= 13 && b <= 14
    ) true game.rounds

let sum_possible_ids games = 
    let possible = List.filter is_possible games in
    List.fold_left (fun a g -> a + g.id) 0 possible

(* PART 2 *)

let minimum_counts game = 
    List.fold_left (fun (r1, g1, b1) round ->
        let max a b = if a > b then a else b in
        let (r2, g2, b2) = get_rgb round in
        (max r1 r2, max g1 g2, max b1 b2)
    ) (0, 0, 0) game.rounds

let sum_power_minimum_sets = 
    List.fold_left (fun acc game ->
        let (r, g, b) = minimum_counts game in
        let power = r * g * b in
        acc + power
    ) 0 
 
let () =
    let lines = read_lines "input.txt" in
    let games = List.map parse_game lines in
    Printf.printf "%d\n" @@ sum_possible_ids games;
    Printf.printf "%d\n" @@ sum_power_minimum_sets games

