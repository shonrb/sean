open Str
open Pervasives

let rec range a b =
    if   a < b
    then a :: range (a+1) b
    else []

let (<>) = Core.Fn.compose

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

let cards = 
    let split_card str =
        let label :: numbers        :: [] = split_on ": " str in
        let _     :: card_no_string :: [] = split_on " " label in
        let win   :: have           :: [] = split_on " | " numbers in 
        (card_no_string, win, have)
    in

    let parse_card str = 
        let (n, w, h) = split_card str in
        let to_list   = List.map int_of_string <> split_on " +" in
        let winning   = to_list w in
        let have      = to_list h in
        let card_no   = int_of_string n in
        let wins      = List.length @@ List.filter (fun x ->
            List.exists (fun y -> x == y) winning
        ) have in
        (card_no, wins)
    in

    "input.txt" |> Core.In_channel.read_lines |> List.map parse_card

let total_points = 
    let card_points (_, win_count) =
        if   win_count == 0
        then 0 
        else Core.Int.pow 2 @@ win_count - 1
    in
    cards |> List.map card_points |> List.fold_left (+) 0

let total_rewarded_cards = 
    let rewarded_cards (number, winners) = 
        let indices = range number @@ number + winners in
        List.map (List.nth cards) indices
    in

    let rec process finished queue =
        match queue with
        | x :: xs -> 
            let new_cards    = rewarded_cards x in
            let new_queue    = new_cards @ xs in
            let new_finished = x :: finished in
            process new_finished new_queue
        | [] -> finished
    in
    
    cards |> process [] |> List.length

let () =
    Printf.printf "%d\n" total_points;
    Printf.printf "%d\n" total_rewarded_cards
    
