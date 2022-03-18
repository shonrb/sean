module BaudRateCounter #(parameter SAMPLING_RATE) (
    input wire clk,
    output wire out
);

parameter BAUD_RATE = 9600;
parameter CLK_SPEED = 50000000;
parameter CNT_MAX = CLK_SPEED / (2 * BAUD_RATE * SAMPLING_RATE);
parameter CNT_SIZE = $clog2(CNT_MAX) - 1;

reg [CNT_SIZE: 0] cnt;
reg outreg = 1'b0;

assign out = outreg;

always @(posedge clk) begin
    if (cnt == CNT_MAX[CNT_SIZE:0]) begin
        cnt <= 0;
        outreg <= ~outreg;
    end
    else
        cnt <= cnt + 1;
end
endmodule
