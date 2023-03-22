from hissp import hisp
hisp(

["defun", "factorial", ["n"],
  ["if", 
    ["eq", "n", 0],
    1,
    ["*", "n", ["factorial", ["-", "n", 1]]]]],
    
["factorial", 5]

)