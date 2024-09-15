module NodeMap = Map.Make(String)

let (<>) = Core.Fn.compose

let nodes, instructions =
    let inst :: _ :: rest = Core.In_channel.read_lines "input.txt" in
    let instructions = Stream.from (fun i ->
        i mod String.length inst |> String.get inst |> Option.some
    ) in
    let nodes = List.fold_left (fun acc line ->
        let from  = String.sub line 0  3 in
        let left  = String.sub line 7  3 in
        let right = String.sub line 12 3 in
        NodeMap.add from (left, right) acc
    ) NodeMap.empty rest in
    nodes, instructions

let search is_found start_from =
    let rec steps_to on = 
        if is_found on then 0 else
        let l, r = NodeMap.find on nodes in
        let dir = Stream.next instructions in
        let next = match dir with
            | 'L' -> l
            | 'R' -> r in
        1 + steps_to next
    in
    steps_to start_from

(* PART 1 *)

let steps_aaa_to_zzz = search (fun x -> x = "ZZZ") "AAA" 

(* PART 2 *)

let lcm (x :: xs) =
    let rec gcd x y =
        if y == 0 
        then x
        else gcd y @@ x mod y
    in
    List.fold_left (fun acc v ->
        acc * v / gcd acc v
    ) x xs
    
let steps_to_all_z =
    nodes 
        |> NodeMap.bindings 
        |> List.map (fun (x, _) -> x) 
        |> List.filter @@ String.ends_with ~suffix: "A"
        |> List.map (search @@ String.ends_with ~suffix: "Z")
        |> lcm

let () =
    Printf.printf "%d\n" steps_aaa_to_zzz;
    Printf.printf "%d\n" steps_to_all_z

