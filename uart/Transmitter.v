module Transmitter (
    input wire clk,
    input reg [7:0] in,
    input wire transmit,
    output wire tx,
    output wire busy
);

parameter TX_IDLE      = 2'd0;
parameter TX_START_BIT = 2'd1;
parameter TX_DATA_BITS = 2'd2;
parameter TX_STOP_BIT  = 2'd3;

reg [7:0] data;
reg [1:0] state    = TX_IDLE;
reg [2:0] biti     = 0;
reg       tx_reg   = 1; // Signal is high by default
reg       busy_reg = 0;

reg baudclk;
BaudRateCounter #(.SAMPLING_RATE(1)) baud (
    .clk    (clk),
    .out    (baudclk)
);

assign tx = tx_reg;
assign busy = busy_reg;

always @(posedge baudclk) begin
    case (state)
    TX_IDLE: begin
        if (transmit)
            state <= TX_START_BIT;
    end 
    TX_START_BIT: begin
        data <= in;
        tx_reg <= 0;
        busy_reg <= 1;
        state <= TX_DATA_BITS;
    end
    TX_DATA_BITS: begin
        biti <= biti + 3'd1;
        tx_reg <= data[biti];
        if (biti == 3'd7)
            state <= TX_STOP_BIT;
    end
    TX_STOP_BIT: begin
        tx_reg <= 1;
        biti <= 0;
        busy_reg <= 0;
        state <= TX_IDLE;
    end
    endcase
end
endmodule
