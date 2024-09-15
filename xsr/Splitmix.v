module Splitmix (
  input  wire [63 : 0] seed,
  output wire [63 : 0] next_state,
  output wire [63 : 0] value
);

parameter val1 = 64'h9e3779b97f4a7c15; 
parameter val2 = 64'hbf58476d1ce4e5b9;
parameter val3 = 64'h94d049bb133111eb; 

wire [63 : 0] a;
wire [63 : 0] b;
wire [63 : 0] c;

assign a   = seed + val1;
assign b   = (a ^ (a >> 30)) * val2;
assign c   = (b ^ (b >> 27)) * val3;
assign value = (c ^ (c >> 31));
assign next_state = a;

endmodule

