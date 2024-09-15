let times     = [40;  82;   91;   66]
let distances = [277; 1338; 1349; 1063]
let real_time     = 40829166
let real_distance = 277133813491063

let count_ways_to_win time record = 
    let rec check t = 
        if t > (time+1) / 2 then
            0
        else if t * (time - t) > record then
            time + 1 - t * 2
        else
            check (t + 1)
    in
    check 1

let product_of_ways_to_win = 
    List.fold_left Int.mul 1 @@ List.map2 count_ways_to_win times distances 
    
let () =
    Printf.printf "%d\n" product_of_ways_to_win;
    Printf.printf "%d\n" @@ count_ways_to_win real_time real_distance
