open Printf 

let range a b = List.init (b - a) (fun i -> a + i)

let make_array len get from = Array.init (len from) (get from)

let running_total vals =
    let rec calc v acc = match v with
        | [] -> []
        | x :: xs ->
            let n = x + acc in
            n :: calc xs n in
    calc vals 0

let image =
    "input.txt"
        |> Core.In_channel.read_lines
        |> List.map (make_array String.length String.get)
        |> make_array List.length List.nth 

let width = Array.length image.(0)

let height = Array.length image

let galaxies = 
    List.fold_left (fun acci i -> 
        acci @ List.fold_left (fun accj j -> 
            if image.(i).(j) == '#' then 
                (i, j) :: accj
            else 
                accj
        ) [] @@ range 0 width
    ) [] @@ range 0 height

let expand_space expansion_rate =
    let expand_space major_max minor_max get = range 0 major_max
        |> List.map (fun major ->
            let is_space minor = get major minor == '.' in
            let empty = List.for_all is_space @@ range 0 minor_max in
            if empty then expansion_rate - 1 else 0
        )
        |> running_total
        |> Array.of_list in
    let di = expand_space height width (fun maj min -> image.(maj).(min)) in
    let dj = expand_space width height (fun maj min -> image.(min).(maj)) in
    List.map (fun (i, j) -> (i + di.(i), j + dj.(j)))

let dist (i1, j1) (i2, j2) = abs (i2 - i1) + abs (j2 - j1)

let total_dist = 
    let rec total = function
        | x :: [] -> 0
        | x :: xs -> 
            let dists = List.fold_left (fun acc y -> acc + dist x y) 0 xs in
            dists + total xs in
    total

let part1 = galaxies |> expand_space 2 |> total_dist

let part2 = galaxies |> expand_space 1000000 |> total_dist

let () = printf "%d\n%d\n" part1 part2

