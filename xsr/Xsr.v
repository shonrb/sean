module Xsr (
  input wire [63:0] seed,
  input wire gen,
  output wire [63:0] out
);

function [63:0] rotl; 
  input [63:0] x, k;
  begin
    rotl = (x << k) | (x >> (64 - k));
  end
endfunction

reg start = 1;
reg [63:0] state1, state2, state3, state4, outreg = 0;
wire [63:0] mix1, mix2, mix3, _;
wire [63:0] init1, init2, init3, init4;

Splitmix splitmix1( .seed(seed), .next_state(mix1), .value(init1) );
Splitmix splitmix2( .seed(mix1), .next_state(mix2), .value(init2) );
Splitmix splitmix3( .seed(mix2), .next_state(mix3), .value(init3) );
Splitmix splitmix4( .seed(mix3), .next_state(_),    .value(init4) );

reg [63:0] t, s3, s4;

assign out = outreg;

always @(posedge gen) begin
  if (start) begin
    state1 = init1;
    state2 = init2;
    state3 = init3;
    state4 = init4;
    start = 0;
  end

  outreg = rotl(state2 * 5, 7) * 9;
  t  = state2 << 17;
  s3 = state3 ^ state1;
  s4 = state4 ^ state2;
  state2 = state2 ^ s3;
  state1 = state1 ^ s4;
  state3 = s3 ^ t;
  state4 = rotl(state4, 45);
end

endmodule

