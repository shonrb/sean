module Main (
    input wire clk, 
    input wire finish,
    input wire transmit,
    input reg [7:0] in,
    output reg [7:0] out,
    output reg read_done,
    output wire busy
);

wire connection;

Transmitter transmitter (
    .clk(clk),
    .busy(busy),
    .in(in),
    .transmit(transmit),
    .tx(connection)
);

Receiver receiver (
    .clk(clk),
    .rx(connection),
    .out(out),
    .done(read_done)
);

always @(posedge clk) begin
    if (finish)
        $finish;
end
endmodule;
