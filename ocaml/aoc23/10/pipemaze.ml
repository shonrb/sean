open Printf 

module Point = struct
    type t = int * int
    let compare (a1, a2) (b1, b2) = match Stdlib.compare a1 b1 with
        | 0 -> Stdlib.compare a2 b2
        | c -> c
end

module PointSet = Set.Make(Point)

let make_array len get from = Array.init (len from) (get from)

let set_size = Core.Fn.compose List.length PointSet.elements

let rec range a b =
    if   a < b
    then a :: range (a+1) b
    else []

let maze =
    "input.txt"
        |> Core.In_channel.read_lines 
        |> List.map (make_array String.length String.get) 
        |> make_array List.length List.nth

(* PART 1 *)

let animal = 
    let rec find_row i = 
        let row = maze.(i) in
        let res = Array.find_index (fun x -> x == 'S') row in
        match res with
            | Option.Some(j) -> i, j
            | Option.None    -> find_row @@ i + 1
    in
    find_row 0

let loop =
    let rec search i j di dj =
        let c = maze.(i).(j) in
        PointSet.add (i, j) @@ match c with 
            | 'S' -> PointSet.empty
            | '|' -> search (i + di) j di dj
            | '-' -> search i (j + dj) di dj
            | '7' | 'L' -> search (i + dj) (j + di) dj di
            | 'F' | 'J' -> search (i - dj) (j - di) (-dj) (-di)
    in
    let i, j = animal in
    match maze.(i+1).(j) with | 'J' | '|' | 'L' -> search (i-1) j (-1) 0 | _ ->
    match maze.(i-1).(j) with | '7' | '|' | 'F' -> search (i+1) j 1 0    | _ ->
    match maze.(i).(j+1) with | 'J' | '-' | '7' -> search i (j+1) 0 1    | _ ->
    match maze.(i).(j-1) with | 'L' | '-' | 'F' -> search i (j-1) 0 (-1)

let loop_length = set_size loop

(* PART 2 *)

let max_i = Array.length maze

let max_j = Array.length maze.(0)

let enclosed_in_row i =
    let changes entry this = match (entry, this) with
        | ('F', 'J')
        | ('L', '7')
        | (_,   '-') 
        | (_,   'S') -> false
        | _ -> true
    in

    let rec cast j prev_change odd = 
        if j == max_j then 
            PointSet.empty 
        else if PointSet.mem (i, j) loop then
            let this = maze.(i).(j) in
            if   changes prev_change this
            then cast (j+1) this (not odd) 
            else cast (j+1) prev_change odd
        else if odd then
            PointSet.add (i, j) @@ cast (j+1) prev_change odd
        else 
            cast (j+1) prev_change odd
    in
    cast 0 ' ' false

let inside =
    range 0 max_i
        |> List.map enclosed_in_row
        |> List.fold_left PointSet.union PointSet.empty

let show () = 
    List.iter (fun i -> 
        List.iter (fun j -> 
            printf @@ 
                if PointSet.mem (i, j) loop then
                    "\o033[31m"
                else if PointSet.mem (i, j) inside then
                    "\o033[32m"
                else
                    "";
            printf "%c\o033[0m" @@ maze.(i).(j)
        ) @@ range 0 max_i;
        printf "\n%!"
    ) @@ range 0 max_j;
    ()

let () =
    show ();
    printf "%d\n" @@ loop_length / 2;
    printf "%d\n" @@ set_size inside
