module Receiver (
    input wire clk,
    input wire rx,
    output reg [7:0] out,
    output reg done
);

parameter OVERSAMPLING = 4;
parameter SAMPLE_SIZE = $clog2(OVERSAMPLING);
parameter READ_SAMPLE = OVERSAMPLING / 2;

parameter RX_IDLE      = 2'd0;
parameter RX_START_BIT = 2'd1;
parameter RX_DATA_BITS = 2'd2;
parameter RX_STOP_BIT  = 2'd3;

reg [7:0]           data;
reg [1:0]           state   = RX_IDLE;
reg [3:0]           biti    = 0;
reg [SAMPLE_SIZE:0] samples = 0;

reg baudclk;  
BaudRateCounter #(.SAMPLING_RATE(OVERSAMPLING)) baud (
    .clk    (clk),
    .out    (baudclk)
);

always @(posedge baudclk) begin
    case (state) 
    RX_IDLE: begin
        if (!rx) begin
            state <= RX_START_BIT;
            done <= 0;
            samples <= 1; // Detection of low rx counts as a sample
        end
    end
    RX_START_BIT: begin
        samples <= samples + 1;
        if (samples == OVERSAMPLING-1) begin
            state <= RX_DATA_BITS;
            samples <= 0;
        end
    end
    RX_DATA_BITS: begin
        samples <= samples + 1;
        if (samples == READ_SAMPLE) begin
            data[biti[2:0]] <= rx;
            biti <= biti + 0'b1;
        end
        if (samples == OVERSAMPLING-1) begin
            if (biti == 8)
                state <= RX_STOP_BIT;
            samples <= 0;
        end
    end
    RX_STOP_BIT: begin
        samples <= samples + 1;
        if (samples == OVERSAMPLING-1) begin
            state <= RX_IDLE;
            samples <= 0;
            biti <= 0;
            out <= data;
            done <= 1;
        end
    end
    default: begin
        $display("INVALID RECIEVER STATE");
        $finish;
    end
    endcase
end
endmodule
