open Printf

let rows =
    List.map (fun x ->
        List.map int_of_string @@ String.split_on_char ' ' x
    ) @@ Core.In_channel.read_lines "input.txt"

let rec rate_of_change = function
    | x1 :: x2 :: xs -> (x2 - x1) :: rate_of_change (x2 :: xs)
    | _              -> []

let rec get_extrapolated seq = 
    if List.for_all (fun x -> x == 0) seq then 0 else 
    let last   = List.hd @@ List.rev seq in
    let change = get_extrapolated @@ rate_of_change seq in
    last + change

let sum_extrapolated = 
    List.fold_left (fun acc row -> acc + get_extrapolated row) 0

(* PART 1 *)

let extrapolated_sum_forwards = sum_extrapolated rows

(* PART 2 *)

let extrapolated_sum_backwards = sum_extrapolated @@ List.map List.rev rows

let () =
    printf "%d\n" extrapolated_sum_forwards;
    printf "%d\n" extrapolated_sum_backwards
