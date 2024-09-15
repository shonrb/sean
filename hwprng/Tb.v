module Tb;

wire [63:0] out;
reg clock = 0;

Xsr xsr ( .seed(64'd1234), .gen(clock), .out(out) );

always
  #20 clock = !clock;

initial begin
  //$monitor("Clock: %d, PRNG Value: %d", clock, out);
  $monitor("%d", out);
  #1000 $finish;
end

endmodule
