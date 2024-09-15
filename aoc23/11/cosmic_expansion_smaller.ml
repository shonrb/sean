open Printf
open Core

let (<<) = Fn.compose

let galaxies = "input.txt"
    |> In_channel.read_lines
    |> List.foldi ~init: [] ~f: (fun i acci line ->
        acci @ String.foldi line ~init: [] ~f: (fun j accj c ->
            if   c == '#'
            then (i, j) :: accj
            else accj
        )
    )

let total_dist expansion_rate =
    let total_dist_ax points = 
        let adjusted = List.map points ~f: (fun point ->
            let is_empty i = not @@ List.mem points i ~equal: (==) in
            let expansion = List.range 0 point
                |> List.filter ~f: is_empty
                |> List.length in
            point + expansion * (expansion_rate-1)
        ) in
        List.foldi adjusted ~init: 0 ~f: (fun i acci p1 ->
            let remaining = List.drop adjusted (i + 1) in
            acci + List.fold_left remaining ~init: 0 ~f: (fun accj p2 -> 
                accj + abs (p2 - p1)
            ) 
        ) in
    let is = List.map ~f: fst galaxies in
    let js = List.map ~f: snd galaxies in
    total_dist_ax is + total_dist_ax js

let () = printf "%d\n%d\n" (total_dist 2) (total_dist 1000000)

