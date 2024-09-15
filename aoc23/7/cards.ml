module CountMap = Map.Make(Char)

type countmap = int CountMap.t

let add_char map c = 
    let count = match CountMap.find_opt c map with 
        | Option.Some(count) -> count 
        | Option.None        -> 0
    in
    CountMap.add c (count + 1) map

let snd (_, x) = x

let parse_char c = int_of_char c - int_of_char '0'

let hands =
    List.map (fun line ->
        let [c; b] = String.split_on_char ' ' line in
        let cards = List.init (String.length c) (String.get c) in
        let bet = int_of_string b in
        (cards, bet)
    ) @@ Core.In_channel.read_lines "input.txt"

let rec compare_hands card_value h1 h2 = match (h1, h2) with
    | (x1 :: x1s, x2 :: x2s) -> 
        if   x1 == x2 
        then compare_hands card_value x1s x2s
        else compare (card_value x1) (card_value x2)
    | _ -> 0

let winnings hand_type_value card_value =
    hands |> List.sort (fun (h1, _) (h2, _) ->
        let types = compare (hand_type_value h1) (hand_type_value h2) in 
        if   types == 0
        then compare_hands card_value h1 h2
        else types 
    ) |> List.mapi (fun i (_, bet) ->
        bet * (i + 1)
    ) |> List.fold_left (+) 0

(* PART 1 *)

let card_value c = match c with
    | 'A' -> 14
    | 'K' -> 13
    | 'Q' -> 12
    | 'J' -> 11
    | 'T' -> 10
    | '2' .. '9' -> parse_char c

let card_count_value = function
    | [5]             -> 6
    | [1; 4]          -> 5
    | [2; 3]          -> 4 
    | [1; 1; 3]       -> 3
    | [1; 2; 2]       -> 2
    | [1; 1; 1; 2]    -> 1
    | [1; 1; 1; 1; 1] -> 0

let hand_type_value cards = 
    cards 
        |> List.fold_left add_char CountMap.empty 
        |> CountMap.bindings
        |> List.map snd
        |> List.sort compare
        |> card_count_value

(* PART 2 *)

let card_value2 c = if c == 'J' then 1 else card_value c

let hand_type_value2 cards =
    let freqs = List.fold_left add_char CountMap.empty cards in
    let jokers = Option.value (CountMap.find_opt 'J' freqs) ~default:0 in
    let counts = freqs
        |> CountMap.remove 'J'
        |> CountMap.bindings
        |> List.map snd
        |> List.sort compare in
    let c :: cs = match List.rev counts with
        | []     -> [0]
        | _ as l -> l in
    let adjusted = List.rev @@ (c + jokers) :: cs in
    card_count_value adjusted

let () =
    Printf.printf "%d\n" @@ winnings hand_type_value card_value;
    Printf.printf "%d\n" @@ winnings hand_type_value2 card_value2

